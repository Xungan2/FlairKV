#include "Core/Endian.h"
#include "Protocol/FlairProtocol.h"

namespace LogCabin {
namespace Protocol {
namespace FlairProtocol{

void 
fromBigEndian(FlairProtocol& flair_hdr)
{
    flair_hdr.seq = be64toh(flair_hdr.seq);
    flair_hdr.sid = be32toh(flair_hdr.sid);
    flair_hdr.log_idx = be64toh(flair_hdr.log_idx);
}

void 
toBigEndian(FlairProtocol& flair_hdr)
{
    flair_hdr.seq = htobe64(flair_hdr.seq);
    flair_hdr.sid = htobe32(flair_hdr.sid);
    flair_hdr.log_idx = htobe64(flair_hdr.log_idx);
}

}
}
}