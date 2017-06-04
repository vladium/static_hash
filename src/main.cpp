
#include "vr/crc32.h"

#include <cstring>
#include <iostream>

int
main (int32_t ac, char * av [])
{
    using namespace vr;

    uint8_t const * v = reinterpret_cast<uint8_t const *> (av [1]);

    uint32_t const crc_ref = crc32_reference (v, std::strlen (av [1]), 1);
    uint32_t const h = crc32 (v, std::strlen (av [1]), 1);

    std::cout << "crc('" << av [1] << "') = " << std::hex << crc_ref << ", h = " << h << std::endl;

    switch (h)
    {
        case "abcd"_hash:           std::cout << "abcd" << std::endl; break;
        case "fgh"_hash:            std::cout << "fgh" << std::endl; break;
        case "abracadabra"_hash:    std::cout << "abracadabra" << std::endl; break;

        default: std::cout << "(no match)" << std::endl; break;

    } // end of switch

    return 0;
}
