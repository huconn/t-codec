CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

if (USE_VPU_LEGACY)
add_subdirectory(plugin)
endif()

if (USE_VPU_V3)
add_subdirectory(vdec_v3)
endif()

add_subdirectory(wrapper)