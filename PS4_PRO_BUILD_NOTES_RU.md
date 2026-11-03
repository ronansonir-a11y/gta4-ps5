# PS4 Pro / OpenOrbis: что сделано, где лежат файлы игры, как собрать

Патч: `ps4-pro.patch` (применяется к `elripalda/gta4-ps5`, коммит `c628283b018ea46077e0e30ff4cac7ab959a6de5`).
Ветка PS5 / RADV (`ps5/`, `glue/.../src/ps5/`, `patches/liberty-ps5.patch`) **не изменена**.

---

## 1. Что исправлено в `glue/` (PS4 больше не упирается в Vulkan)

- `glue/CMakeLists.txt`: для PS4 `REXGLUE_USE_VULKAN=OFF`, `REXGLUE_USE_D3D12=OFF`, `REXGLUE_ENABLE_FIDELITYFX=OFF`, `LIBERTY_RECOMP_GNM=ON`. Для всех остальных (включая PS5/RADV) осталось `REXGLUE_USE_VULKAN=ON`, как было.
- `glue/rexglue-sdk-main/CMakeLists.txt`: проверка «хотя бы один бэкенд» теперь разрешает и `LIBERTY_RECOMP_GNM`; добавлена ветка `if(PS4)` **перед** `UNIX`, чтобы Orbis не определялся как Linux; для PS4 не подключается `ps5_compat.h` и не включается `-mcmodel=large`.
- `src/core/CMakeLists.txt`: настоящая ветка `elseif(PS4)` с уже существующими `memory_ps4.cpp`, `threading_ps4.cpp`, `dynlib_ps4.cpp`, `fiber_ps4.cpp`, `logging_ps4.cpp`, `net_ps4.cpp`, `ucontext_ps4.c`. Осиротевший `keyboard_dialog_ps4.cpp` исключён: в этом чекауте нет заголовка `rex/kernel/xam/keyboard_dialog.h` и вызывающего его frontend-кода. Вместо `exception_handler_posix.cpp` (Linux `gregs[REG_RIP]`) добавлен `exception_handler_ps4.cpp`. Восемь `static_assert(REX_PLATFORM_LINUX || REX_PLATFORM_MAC, ...)` дополнены `|| REX_PLATFORM_PS4`.
- `src/input/CMakeLists.txt` + `input_system.cpp`: подключён `ps4/ps4_input_driver.cpp` и ветка `REX_PLATFORM_PS4` в `CreateDefaultInputSystem()`.
- `src/audio/CMakeLists.txt` + `src/ui/rex_app.cpp`: подключены `orbis/orbis_audio_system.cpp`, `orbis_audio_driver.cpp` и выбран `OrbisAudioSystem` как `audio_factory` для PS4 (SDL-бэкенд на Orbis не работает).
- Публичные заголовки вынесены в `include/rex/input/ps4/` и `include/rex/audio/orbis/`, чтобы исполняемый модуль мог их видеть; старые `src/.../*.h` стали тонкими forwarding-заголовками.
- `src/graphics/CMakeLists.txt`: `rexgpu-xenos` на PS4 не собирается; блок `REXGLUE_USE_VULKAN` с `rexgpu-gta4-native` пропускается автоматически.
- `gta4-recomp/CMakeLists.txt`: добавлена ветка `elseif(PS4)` (INTERFACE-библиотека с `LIBERTY_RECOMP_PS4` и 1920×1080), без PS5 hub / RADV / FSR3.
- `toolchains/ps4-openorbis.cmake`: выставляет `PS4`, `-march=btver2` (Jaguar), сам создаёт `orbis-ld` в каталоге сборки, делает `libcxx_ps4_compat.a` необязательным, добавляет опции `LIBERTY_PS4_NEO_MODE`, `LIBERTY_PS4_GAME_ROOT`, 1920/1080. Корневой `CMakeLists.txt` больше не подсовывает PS4 `-march=sandybridge`.

## 2. PS4 Pro / Neo и 1080p

