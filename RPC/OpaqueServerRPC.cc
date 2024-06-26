/* Copyright (c) 2012 Stanford University
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

#include <sys/socket.h>
#include <netinet/in.h>

#include "RPC/OpaqueServerRPC.h"
#include "Protocol/FlairProtocol.h"

namespace LogCabin {
namespace RPC {

OpaqueServerRPC::OpaqueServerRPC()
    : request()
    , response()
    , socket()
    , messageId(~0UL)
    , responseTarget(NULL)
    , is_flair(0)
{
}

OpaqueServerRPC::OpaqueServerRPC(
        std::weak_ptr<OpaqueServer::SocketWithHandler> socket,
        MessageSocket::MessageId messageId,
        Core::Buffer request,
        uint8_t is_flair,
        sockaddr* udp_addr,
        socklen_t* udp_addr_len)
    : request(std::move(request))
    , response()
    , socket(socket)
    , messageId(messageId)
    , responseTarget(NULL)
    , is_flair(is_flair)
{
    if (udp_addr)
        this->udp_addr = *udp_addr;
    if (udp_addr_len)
        this->udp_addr_len = *udp_addr_len;
}

OpaqueServerRPC::OpaqueServerRPC(OpaqueServerRPC&& other)
    : request(std::move(other.request))
    , response(std::move(other.response))
    , socket(std::move(other.socket))
    , messageId(std::move(other.messageId))
    , responseTarget(std::move(other.responseTarget))
    , is_flair(other.is_flair)
    , udp_addr(other.udp_addr)
    , udp_addr_len(other.udp_addr_len)
{
}

OpaqueServerRPC::~OpaqueServerRPC()
{
}

OpaqueServerRPC&
OpaqueServerRPC::operator=(OpaqueServerRPC&& other)
{
    request = std::move(other.request);
    response = std::move(other.response);
    socket = std::move(other.socket);
    messageId = std::move(other.messageId);
    responseTarget = std::move(other.responseTarget);
    is_flair = other.is_flair;
    udp_addr = other.udp_addr;
    udp_addr_len = other.udp_addr_len;
    return *this;
}

void
OpaqueServerRPC::closeSession()
{
    std::shared_ptr<OpaqueServer::SocketWithHandler> socketRef = socket.lock();
    if (socketRef)
        socketRef->monitor.close();
    socket.reset();
    responseTarget = NULL;
}

void
OpaqueServerRPC::sendReply()
{
    std::shared_ptr<OpaqueServer::SocketWithHandler> socketRef = socket.lock();
    if (socketRef) {
        if (!is_flair)
            socketRef->monitor.sendMessage(messageId, std::move(response));
        else
            socketRef->monitor.sendMessage(messageId, std::move(response), &udp_addr, &udp_addr_len);
    } else {
        // During normal operation, this indicates that either the socket has
        // been disconnected or the reply has already been sent.

        // For unit testing only, we can store replies from mock RPCs
        // that have no sessions.
        if (responseTarget != NULL) {
            *responseTarget = std::move(response);
            responseTarget = NULL;
        }

        // Drop the reply on the floor.
        response.reset();
    }
    // Prevent the server from replying again.
    socket.reset();
}

} // namespace LogCabin::RPC
} // namespace LogCabin
