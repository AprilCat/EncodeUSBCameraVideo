cmake_minimum_required(VERSION 3.14)
project(vaapi_test)

set(CMAKE_CXX_STANDARD 11)

link_libraries(
        opencv
        pthread
        dl
        opencv_video
        opencv_highgui
        opencv_core
        opencv_imgproc
        opencv_videoio
        avformat
        avcodec
        avdevice
        avutil
        rt
        avcodec
        m
        swscale
        swresample
)

file(GLOB sources "./*.cpp" "./*.h")

add_executable(vaapi_test ${sources})