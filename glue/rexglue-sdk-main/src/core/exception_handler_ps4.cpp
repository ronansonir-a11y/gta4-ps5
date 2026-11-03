/**
 * [ps4 pro] Orbis exception-handler install path.
 *
 * exception_handler_posix.cpp reads Linux gregs[]/REG_RIP out of mcontext_t;
 * OpenOrbis ships FreeBSD-style musl ucontext_t instead, so compiling that
 * file for PS4 would either produce an empty translation unit (missing
 * ExceptionHandler::Install / Uninstall, referenced by mmio_handler.cpp) or
 * fail on Linux-only register macros.
 *
 * Orbis is a hard-fail console (no recoverable SIGSEGV dispatch to the guest
 * MMIO handler), so this backend keeps the handler list but does not install
 * signal handlers. If MMIO faults turn out to matter on the PS4, replace this
 * with a FreeBSD mcontext adapter (ucontext_ps4.c documents the gregset
 * indices: RIP=16, RSP=15, ...).
 */
#include <rex/platform.h>

#if REX_PLATFORM_PS4

#include <rex/exception_handler.h>
#include <rex/assert.h>

namespace rex::arch {
namespace {
constexpr size_t kMaxHandlerCount = 16;
struct HandlerEntry {
  ExceptionHandler::Handler fn = nullptr;
  void* data = nullptr;
};
HandlerEntry g_handlers[kMaxHandlerCount]{};
}  // namespace

void ExceptionHandler::Install(Handler fn, void* data) {
  for (size_t i = 0; i < kMaxHandlerCount; ++i) {
    if (!g_handlers[i].fn) {
      g_handlers[i].fn = fn;
      g_handlers[i].data = data;
      return;
    }
  }
  assert_always("Too many exception handlers installed");
}

void ExceptionHandler::Uninstall(Handler fn, void* data) {
  for (size_t i = 0; i < kMaxHandlerCount; ++i) {
    if (g_handlers[i].fn == fn && g_handlers[i].data == data) {
      g_handlers[i] = {};
      return;
    }
  }
}
}  // namespace rex::arch

#endif  // REX_PLATFORM_PS4
