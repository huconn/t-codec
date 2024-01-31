CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

add_subdirectory(vpu_script_decoder_wrapper)

SET(OUTPUT_ELF "vpu_test")

SET(SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/vpu_test_common.c
	${CMAKE_CURRENT_SOURCE_DIR}/vpu_test_decoder.c
	${CMAKE_CURRENT_SOURCE_DIR}/vpu_test_encoder.c
	${CMAKE_CURRENT_SOURCE_DIR}/vpu_script_common.c
	${CMAKE_CURRENT_SOURCE_DIR}/vpu_script_system.c
	${CMAKE_CURRENT_SOURCE_DIR}/vpu_script_process.c
	${CMAKE_CURRENT_SOURCE_DIR}/vpu_script.c
	${CMAKE_CURRENT_SOURCE_DIR}/vpu_script_test.c
	${CMAKE_CURRENT_SOURCE_DIR}/vpu_test_main.c
	)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/vpu_script_decoder_wrapper
	${CMAKE_SOURCE_DIR}/encoder/video/include
	${CMAKE_SOURCE_DIR}/decoder/video/include
	${CMAKE_SOURCE_DIR}/decoder/resources/vdec_v3
	${CMAKE_SOURCE_DIR}/encoder/resources/venc_v3
	)

ADD_COMPILE_OPTIONS(-g -w)

message(STATUS "TUTIL_LIBRARIES: '${TUTIL_LIBRARIES}'")

LINK_LIBRARIES(tc_video_encoder tc_video_decoder tu_jp_factory tu_calc vpu_script_decoder_wrapper)
LINK_DIRECTORIES(${TUTIL_LIB_DIR})

ADD_EXECUTABLE(${OUTPUT_ELF} ${SRC_FILES})

INSTALL(
	TARGETS ${OUTPUT_ELF}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
