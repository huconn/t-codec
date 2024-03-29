CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(OUTPUT_ELF "tc_video_decoder_resource")

SET(SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/video_decode_resource.c
	)

INCLUDE_DIRECTORIES(
	${GLIB2_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/decoder/video/include
	${CMAKE_SOURCE_DIR}/decoder/resources/plugin/include
	${CMAKE_SOURCE_DIR}/decoder/resources/plugin/interface/include
	${LINUX_KERNEL_DIR}/include/video/telechips
	${CMAKE_SOURCE_DIR}/decoder/resources/vdec_v3
	)

ADD_COMPILE_OPTIONS(-g -Wall)

if(USE_VPU_LEGACY)
set(OPTIONAL_LIBRARIES tc_decoderplugininterface)
endif()

if(USE_VPU_V3)
set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} vdec_v3_interface)
endif()

LINK_LIBRARIES(${GLIB2_LIBRARIES} ${OPTIONAL_LIBRARIES})

ADD_LIBRARY(${OUTPUT_ELF} SHARED ${SRC_FILES})

SET(VIDEO_DECODER_RESOURCE_MAJOR_VERSION "1")
SET(VIDEO_DECODER_RESOURCE_MINOR_VERSION "0")
SET(VIDEO_DECODER_RESOURCE_PATCH_VERSION "0")

SET_TARGET_PROPERTIES(${OUTPUT_ELF}
	PROPERTIES
	VERSION "${VIDEO_DECODER_RESOURCE_MAJOR_VERSION}.${VIDEO_DECODER_RESOURCE_MINOR_VERSION}.${VIDEO_DECODER_RESOURCE_PATCH_VERSION}"
	SOVERSION "${VIDEO_DECODER_RESOURCE_MAJOR_VERSION}")

CONFIGURE_FILE(
		${CMAKE_CURRENT_SOURCE_DIR}/include/video_decode_resource_version.h.in
		${CMAKE_CURRENT_SOURCE_DIR}/include/video_decode_resource_version.h)

TARGET_INCLUDE_DIRECTORIES(${OUTPUT_ELF} PUBLIC ${GLIB2_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${OUTPUT_ELF} ${LIB} ${GLIB2_LIBRARIES} ${OPTIONAL_LIBRARIES})

INSTALL(TARGETS ${OUTPUT_ELF}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
