CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(OUTPUT_ELF "tc_video_decoder")

SET(SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/video_clock_control.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/video_index_control.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/video_decoder.c
	)

INCLUDE_DIRECTORIES(
	${GLIB2_INCLUDE_DIRS}
	${TUTIL_INCLUDE_DIRS}
	${TUTIL_INC_DIR}
	${CMAKE_SOURCE_DIR}/decoder/video/include
	${CMAKE_SOURCE_DIR}/decoder/resources/wrapper/include
	)

ADD_COMPILE_OPTIONS(-g -Wall)

LINK_LIBRARIES(tc_video_decoder_resource ${GLIB2_LIBRARIES} ${TUTIL_LIBRARIES})
LINK_DIRECTORIES(${TUTIL_LIB_DIR})

ADD_LIBRARY(${OUTPUT_ELF} SHARED ${SRC_FILES})

SET(VIDEO_DECODER_MAJOR_VERSION "1")
SET(VIDEO_DECODER_MINOR_VERSION "0")
SET(VIDEO_DECODER_PATCH_VERSION "0")

SET_TARGET_PROPERTIES(${OUTPUT_ELF}
	PROPERTIES
	VERSION "${VIDEO_DECODER_MAJOR_VERSION}.${VIDEO_DECODER_MINOR_VERSION}.${VIDEO_DECODER_PATCH_VERSION}"
	SOVERSION "${VIDEO_DECODER_MAJOR_VERSION}")

CONFIGURE_FILE(
		${CMAKE_CURRENT_SOURCE_DIR}/include/video_decoder_version.h.in
		${CMAKE_CURRENT_SOURCE_DIR}/include/video_decoder_version.h)

TARGET_INCLUDE_DIRECTORIES(${OUTPUT_ELF} PUBLIC ${TUTIL_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${OUTPUT_ELF} ${LIB} ${TUTIL_LIBRARIES} ${GLIB2_LIBRARIES} tc_video_decoder_resource)

INSTALL(TARGETS ${OUTPUT_ELF}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)

INSTALL(FILES
			${CMAKE_CURRENT_SOURCE_DIR}/include/video_decoder.h
		DESTINATION
			${CMAKE_INSTALL_INCLUDEDIR})