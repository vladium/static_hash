
#include "vr/hashing.h"

extern bool test_crc32 (); // in "test.cpp"

#include <iostream>

//----------------------------------------------------------------------------
int
main (int32_t ac, char * av [])
{
    using namespace vr;

    if (ac != 2)
    {
        std::cerr << "usage: static_hash (<string>|'test')" << std::endl;
        return 1;
    }

    uint32_t h;
    switch (h = str_hash (av [1]))
    {
        case "abcd"_hash:           std::cout << "abcd" << std::endl; break;
        case "fgh"_hash:            std::cout << "fgh" << std::endl; break;
        case "abracadabra"_hash:    std::cout << "abracadabra" << std::endl; break;

        case "test"_hash:
            std::cout << "running test_crc32() ..." << std::flush;
            std::cout << ", success: " << test_crc32 () << std::endl;
            break;

    } // end of switch

    std::cout << '\'' << av [1] << "' hashed to 0x" << std::hex << h << std::endl;

    return 0;
}
//----------------------------------------------------------------------------
