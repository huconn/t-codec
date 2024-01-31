CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

SET(OUTPUT_ELF "vpu_script_decoder_wrapper")

option(ENABLE_VDEC_INTERFACE "Enable vdec interface" ON)

if (ENABLE_VDEC_INTERFACE)
    add_definitions(-DENABLE_VDEC_INTERFACE)
endif()

if (ENABLE_VDEC_INTERFACE)
	SET(SRC_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/vpu_script_decoder_wrapper_vdec.c
		)

	INCLUDE_DIRECTORIES(
		${CMAKE_CURRENT_SOURCE_DIR}/includes
		${CMAKE_SOURCE_DIR}/decoder/resources/vdec_v3
		${CMAKE_SOURCE_DIR}/encoder/resources/venc_v3
		)

	LINK_LIBRARIES(venc_v3_interface vdec_v3_interface)
else()
	SET(SRC_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/vpu_script_decoder_wrapper_tcodec.c
		)

	INCLUDE_DIRECTORIES(
		${CMAKE_CURRENT_SOURCE_DIR}/includes
		${CMAKE_SOURCE_DIR}/encoder/video/include
		${CMAKE_SOURCE_DIR}/decoder/video/include
		)

	LINK_LIBRARIES(tc_video_encoder tc_video_decoder tu_jp_factory tu_calc)
	LINK_DIRECTORIES(${TUTIL_LIB_DIR})
	message(STATUS "TUTIL_LIBRARIES: '${TUTIL_LIBRARIES}'")
endif()


ADD_COMPILE_OPTIONS(-g -w)

ADD_LIBRARY(${OUTPUT_ELF} SHARED ${SRC_FILES})

INSTALL(
	TARGETS ${OUTPUT_ELF}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