- `LibertyRecomp/os/ps4/user_malloc_ps4.cpp`: `sceKernelIsNeoMode()` + `sceKernelGetDirectMemorySize()`; `HEAP_TOTAL_NEO = 5120 * MB` (5 GiB), `HEAP_TOTAL_BASE = 4608 * MB` (4.5 GiB) как fallback; размер, который вернул kernel, — жёсткий потолок.
- `glue/rexglue-sdk-main/src/core/memory_ps4.cpp`: `searchEnd` берётся из `sceKernelGetDirectMemorySize()`, а не из константы `SCE_KERNEL_MAIN_DMEM_SIZE`; лестница fallback 5 GiB → 4.5 GiB → 4/2/1 GiB; в лог выводится Neo mode.
- `src/ui/window.cpp`: `resolution` по умолчанию `"1080p"` на `REX_PLATFORM_PS4` (в `video_mode_util.h` это 1920×1080), а не `"4k"` как на PS5.
- `src/graphics/pipeline/texture/cache.cpp`: кэш текстур на PS4 192 / 384 МБ вместо 384 / 768 МБ.
- `gta4-recomp/src/gta4_app.cpp`: на PS4 тени 256 вместо 512, `gta4_shadow_distance_scale = 1.0`, отражения `original` с потолком `1080p`, FXAA вместо SMAA, `gta4_draw_distance_scale = 1.0` вместо 3.0.
- `LibertyRecomp/CMakeLists.txt`: опция `LIBERTY_PS4_NEO_MODE` (по умолчанию ON) выставляет бит 23 в `ATTRIBUTE` файла `param.sfo`: `1 << 23 = 0x800000 = 8388608`. Именно этот бит включает Neo/Pro и даёт +512 МиБ direct memory. Бит проставляется и в ветке PkgTool.Core, и в Python-ветке.
- Так как в OpenOrbis **нет** `scripts/make_param_sfo.py` (проверено: 404), добавлен самодостаточный `tools/ps4/sfo_neo.py`, который умеет `create`, `patch`, `inspect` настоящего SFO (формат PSF, `0x0404` = uint32, `0x0204` = UTF-8). Даже если генератор отсутствует, `param.sfo` будет настоящим, а не пустой заглушкой.

## 3. `eboot.bin` без 7 ГБ ассетов

- Новая опция `LIBERTY_RECOMP_SKIP_GAME_PAYLOAD` (по умолчанию OFF, в новом пресете ON). При ON корневой `CMakeLists.txt` создаёт в **каталоге сборки** `ps4_empty_game_payload/` с четырьмя пустыми файлами `default.xex`, `common.rpf`, `xbox360.rpf`, `audio.rpf`, переопределяет на него `LIBERTY_RECOMP_EMBEDDED_GAME_PATH` и не падает на `FATAL_ERROR`.
- В `LibertyRecomp/CMakeLists.txt` снята вторая проверка тех же файлов и оба `copy_directory` в PKG-staging (переменная `_ps4_copy_game_payload`), чтобы в PKG не тащились пустышки/7 ГБ.
- `CMakePresets.json`: новый пресет `ps4-pro-release` (configure + build) со всеми нужными `cacheVariables`.
- `.github/workflows/build-ps4-pro.yml`: Ubuntu 24.04, clang/lld/ninja/CMake 3.31, скачивает репозиторий OpenOrbis и готовые host-бинарники релиза `v0.5.3` (`create-fself`, `create-gp4`, `PkgTool.Core`, `include/orbis`, `link.x`, `lib/crt1.o`), запускает `./setup.sh`, конфигурирует `ps4-pro-release`, собирает `LibertyRecomp` → `eboot.bin` и `LibertyRecomp-sfo` → `sce_sys/param.sfo`, проверяет бит 23 и выкладывает оба файла артефактом `gta4-ps4-pro-eboot`. PKG собирается best-effort (`continue-on-error: true`).
- В `.gitignore` добавлены исключения `!/.github/workflows/build-ps4-pro.yml` и `!/tools/ps4/sfo_neo.py` — иначе репозиторий игнорировал бы оба файла.

---

## Где PS4-код читает файлы игры (главный вопрос)

**В оригинале — `/app0/game`, а не `/data/gta4/`.**

- `LibertyRecomp/install/embedded_assets.cpp` → `EmbeddedAssets::ResolveEmbeddedGameRoot()` возвращал `"/app0/game"`.
- `LibertyRecomp/install/platform_paths.cpp` → `PlatformPaths::GetGameDirectory()` возвращал `"/app0/game"`.
- DLC: `/app0/dlc`, манифест: `/app0/manifest.json`, AES-ключ: `/app0/aes_key.bin`.
- Запись: `/user/data/LBTY00001/` (кэш и `shader_cache` внутри), сохранения монтируются как `/savedata0`.
- `LibertyRecomp/os/ps4/ps4_main.cpp` сам RPF не открывает: он инициализирует кучу, `sceUserService`, sysmodules, сеть, трофеи и mount сохранений, затем зовёт `main()`.
- При сборке PKG `LIBERTY_RECOMP_EMBEDDED_GAME_PATH` копировался в staging `game/`, что на консоли и превращалось в `/app0/game` (read-only).

Патч добавляет `LibertyRecomp/install/ps4_game_root.h`, и обе функции теперь берут путь оттуда:

