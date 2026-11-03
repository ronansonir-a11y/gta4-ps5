#pragma once
/* PS4 / OpenOrbis <math.h> compatibility shim.
 *
 * The PS4 build compiles C++23 sources against modern host libc++ headers but
 * links the OpenOrbis SDK's static libc++.  Host libc++'s own math.h wrapper
 * ends with global `using std::__math::isinf/isnan;` declarations; those
 * conflict with the static-inline global isinf/isnan supplied by the SDK's
 * musl math.h.  Keeping the SDK C headers ahead of libc++ instead breaks every
 * other libc++ C wrapper (cstddef/cstdint/cctype/...), so only math.h is
 * shadowed here.
 *
 * This header is placed before the host libc++ headers via -isystem.  We claim
 * libc++'s _LIBCPP_MATH_H guard so <cmath>'s "didn't find libc++'s math.h"
 * check passes, then include the Orbis C header directly by relative path.  Do
 * not use #include_next here: that would re-enter the host wrapper.
 */
#ifndef _LIBCPP_MATH_H
#  define _LIBCPP_MATH_H
#endif

#include "../../../tools/OpenOrbis-PS4-Toolchain/include/math.h"
