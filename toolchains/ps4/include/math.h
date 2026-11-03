#pragma once
/* PS4 / OpenOrbis <math.h> compatibility shim.
 *
 * The PS4 build uses modern host libc++ headers (C++23) with the OpenOrbis SDK
 * C library.  Host libc++'s math.h wrapper adds global
 * `using std::__math::isinf/isnan;` declarations; the SDK's C++ math.h defines
 * static-inline global isinf/isnan returning int, while libc++'s preferred
 * overloads return bool with the same signatures.  Those cannot coexist.
 *
 * In C++ mode we rename the SDK's two conflicting static-inline functions
 * before including the Orbis header, then hand off to libc++'s own math.h
 * wrapper via #include_next.  The wrapper still supplies all std::__math
 * functions, includes <stdlib.h> (so <cmath>/<cstdlib> see the same ::abs
 * overload set) and defines _LIBCPP_MATH_H for <cmath>.  In C mode the SDK's
 * classification macros are left untouched and the wrapper is a no-op.
 */
#ifdef __cplusplus
#  define isinf   __ps4_sdk_isinf
#  define isnan   __ps4_sdk_isnan
#endif

#include "../../../tools/OpenOrbis-PS4-Toolchain/include/math.h"

#ifdef __cplusplus
#  undef isinf
#  undef isnan
#endif

#include_next <math.h>
