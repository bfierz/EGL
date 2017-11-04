cmake_minimum_required(VERSION 3.0)
project(EGL)

# Download and compile GLEW
set(GLEW_PAKET_NAME glew-2.1.0)
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/glew/${GLEW_PAKET_NAME}.zip)
	message(STATUS "Downloading ${GLEW_PAKET_NAME}")
	file(DOWNLOAD
		https://github.com/nigels-com/glew/releases/download/${GLEW_PAKET_NAME}/${GLEW_PAKET_NAME}.zip
		${CMAKE_CURRENT_BINARY_DIR}/glew/${GLEW_PAKET_NAME}.zip
		STATUS GLEW_DOWNLOAD_STATUS
	)
	list(GET GLEW_DOWNLOAD_STATUS 0 GLEW_DOWNLOAD_STATUS_CODE)
	list(GET GLEW_DOWNLOAD_STATUS 1 GLEW_DOWNLOAD_STATUS_STRING)
endif()

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/glew/${GLEW_PAKET_NAME}.zip AND NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/glew/${GLEW_PAKET_NAME})
	message(STATUS "Unpacking ${GLEW_PAKET_NAME}")
	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xfz ${CMAKE_CURRENT_BINARY_DIR}/glew/${GLEW_PAKET_NAME}.zip
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/glew/
		RESULT_VARIABLE GLEW_UNZIP_STATUS_CODE
	)
endif()

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/glew/${GLEW_PAKET_NAME})
	add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/glew/${GLEW_PAKET_NAME}/build/cmake ${CMAKE_CURRENT_BINARY_DIR}/glew/build)
endif()

set(SOURCE_FILES 
	include/EGL/egl.h
	include/EGL/eglext.h
	include/EGL/eglplatform.h
	src/egl.c
	src/egl_common.c
	src/egl_internal.h)

if(WIN32)
	set(PLATFORM_SOURCE_FILES src/egl_windows.c)
else()
	set(PLATFORM_SOURCE_FILES src/egl_x11.c)
endif()

add_library(egl SHARED "")

target_include_directories(egl
	PUBLIC
		include
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/glew/${GLEW_PAKET_NAME}/include
)
target_compile_definitions(egl PRIVATE KHRONOS_API_EXPORT GLEW_STATIC)
target_sources(egl 
	PRIVATE
		${SOURCE_FILES} 
		${PLATFORM_SOURCE_FILES}
)
target_link_libraries(egl
	PRIVATE
		glew_s
)

option(EGL_BUILD_EXAMPLE OFF)
if (EGL_BUILD_EXAMPLE)
	add_executable(egl_ctx_sample
		example/main.cpp
	)
	target_include_directories(egl_ctx_sample
		PRIVATE
			${CMAKE_CURRENT_BINARY_DIR}/glew/${GLEW_PAKET_NAME}/include
	)
	target_link_libraries(egl_ctx_sample
		PRIVATE
			egl
			glew_s
			${OPENGL_gl_LIBRARY}
	)
endif()