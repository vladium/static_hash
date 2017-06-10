#pragma once

#if !defined (__GNUC__)
#   error this demo code requires gcc/clang
#endif

//----------------------------------------------------------------------------

#if !defined (VR_FORCE_INLINE)
#   define VR_FORCE_INLINE      inline __attribute__((always_inline))
#endif

#if !defined (VR_UNLIKELY)
#   define VR_UNLIKELY(c)       __builtin_expect(static_cast<bool> (c), 0)
#endif

//----------------------------------------------------------------------------
