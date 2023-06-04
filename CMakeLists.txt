cmake_minimum_required(VERSION 3.21)

project(H264Decoder)
set(TARGET H264Decoder)
set(CMAKE_CXX_STANDARD 17)
set(ROOT_PATH ${CMAKE_SOURCE_DIR})

add_definitions(-DRESOURCE_DIR="${ROOT_PATH}")

set(HDR_DIR include)
set(BASE_HDR "")

set(BASE_SRC
    src/FileReader.cpp
    src/BitReader.cpp
    src/FlvDemuxer.cpp
    src/AvcDecoder.cpp
    src/main.cpp
)


add_executable(${TARGET} ${BASE_HDR} ${BASE_SRC})
target_include_directories(${TARGET} PUBLIC ${HDR_DIR})
