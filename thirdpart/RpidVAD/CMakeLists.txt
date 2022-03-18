cmake_minimum_required(VERSION 3.12)
project(rpvad)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)

add_definitions(-D_EXPORT_rpvad_API  -DWEBRTC_WIN)

else()

add_definitions(-DWEBRTC_POSIX)
endif()


aux_source_directory(sources MAIN_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/webrtc/rtc_base/*.cc WEBRTC_SRC)
FILE (GLOB ALL_SOURCES "webrtc/common_audio/vad/*.c"  "webrtc/common_audio/signal_processing/*.c" "webrtc/common_audio/rtc_base/*.cc")
FILE (GLOB ALL_INCLUDES "include/*.h" "webrtc/*.h" )
#FILE (GLOB ALL_INCLUDES "*.hpp" "*.h" "./AFolder/*.hpp"  "./AFolder/*.h" )



SET (ALL_SRCS  ${ALL_SOURCES})


list(REMOVE_ITEM MAIN_SRC "sources/test.cpp")

message( ${MAIN_SRC})

#set(MAIN_SRC sources/librpvad.cpp)
SET(WEBRTC_SRC webrtc/rtc_base/checks.cc)

add_library(rpvad SHARED ${MAIN_SRC}  ${WEBRTC_SRC}  ${ALL_SRCS} ${WEBRTC_SRC})

if( NOT WIN32)
target_link_libraries(rpvad PUBLIC pthread)
endif()




add_executable(rpvad_test sources/test.cpp)

target_link_libraries(rpvad_test PUBLIC rpvad)