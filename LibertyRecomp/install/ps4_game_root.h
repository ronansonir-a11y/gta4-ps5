#pragma once
// [ps4 pro] Where the PS4 build looks for the Xbox 360 game files at runtime.
//
// Historically the PKG shipped them at /app0/game (read-only). To build a
// small eboot.bin without uploading 7 GB of assets, the real files can be
// placed anywhere the homebrew process can read, e.g. /data/gta4/ on a
// jailbroken console, and selected with the LIBERTY_PS4_GAME_ROOT environment
// variable or by setting the CMake cache variable of the same name.

#include <filesystem>
#include <cstdlib>

#if defined(__ORBIS__)

#ifndef LIBERTY_PS4_GAME_ROOT_DEFAULT
#define LIBERTY_PS4_GAME_ROOT_DEFAULT "/app0/game"
#endif

namespace liberty::ps4 {
inline std::filesystem::path GameRoot() {
    if (const char* env = std::getenv("LIBERTY_PS4_GAME_ROOT")) {
        if (env[0] != '\0') return std::filesystem::path(env);
    }
    return std::filesystem::path(LIBERTY_PS4_GAME_ROOT_DEFAULT);
}
}  // namespace liberty::ps4
#endif  // __ORBIS__
