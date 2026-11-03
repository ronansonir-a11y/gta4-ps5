# PS4 Pro (OpenOrbis) build — what this patch does and what is still missing

Apply with `git apply ps4-pro.patch` on `elripalda/gta4-ps5` @ `c628283`.

## What is implemented

1. **`glue/` unblocked for PS4**
   - `glue/CMakeLists.txt`: `REXGLUE_USE_VULKAN` is `OFF` only for PS4 (`ON` stays for the PS5/RADV build), `REXGLUE_ENABLE_FIDELITYFX=OFF`, `LIBERTY_RECOMP_GNM=ON`.
   - `glue/rexglue-sdk-main/CMakeLists.txt`: backend `FATAL_ERROR` relaxed for `LIBERTY_RECOMP_GNM`, added `elseif(PS4)` platform detection (`REX_PLATFORM="ps4-amd64"`, no `ps5_compat.h`).
   - `src/core/CMakeLists.txt`: real `elseif(PS4)` source list using the existing `memory_ps4.cpp`, `threading_ps4.cpp`, `dynlib_ps4.cpp`, `fiber_ps4.cpp`, `logging_ps4.cpp`, `net_ps4.cpp`, `ucontext_ps4.c` instead of the POSIX counterparts (avoids duplicate symbols and the `REX_PLATFORM_LINUX` static_assert). The orphaned `keyboard_dialog_ps4.cpp` is excluded because its `rex/kernel/xam/keyboard_dialog.h` API and frontend are absent from this checkout.
   - `src/input/CMakeLists.txt` + `input_system.cpp`: `ps4/ps4_input_driver.cpp` and a `REX_PLATFORM_PS4` branch in `CreateDefaultInputSystem()`.
   - `src/audio/CMakeLists.txt`: `orbis/orbis_audio_system.cpp` + `orbis_audio_driver.cpp`.
   - `src/graphics/CMakeLists.txt`: `rexgpu-xenos` skipped for PS4 (the `REXGLUE_USE_VULKAN` block with `rexgpu-gta4-native` is skipped automatically).
   - `gta4-recomp/CMakeLists.txt`: `elseif(PS4)` INTERFACE consumer with `LIBERTY_RECOMP_PS4`, 1920x1080 defines, no PS5 hub/RADV/FSR3.
   - `toolchains/ps4-openorbis.cmake`: sets `PS4`, `-march=btver2` (Jaguar), generates an `orbis-ld` wrapper in the build tree, makes `libcxx_ps4_compat.a` optional, adds `LIBERTY_PS4_NEO_MODE` / `LIBERTY_PS4_GAME_ROOT` / 1920x1080 cache variables. Root CMakeLists no longer injects `-march=sandybridge` for PS4.

2. **Neo memory + 1080p**
   - `LibertyRecomp/os/ps4/user_malloc_ps4.cpp`: `sceKernelIsNeoMode()` + `sceKernelGetDirectMemorySize()`, `HEAP_TOTAL_NEO = 5120 * MB`, fallback `HEAP_TOTAL_BASE = 4608 * MB`, and the kernel-reported size as a hard cap.
   - `glue/rexglue-sdk-main/src/core/memory_ps4.cpp`: `searchEnd` from `sceKernelGetDirectMemorySize()` instead of `SCE_KERNEL_MAIN_DMEM_SIZE`, new fallback ladder 5 GiB → 4.5 GiB → 4/2/1 GiB, logs Neo mode.
   - `src/ui/window.cpp`: `resolution` defaults to `"1080p"` on `REX_PLATFORM_PS4` (`video_mode_util.h` maps it to 1920x1080).
   - `src/graphics/pipeline/texture/cache.cpp`: PS4 soft/hard texture cache 192/384 MB instead of 384/768 MB.
   - `gta4-recomp/src/gta4_app.cpp`: PS4 defaults 256 shadow-map base, shadow distance 1.0, reflections `original` / cap `1080p`, FXAA, draw distance 1.0.
   - `LibertyRecomp/CMakeLists.txt`: `LIBERTY_PS4_NEO_MODE` → `ATTRIBUTE = 1 << 23 = 8388608` in `param.sfo` (both the PkgTool.Core path and the Python path), plus a self-contained `tools/ps4/sfo_neo.py` generator/patcher so a stock OpenOrbis checkout (which has no `make_param_sfo.py`) still produces a real SFO.

3. **No 7 GB payload at build time**
   - `CMakeLists.txt`: `LIBERTY_RECOMP_SKIP_GAME_PAYLOAD` option. When ON, the root CMake creates four zero-byte stubs in `${CMAKE_BINARY_DIR}/ps4_empty_game_payload/`, repoints `LIBERTY_RECOMP_EMBEDDED_GAME_PATH` there and skips both FATAL_ERROR validations (root and `LibertyRecomp/CMakeLists.txt`).
   - `LibertyRecomp/CMakeLists.txt`: skips `copy_directory` of the payload into the PKG staging area when the option is ON.
   - `CMakePresets.json`: new `ps4-pro-release` configure + build presets with all of the above ON.
   - `.github/workflows/build-ps4-pro.yml`: Ubuntu 24.04 CI job that installs clang/lld/ninja/CMake 3.31, fetches the OpenOrbis repo + the v0.5.3 prebuilt host binaries, runs `./setup.sh`, configures `ps4-pro-release`, builds `LibertyRecomp` (→ `eboot.bin`) and `LibertyRecomp-sfo` (→ `param.sfo`), verifies the Neo bit and uploads both files as an artefact.

