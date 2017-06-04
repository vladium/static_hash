#pragma once

#include "vr/types.h"

#include <x86intrin.h> // actually only need <nmmintrin.h> for SSE4.2

#if !defined (VR_FORCE_INLINE)
#   define VR_FORCE_INLINE __attribute__((always_inline))
#endif

//----------------------------------------------------------------------------
namespace vr
{

inline VR_FORCE_INLINE uint32_t
crc32 (uint32_t const crc, uint8_t  const x)
{ return _mm_crc32_u8  (crc, x); }

inline VR_FORCE_INLINE uint32_t
crc32 (uint32_t const crc, uint16_t const x)
{ return _mm_crc32_u16 (crc, x); }

inline VR_FORCE_INLINE uint32_t
crc32 (uint32_t const crc, uint32_t const x)
{ return _mm_crc32_u32 (crc, x); }

inline VR_FORCE_INLINE uint32_t
crc32 (uint32_t const crc, uint64_t const x)
{ return _mm_crc32_u64 (crc, x); } // upper 32 bits are not used

} // end of namespace
//----------------------------------------------------------------------------
