cmake_minimum_required(VERSION 3.14)
project(vaapi_test C)

set(CMAKE_C_STANDARD 11)

link_libraries(
        opencv
        pthread
        dl
        opencv_video
        opencv_highgui
        opencv_core
        opencv_imgproc
        opencv_videoio
        avdevice
        avformat
        avfilter
        avcodec
        swresample
        swscale
        rt
        m
        avutil
)

file(GLOB sources "./*.c" "./*.h")

add_executable(vaapi_test ${sources})