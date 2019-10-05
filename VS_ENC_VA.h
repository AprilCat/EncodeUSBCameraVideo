//
// Created by nephilim on 19-10-3.
//

#ifndef VAAPI_TEST_VS_ENC_VA_H
#define VAAPI_TEST_VS_ENC_VA_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>

#ifdef __cplusplus
}
#endif

#include <string>

class VS_ENC_VA {
public:
    AVFrame* sw_frame;
    AVFrame* hw_frame;
    AVCodecContext* avctx;
    AVCodec* codec;
    int width, height, size;
    AVBufferRef* hw_device_ctx;

public:
    VS_ENC_VA();
    ~VS_ENC_VA();
    int initEncParams();
    void setEncParams(int width, int height);
    void startThread(std::string input, std::string output);
    void stopThread();
};


#endif //VAAPI_TEST_VS_ENC_VA_H
