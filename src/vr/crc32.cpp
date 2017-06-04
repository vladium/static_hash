
#include "vr/crc32.h"

#include <cassert>

//----------------------------------------------------------------------------
namespace vr
{

uint32_t
crc32_reference (uint8_t const * buf, int32_t len, uint32_t crc)
{
    assert (crc > 0);

    while (len --)
    {
        crc = (crc >> 8) ^ impl::g_crctable [(crc ^ (* buf ++)) & 0xFF];
    }

    return crc; // iSCSI would return (~ crc) after initializing 'crc' to -1
}

} // end of namespace
//----------------------------------------------------------------------------
