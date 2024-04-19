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
    flair_hdr.seq = h64tobe(flair_hdr.seq);
    flair_hdr.sid = h32tohe(flair_hdr.sid);
    flair_hdr.log_idx = h64tobe(flair_hdr.log_idx);
}

}
}
}