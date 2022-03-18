cmake_minimum_required(VERSION 3.12)
project(rpavkits)
set(LIBNAME  rpavkits)
set(EXENAME   rpavkits_test)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)

	add_definitions(-D_QMAVKIT_EXPORT)
	set(ROOT_DIR ${FFMPEG_SDK_ROOT})
	message(${ROOT_DIR})
else()

	if( NOT DEFINED  ROOT_DIR)
	set(ROOT_DIR /opt/qmcds/)
	endif()


endif()

include_directories(${ROOT_DIR}/include)
link_directories(${ROOT_DIR}/lib)

set(APISRC  sources/rpavkitsapi.cpp sources/rpavkits.cpp sources/ffmpeg_mem.cpp sources/qmwav_fmt.cpp)
add_library(${LIBNAME} SHARED  ${APISRC} "sources/qmresample.cpp")

target_link_libraries(${LIBNAME} PUBLIC avformat swresample avcodec avutil qmvad) 

add_executable(${EXENAME} sources/test.cpp )
target_link_libraries(${EXENAME} PRIVATE ${LIBNAME} avformat swresample)