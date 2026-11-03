#pragma once
/* OpenOrbis / musl <signal.h> compatibility shim.
 *
 * The SDK header defines the sigaction union member as __sa_sigaction but
 * its convenience macro points at a non-existent `sa_sigaction` member:
 *   #define sa_sigaction __sa_handler.sa_sigaction
 * PS4 sources (seh_posix.cpp, threading_ps4.cpp, exception_handler_posix.cpp)
 * use sa.sa_sigaction / handler->sa_sigaction, which therefore fails to
 * compile.  Include the SDK header directly and correct just that macro.
 */
#include "../../../tools/OpenOrbis-PS4-Toolchain/include/signal.h"

#undef sa_sigaction
#define sa_sigaction __sa_handler.__sa_sigaction
