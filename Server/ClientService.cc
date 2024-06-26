/* Copyright (c) 2012 Stanford University
 * Copyright (c) 2015 Diego Ongaro
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <string.h>

#include "build/Protocol/Client.pb.h"
#include "Core/Buffer.h"
#include "Core/ProtoBuf.h"
#include "Core/Time.h"
#include "RPC/ServerRPC.h"
#include "Server/RaftConsensus.h"
#include "Server/ClientService.h"
#include "Server/Globals.h"
#include "Server/StateMachine.h"

// #define NO_SWITCH

namespace LogCabin {
namespace Server {

typedef RaftConsensus::ClientResult Result;

ClientService::ClientService(Globals& globals)
    : globals(globals)
{
}

ClientService::~ClientService()
{
}

void
ClientService::handleRPC(RPC::ServerRPC rpc)
{
    using Protocol::Client::OpCode;

    // Call the appropriate RPC handler based on the request's opCode.
    switch (rpc.getOpCode()) {
        case OpCode::GET_SERVER_INFO:
            getServerInfo(std::move(rpc));
            break;
        case OpCode::VERIFY_RECIPIENT:
            verifyRecipient(std::move(rpc));
            break;
        case OpCode::GET_CONFIGURATION:
            getConfiguration(std::move(rpc));
            break;
        case OpCode::SET_CONFIGURATION:
            setConfiguration(std::move(rpc));
            break;
        case OpCode::STATE_MACHINE_COMMAND:
            stateMachineCommand(std::move(rpc));
            break;
        case OpCode::STATE_MACHINE_QUERY:
            stateMachineQuery(std::move(rpc));
            break;
        default:
            WARNING("Received RPC request with unknown opcode %u: "
                    "rejecting it as invalid request",
                    rpc.getOpCode());
            rpc.rejectInvalidRequest();
    }
}

std::string
ClientService::getName() const
{
    return "ClientService";
}


/**
 * Place this at the top of each RPC handler. Afterwards, 'request' will refer
 * to the protocol buffer for the request with all required fields set.
 * 'response' will be an empty protocol buffer for you to fill in the response.
 */
#define PRELUDE(rpcClass) \
    Protocol::Client::rpcClass::Request request; \
    Protocol::Client::rpcClass::Response response; \
    if (!rpc.getRequest(request)) \
        return;

////////// RPC handlers //////////


void
ClientService::getServerInfo(RPC::ServerRPC rpc)
{
    PRELUDE(GetServerInfo);
    Protocol::Client::Server& info = *response.mutable_server_info();
    info.set_server_id(globals.raft->serverId);
    info.set_addresses(globals.raft->serverAddresses);
    rpc.reply(response);
}

void
ClientService::getConfiguration(RPC::ServerRPC rpc)
{
    PRELUDE(GetConfiguration);
    Protocol::Raft::SimpleConfiguration configuration;
    uint64_t id;
    Result result = globals.raft->getConfiguration(configuration, id);
    if (result == Result::RETRY || result == Result::NOT_LEADER) {
        Protocol::Client::Error error;
        error.set_error_code(Protocol::Client::Error::NOT_LEADER);
        std::string leaderHint = globals.raft->getLeaderHint();
        if (!leaderHint.empty())
            error.set_leader_hint(leaderHint);
        rpc.returnError(error);
        return;
    }
    response.set_id(id);
    for (auto it = configuration.servers().begin();
         it != configuration.servers().end();
         ++it) {
        Protocol::Client::Server* server = response.add_servers();
        server->set_server_id(it->server_id());
        server->set_addresses(it->addresses());
    }
    rpc.reply(response);
}

void
ClientService::setConfiguration(RPC::ServerRPC rpc)
{
    PRELUDE(SetConfiguration);
    Result result = globals.raft->setConfiguration(request, response);
    if (result == Result::RETRY || result == Result::NOT_LEADER) {
        Protocol::Client::Error error;
        error.set_error_code(Protocol::Client::Error::NOT_LEADER);
        std::string leaderHint = globals.raft->getLeaderHint();
        if (!leaderHint.empty())
            error.set_leader_hint(leaderHint);
        rpc.returnError(error);
        return;
    }
    rpc.reply(response);
}