| Что | Путь |
| --- | --- |
| По умолчанию (как было) | `/app0/game` |
| CMake-переменная при конфигурации | `-DLIBERTY_PS4_GAME_ROOT=/data/gta4` |
| Переменная окружения на консоли | `LIBERTY_PS4_GAME_ROOT=/data/gta4` |
| В workflow по умолчанию | `/data/gta4` |

То есть свои настоящие файлы вы кладёте либо в `/app0/game/` при пересборке PKG, либо в любой читаемый каталог (например `/data/gta4/`) и указываете его через `LIBERTY_PS4_GAME_ROOT`. Нужны `default.xex`, `common.rpf`, `xbox360.rpf`, `audio.rpf` из USA Xbox 360 диска + TU8 (точные хэши — в отчёте `/home/user/LibertyRecomp_PS5_PS4Pro_otchet.md`). Сохранения и кэш по-прежнему пишутся в `/user/data/LBTY00001/`, трофеи — в `sce_sys/`.

---

## Как применить

```bash
git clone --depth 1 https://github.com/elripalda/gta4-ps5.git && cd gta4-ps5
git apply /path/to/ps4-pro.patch
git add -A && git commit -m "PS4 Pro (OpenOrbis): Neo mode, 1080p, payload-free build"
git push origin HEAD:ps4-pro
```

Дальше в GitHub: **Actions → Build PS4 Pro (OpenOrbis / Neo / 1080p) → Run workflow**. Параметры `neo_mode`, `skip_game_payload`, `ps4_game_root` можно менять. После успеха скачивайте артефакт `gta4-ps4-pro-eboot` (`eboot.bin` + `sce_sys/param.sfo`).

Локально на Ubuntu 24.04:

```bash
export OO_PS4_TOOLCHAIN="$PWD/tools/OpenOrbis-PS4-Toolchain"
./setup.sh
cmake --preset ps4-pro-release -DLIBERTY_PS4_GAME_ROOT=/data/gta4
cmake --build out/build/ps4-pro-release --target LibertyRecomp LibertyRecomp-sfo -j2
python3 tools/ps4/sfo_neo.py inspect out/build/ps4-pro-release/pkg_staging/sce_sys/param.sfo
# ATTRIBUTE = 0x00800000 (8388608); Neo bit 23 = SET
```

---

## Честно про ограничения

**`eboot.bin` в этой песочнице не собран**: здесь есть только Python, `gcc` и `make`; нет `clang`, `lld`, `ninja` и SDK OpenOrbis. Готовый workflow — это и есть исполняемый эквивалент сборки: он сделает `eboot.bin` на GitHub. Я проверил: патч применяется к чистому клону (`git apply --check`), все CMake-файлы парсятся `cmake-format`, `CMakePresets.json` и YAML валидны, `sfo_neo.py` создаёт/патчит/читает настоящий SFO с битом 23, корневой `CMakeLists.txt` и toolchain проходят конфигурацию CMake до проверки сторонних зависимостей.

Что всё ещё может всплыть при первом реальном билде (это оживимая конфигурация, а не гарантированно зелёный билд):

1. `LibertyRecomp/gpu/video.cpp` — legacy D3D11-подобный путь; `toolchains/ps4-openorbis.cmake` утверждает, что на PS4 используется GNM, но отдельного переключателя `LIBERTY_RECOMP_GNM` в `LibertyRecomp/CMakeLists.txt` нет. Патч убирает блокировку Vulkan, но не переписывает выбор рендерера — возможна работа на этапе линковки/рендера.
2. Новый список исходников `src/core` для PS4: `mapped_memory_posix.cpp` (`shm_open`) и `filesystem_posix.cpp` (`<ftw.h>`, `<pwd.h>`, `<libgen.h>`) могут потребовать Orbis-заглушек. `exception_handler_ps4.cpp` пока не ставит обработчики SIGSEGV/SIGBUS, поэтому гостевые MMIO-fault'ы не восстанавливаются.
3. В релизе OpenOrbis нет `libcxx_ps4_compat.a` (это локально пересобранный libc++ LLVM 21) — toolchain теперь просто пропускает его; заголовки C++23 берутся от host clang и могут потребовать дополнительных `-isystem`.
4. `create-fself` берётся из релиза `v0.5.3`; `create-gp4`/`PkgTool.Core` для PKG — best-effort.
5. `#include <orbis/AudioOut.h>`, `sceKernelIsNeoMode()`, `sceKernelGetDirectMemorySize()` объявлены в `include/orbis/libkernel.h` релиза — проверено.

PS5/RADV-код не тронут: все новые ветки защищены `PS4`, `LIBERTY_RECOMP_TARGET_PLATFORM STREQUAL "ps4"` или `REX_PLATFORM_PS4`.
