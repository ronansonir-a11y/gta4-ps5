# [ps4 pro] PS4 / OpenOrbis has no Vulkan ICD: the legacy tree renders through
# the GNM-backed path in LibertyRecomp/gpu/video.cpp. The PS5/RADV build keeps
# the original unconditional Vulkan=ON behaviour below.
if(PS4 OR LIBERTY_RECOMP_TARGET_PLATFORM STREQUAL "ps4" OR CMAKE_SYSTEM_NAME STREQUAL "PS4")
    set(REXGLUE_USE_VULKAN OFF CACHE BOOL "" FORCE)
    set(REXGLUE_USE_D3D12 OFF CACHE BOOL "" FORCE)
    set(REXGLUE_ENABLE_FIDELITYFX OFF CACHE BOOL "" FORCE)
    set(LIBERTY_RECOMP_GNM ON CACHE BOOL "Use GNM-native renderer on PS4" FORCE)
    set(REXGLUE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
else()
    set(REXGLUE_USE_VULKAN ON CACHE BOOL "" FORCE)
    set(REXGLUE_USE_D3D12 OFF CACHE BOOL "" FORCE)
    set(REXGLUE_ENABLE_FIDELITYFX OFF CACHE BOOL "Enable optional FidelityFX temporal providers")
    set(REXGLUE_BUILD_TESTS OFF CACHE BOOL "")
endif()

set(REXSDK_DIR "${CMAKE_SOURCE_DIR}/glue/rexglue-sdk-main"
    CACHE PATH "Path to the Graine RexGlue SDK source tree")

if(NOT EXISTS "${REXSDK_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Graine RexGlue SDK not found at ${REXSDK_DIR}")
endif()

add_subdirectory("${REXSDK_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/rexglue-sdk")

# The macOS consumer must package the workspace MoltenVK build.  The generic
# RexGlue staging code still supplies the Vulkan loader and SDK support files,
# but an installed MoltenVK may not contain the fixes carried by this tree.
if(APPLE AND NOT TARGET MoltenVK)
    add_subdirectory(
        "${CMAKE_SOURCE_DIR}/thirdparty/MoltenVK"
        "${CMAKE_BINARY_DIR}/thirdparty/MoltenVK"
    )
endif()

add_subdirectory("${REXSDK_DIR}/gta4-recomp" "${CMAKE_CURRENT_BINARY_DIR}/gta4-recomp")