void
ClientService::stateMachineCommand(RPC::ServerRPC rpc)
{
    #ifndef NO_SWITCH
    {
        std::pair<Result, uint64_t> result = globals.raft->getLastCommitIndex();
        if (result.first == Result::SUCCESS)
            if (rpc.isFlair() && !globals.raft->cmpFlairPair(rpc.flair_hdr.sid, rpc.flair_hdr.seq)) {
                rpc.handleStaleRPC();
                return;
            }
    }
    #endif

    PRELUDE(StateMachineCommand);
    Core::Buffer cmdBuffer;
    rpc.getRequest(cmdBuffer);
    std::pair<Result, uint64_t> result = globals.raft->replicate(cmdBuffer);
    uint8_t cflwrs = globals.raft->getFlairConsistentFollowers();
    if (result.first == Result::RETRY || result.first == Result::NOT_LEADER) {
        rpc.flair_hdr.from_leader = 0;
        rpc.flair_hdr.opcode = Protocol::FlairProtocol::OP_WRITE_FAILED;
        Protocol::Client::Error error;
        error.set_error_code(Protocol::Client::Error::NOT_LEADER);
        std::string leaderHint = globals.raft->getLeaderHint();
        if (!leaderHint.empty())
            error.set_leader_hint(leaderHint);
        rpc.returnError(error);
        return;
    }
    assert(result.first == Result::SUCCESS);
    uint64_t logIndex = result.second;
    rpc.flair_hdr.from_leader = 1;
    rpc.flair_hdr.opcode = Protocol::FlairProtocol::OP_WRITE_REPLY;
    rpc.flair_hdr.log_idx = logIndex;
    rpc.flair_hdr.cflwrs = cflwrs;
    if (!globals.stateMachine->waitForResponse(logIndex, request, response)) {
        rpc.flair_hdr.opcode = Protocol::FlairProtocol::OP_WRITE_FAILED;
        rpc.rejectInvalidRequest();
        return;
    }
    if (rpc.isFlair()) {
        globals.raft->setFlairPair(rpc.flair_hdr.sid, rpc.flair_hdr.seq);
    }
    rpc.reply(response);
}

void
ClientService::stateMachineQuery(RPC::ServerRPC rpc)
{
    if (!rpc.isFlair())
    {
        PRELUDE(StateMachineQuery);
        std::pair<Result, uint64_t> result = globals.raft->getLastCommitIndex();
        if (result.first == Result::RETRY || result.first == Result::NOT_LEADER) {
            Protocol::Client::Error error;
            error.set_error_code(Protocol::Client::Error::NOT_LEADER);
            std::string leaderHint = globals.raft->getLeaderHint();
            if (!leaderHint.empty())
                error.set_leader_hint(leaderHint);
            rpc.returnError(error);
            return;
        }
        assert(result.first == Result::SUCCESS);
        uint64_t logIndex = result.second;
        globals.stateMachine->wait(logIndex);
        if (!globals.stateMachine->query(request, response))
        {
            rpc.rejectInvalidRequest();
            return;
        }
        rpc.reply(response);
    }
    else
    {
        rpc.flair_hdr.opcode = Protocol::FlairProtocol::OP_READ_REPLY;
        PRELUDE(StateMachineQuery);
        std::pair<Result, uint64_t> result = globals.raft->getLastCommitIndex();
        uint8_t is_leader = result.first == Result::SUCCESS ? 1:0;
        rpc.flair_hdr.opcode = Protocol::FlairProtocol::OP_READ_REPLY;
        uint64_t logIndex;
        if (rpc.flair_hdr.log_idx == 0xffffffffffffffff) {
            if (!is_leader) {
                // TODO: Tell the switch which server is the current leader.
                // rpc.flair_hdr.opcode = Protocol::FlairProtocol::OP_READ_FAILED;
            }
            rpc.flair_hdr.from_leader = 1;
            logIndex = result.second;
        }
        else {
            rpc.flair_hdr.from_leader = 0;
            logIndex = rpc.flair_hdr.log_idx;
        }
        globals.stateMachine->wait(logIndex);
        if (!globals.stateMachine->query(request, response))
        {
            rpc.flair_hdr.opcode = Protocol::FlairProtocol::OP_READ_FAILED;
            rpc.rejectInvalidRequest();
            return;
        }
        rpc.reply(response);
    }
}

void
ClientService::verifyRecipient(RPC::ServerRPC rpc)
{
    PRELUDE(VerifyRecipient);

    std::string clusterUUID = globals.clusterUUID.getOrDefault();
    uint64_t serverId = globals.serverId;

    if (!clusterUUID.empty())
        response.set_cluster_uuid(clusterUUID);
    response.set_server_id(serverId);

    if (request.has_cluster_uuid() &&
        !request.cluster_uuid().empty() &&
        !clusterUUID.empty() &&
        clusterUUID != request.cluster_uuid()) {
        response.set_ok(false);
        response.set_error(Core::StringUtil::format(
           "Mismatched cluster UUIDs: request intended for %s, "
           "but this server is in %s",
           request.cluster_uuid().c_str(),
           clusterUUID.c_str()));
    } else if (request.has_server_id() &&
               serverId != request.server_id()) {
        response.set_ok(false);
        response.set_error(Core::StringUtil::format(
           "Mismatched server IDs: request intended for %lu, "
           "but this server is %lu",
           request.server_id(),
           serverId));
    } else {
        response.set_ok(true);
        if (clusterUUID.empty() &&
            request.has_cluster_uuid() &&
            !request.cluster_uuid().empty()) {
            NOTICE("Adopting cluster UUID %s",
                   request.cluster_uuid().c_str());
            globals.clusterUUID.set(request.cluster_uuid());
            response.set_cluster_uuid(request.cluster_uuid());
        }
    }
    rpc.reply(response);
}

} // namespace LogCabin::Server
} // namespace LogCabin
