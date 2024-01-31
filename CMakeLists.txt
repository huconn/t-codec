# Required minimum cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(T-CODEC)

#include path
if(NOT CMAKE_INSTALL_PREFIX)
	set(CMAKE_INSTALL_PREFIX "usr")
endif(NOT CMAKE_INSTALL_PREFIX)

if(NOT CMAKE_INSTALL_BINDIR)
	set(CMAKE_INSTALL_BINDIR "bin")
endif(NOT CMAKE_INSTALL_BINDIR)

if(NOT CMAKE_INSTALL_LIBDIR)
	set(CMAKE_INSTALL_LIBDIR "lib")
endif(NOT CMAKE_INSTALL_LIBDIR)

if(NOT CMAKE_INSTALL_INCLUDEDIR)
	set(CMAKE_INSTALL_INCLUDEDIR "include")
endif(NOT CMAKE_INSTALL_INCLUDEDIR)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GLIB2 glib-2.0)
PKG_CHECK_MODULES(TUTIL T-UTIL)

string(LENGTH "${TUTIL_LIBRARIES}" TU_LEN)

set(TUTIL_INC_DIR "${TUTIL_DIR}/build/install/include/")
set(TUTIL_LIB_DIR "${TUTIL_DIR}/build/install/lib/")

###### stack canary
ADD_COMPILE_OPTIONS(-fstack-protector-all)

###### Include file to compile
INCLUDE (${CMAKE_SOURCE_DIR}/Toolchain-common.cmake)
INCLUDE (${CMAKE_SOURCE_DIR}/compile_definition.cmake)

if(!TU_LEN)
	message(STATUS "TUTIL_LIBRARIES is Working")
else()
	if(USE_T_TEST)
		set(TUTIL_LIBRARIES "-ltu_jp_factory -ltu_calc -ltu_csv_report")
	else()
		set(TUTIL_LIBRARIES "-ltu_jp_factory -ltu_calc")
	endif()
endif()

message(STATUS "USE_VPU_LEGACY: '${USE_VPU_LEGACY}'")
message(STATUS "USE_T_TEST: '${USE_T_TEST}'")
message(STATUS "USE_VPU_V3: '${USE_VPU_V3}'")

if(GET_CSV_REPORT)
	add_definitions(-DGET_CSV_REPORT_INCLUDE)
endif()

if(USE_VPU_LEGACY)
add_definitions(-DTCC_VPU_LEGACY)
endif()

if(USE_VPU_V3)
add_definitions(-DTCC_VPU_V3)
endif()

###### Include sub directory to compile
if(USE_DECODER)
add_subdirectory(decoder)
set(PC_LIB_DECODER "-ltc_video_decoder -ltc_audio_decoder")
endif()
if(USE_ENCODER)
add_subdirectory(encoder)
set(PC_LIB_ENCODER "-ltc_video_encoder")
endif()

if(USE_T_TEST)
if(USE_ENCODER)
	add_definitions(-DENCODER_TEST_INCLUDE)
endif()

if(USE_DECODER)
	add_definitions(-DDECODER_TEST_INCLUDE)
endif()

add_subdirectory(tester)
endif()

if(USE_VPU_V3)
add_subdirectory(vpu_test)
endif()

###### Install pkgconfig
configure_file(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)