## Where the PS4 code reads the game files at runtime

**Not `/data/gta4/` in the original code.** `LibertyRecomp/install/embedded_assets.cpp::ResolveEmbeddedGameRoot()` and `LibertyRecomp/install/platform_paths.cpp::PlatformPaths::GetGameDirectory()` both hard-coded `/app0/game` — the read-only PKG mount. `LibertyRecomp/os/ps4/ps4_main.cpp` itself only initialises the heap, sceUserService, sysmodules, net, trophies and the save-data mount (`/user/data/LBTY00001/`, exposed as `/savedata0`), then calls `main()`; it does not open any RPF.

This patch adds `LibertyRecomp/install/ps4_game_root.h`, used by both functions:

- default: `/app0/game` (compiled from the `LIBERTY_PS4_GAME_ROOT` CMake cache variable);
- override at runtime: the `LIBERTY_PS4_GAME_ROOT` **environment variable**, e.g. `LIBERTY_PS4_GAME_ROOT=/data/gta4`.

So to supply the real files yourself, extract the USA Xbox 360 disc + TU8 into `/data/gta4/` on the jailbroken console (or any readable path), set the env var (or bake another default with `-DLIBERTY_PS4_GAME_ROOT=/data/gta4` at configure time), and put `default.xex`, `common.rpf`, `xbox360.rpf`, `audio.rpf` there. Saves still go to `/user/data/LBTY00001/` and the trophy pack to `sce_sys/`.

## Honest limitations — this is a revived configure/build path, not a guaranteed green build

I could not compile `eboot.bin` in this sandbox: it has only Python, `gcc` and `make` — no `cmake`, `clang`, `lld`, `ninja` or the OpenOrbis SDK. The workflow is the executable equivalent. Remaining upstream gaps that a CI run will expose:

- The GNM renderer in `LibertyRecomp/gpu/video.cpp` is a legacy D3D11-style path; `toolchains/ps4-openorbis.cmake` claims it is used on PS4, but no `LIBERTY_RECOMP_GNM` compile switch exists in `LibertyRecomp/CMakeLists.txt` and `glue/CMakeLists.txt` previously forced Vulkan. This patch removes the blocker but does not rewrite the renderer backend selection; expect link/render work.
- The PS4 core source list in `src/core/CMakeLists.txt` is new. `exception_handler_posix.cpp` is replaced by `exception_handler_ps4.cpp` (Linux `gregs[REG_RIP]` does not exist in Orbis musl); the new backend keeps the handler list but does **not** install SIGSEGV/SIGBUS handlers, so guest MMIO faults are not recovered. Some other POSIX files (`mapped_memory_posix.cpp` → `shm_open`, `filesystem_posix.cpp` → `<ftw.h>`/`<pwd.h>`) may need Orbis-specific stubs; verify symbol uniqueness and missing headers on first compile. All eight `static_assert(REX_PLATFORM_LINUX || REX_PLATFORM_MAC, ...)` in `src/core/*_posix.cpp` now also accept `REX_PLATFORM_PS4`.
- Stock OpenOrbis has no `libcxx_ps4_compat.a` (it is a locally rebuilt LLVM 21 libc++); the toolchain now skips it. C++23 headers then come from the host clang, which may need additional `-isystem` tuning.
- `create-fself` from release v0.5.3 is used to produce `eboot.bin`; `create-gp4`/`PkgTool.Core` PKG generation is best-effort (`continue-on-error: true` in CI).
- The PS5/RADV tree (`ps5/`, `glue/.../src/ps5/`, `patches/liberty-ps5.patch`) is untouched. All new branches are guarded by `PS4` / `LIBERTY_RECOMP_TARGET_PLATFORM STREQUAL "ps4"`.

## Local build (Ubuntu 24.04)

```bash
git clone --depth 1 https://github.com/elripalda/gta4-ps5.git && cd gta4-ps5
git apply /path/to/ps4-pro.patch
export OO_PS4_TOOLCHAIN="$PWD/tools/OpenOrbis-PS4-Toolchain"
./setup.sh
cmake --preset ps4-pro-release -DLIBERTY_PS4_GAME_ROOT=/data/gta4
cmake --build out/build/ps4-pro-release --target LibertyRecomp LibertyRecomp-sfo -j2
python3 tools/ps4/sfo_neo.py inspect out/build/ps4-pro-release/pkg_staging/sce_sys/param.sfo
```
