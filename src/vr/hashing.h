#pragma once

#include "vr/crc32.h"

#include <cstring>
#include <string>

//----------------------------------------------------------------------------
namespace vr
{

constexpr uint32_t crc32_hash_seed ()     { return -1; }

//............................................................................
/**
 * a user-defined string literal for static string hashing, to be used together
 * with @ref str_hash() along the lines of:
 * @code
 *   char const * s = ...;
 *
 *   switch (str_hash (s))
 *   {
 *      case "ABCD"_hash:  ...;
 *
 *      case "XYZ"_hash:   ...;
 *      ...
 *   }
 * @endcode
 */
constexpr uint32_t
operator "" _hash (char const * const str, size_t const len)
{
    return crc32_constexpr (str, len, crc32_hash_seed ());
}
//............................................................................
/**
 * dynamically compute the same hash value as @ref _hash()
 *
 * @param str [does not need to be 0-terminated]
 * @param len number of chars in 'str' to hash
 */
inline uint32_t
str_hash (char const * const str, int32_t const len)
{
    return crc32 (reinterpret_cast<uint8_t const *> (str), len, crc32_hash_seed ());
}

/**
 * @param str [must be 0-terminated]
 */
inline uint32_t
str_hash (char const * const str)
{
    return str_hash (str, std::strlen (str));
}

inline uint32_t
str_hash (std::string const & str)
{
    return str_hash (str.data (), str.size ());
}

} // end of namespace
//----------------------------------------------------------------------------
