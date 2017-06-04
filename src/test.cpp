
#include "vr/hashing.h"

#include <random>
#include <memory>

//----------------------------------------------------------------------------
bool
test_crc32 ()
{
    std::random_device rd;
    std::mt19937 g { rd () };
    std::uniform_int_distribution<> rng { 0, 255 };

    using namespace vr;

    int32_t const len_max           = 109;
    int32_t const len_repeats       = 5000;

    std::unique_ptr<uint8_t []> const buf { new uint8_t [len_max] };

    for (int32_t len = 0; len < len_max; ++ len)
    {
        for (int32_t r = 0; r < len_repeats; ++ r)
        {
            // create a "random string" of length 'len' in 'buf':

            for (int32_t i = 0; i < len; ++ i) buf [i] = rng (g);

            // check that the reference and "fast" impls compute the same value for this string:

            auto const h_ref    = crc32_reference (buf.get (), len, src_hash_seed ());
            auto const h_fast   = crc32 (buf.get (), len, src_hash_seed ());

            if (h_ref != h_fast)
                return false;
        }
    }

    return true;
}
//----------------------------------------------------------------------------
