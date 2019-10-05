//
// Created by nephilim on 19-10-3.
//

#include "VS_ENC_VA.h"
#include <string>
#include <iostream>
#include <pthread.h>
std::string encName("h264_vaapi");
pthread_t handleEnc;
int isRunEnc;
std::string outputFileName;
std::string inputFileName;
static int encode_write(AVCodecContext *avctx, AVFrame *frame, FILE *fout)
{
    int ret = 0;
    AVPacket enc_pkt;

    av_init_packet(&enc_pkt);
    enc_pkt.data = NULL;
    enc_pkt.size = 0;

    if ((ret = avcodec_send_frame(avctx, frame)) < 0) {
//        fprintf(stderr, "Error code: %s\n", av_err2str(ret));
        fprintf(stderr, "Error code: %d\n", ret);
        goto end;
    }
    while (1) {
        ret = avcodec_receive_packet(avctx, &enc_pkt);
        if (ret)
            break;

        enc_pkt.stream_index = 0;
        ret = fwrite(enc_pkt.data, enc_pkt.size, 1, fout);
        av_packet_unref(&enc_pkt);
    }

    end:
    ret = ((ret == AVERROR(EAGAIN)) ? 0 : -1);
    return ret;
}
static void* threadEnc(void* pp){
    VS_ENC_VA* pa = (VS_ENC_VA*)pp;
    int err = 0;
    FILE* fin = fopen(inputFileName.c_str(), "r");
    if(!fin){
        std::cout<<"failed to open input file"<<std::endl;
        isRunEnc = 0;
        return nullptr;
    }
    FILE* fout = fopen(outputFileName.c_str(), "w+b");
    if(!fout){
        std::cout<<"failed to open output file"<<std::endl;
        isRunEnc = 0;
        return nullptr;
    }

    while(isRunEnc){
        if (!(pa->sw_frame = av_frame_alloc())) {
            err = AVERROR(ENOMEM);
            break;
        }
        /* read data into software frame, and transfer them into hw frame */
        pa->sw_frame->width  = pa->width;
        pa->sw_frame->height = pa->height;
        pa->sw_frame->format = AV_PIX_FMT_NV12;
        if ((err = av_frame_get_buffer(pa->sw_frame, 32)) < 0)
            break;
        if ((err = fread((uint8_t*)(pa->sw_frame->data[0]), pa->size, 1, fin)) <= 0)
            break;
        if ((err = fread((uint8_t*)(pa->sw_frame->data[1]), pa->size/2, 1, fin)) <= 0)
            break;

        if (!(pa->hw_frame = av_frame_alloc())) {
            err = AVERROR(ENOMEM);
            break;
        }
        if ((err = av_hwframe_get_buffer(pa->avctx->hw_frames_ctx, pa->hw_frame, 0)) < 0) {
            //fprintf(stderr, "Error code: %s.\n", av_err2str(err));
            fprintf(stderr, "Error code: %d.\n", err);
            break;
        }
        if (!pa->hw_frame->hw_frames_ctx) {
            err = AVERROR(ENOMEM);
            break;
        }
        if ((err = av_hwframe_transfer_data(pa->hw_frame, pa->sw_frame, 0)) < 0) {
            //fprintf(stderr, "Error while transferring frame data to surface."
            //                "Error code: %s.\n", av_err2str(err));
            fprintf(stderr, "Error while transferring frame data to surface.");
            break;
        }

        if ((err = (encode_write(pa->avctx, pa->hw_frame, fout))) < 0) {
            fprintf(stderr, "Failed to encode.\n");
            break;
        }
        av_frame_free(&pa->hw_frame);
        av_frame_free(&pa->sw_frame);
        err = encode_write(pa->avctx, NULL, fout);
        if (err == AVERROR_EOF)
            err = 0;
    }
    isRunEnc = 0;
}

static int set_hwframe_ctx(AVCodecContext *ctx, AVBufferRef *hw_device_ctx, int width, int height)
{
    AVBufferRef *hw_frames_ref;
    AVHWFramesContext *frames_ctx = NULL;
    int err = 0;

    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
        fprintf(stderr, "Failed to create VAAPI frame context.\n");
        return -1;
    }
    frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
    frames_ctx->format    = AV_PIX_FMT_VAAPI;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width     = width;
    frames_ctx->height    = height;
    frames_ctx->initial_pool_size = 20;
    if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
        fprintf(stderr, "Failed to initialize VAAPI frame context.:%d\r\n",err);
        av_buffer_unref(&hw_frames_ref);
        return err;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!ctx->hw_frames_ctx)
        err = AVERROR(ENOMEM);

    av_buffer_unref(&hw_frames_ref);
    return err;
}

VS_ENC_VA::VS_ENC_VA() {
    sw_frame = nullptr;
    hw_frame = nullptr;
    avctx = nullptr;
    codec = nullptr;
    width = 0;
    height = 0;
    hw_device_ctx = nullptr;
    isRunEnc = 0;
}

VS_ENC_VA::~VS_ENC_VA() {

}

void VS_ENC_VA::setEncParams(int width, int height){
    this->height = height;
    this->width = width;
    this->size = height * width;
}

int VS_ENC_VA::initEncParams() {
    int err = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI, nullptr, nullptr, 0);
    if(err < 0){
        std::cout<<"Failed to create a VAAPI device. Error code:"<<(err)<<std::endl;
        return err;
    }
    if(!(codec = avcodec_find_encoder_by_name(encName.c_str()))){
        return -1;
    }
    if(!(avcodec_alloc_context3(codec))){
        err = AVERROR(ENOMEM);
        return err;
    }
    avctx->width = width;
    avctx->height = height;
    avctx->time_base = (AVRational){1, 25};
    avctx->framerate = (AVRational){25, 1};
    avctx->sample_aspect_ratio = (AVRational){1, 1};
    avctx->pix_fmt = AV_PIX_FMT_VAAPI;
    if((err = set_hwframe_ctx(avctx, hw_device_ctx, width, height)) < 0){
        std::cout<<"failed to set hwframe cotext"<<std::endl;
        return err;
    }
    if ((err = avcodec_open2(avctx, codec, NULL)) < 0) {
        fprintf(stderr, "Cannot open video encoder codec. Error code: %d\n", (err));
        return err;
    }
}

void VS_ENC_VA::startThread(std::string input, std::string output) {
    if(isRunEnc == 0){
        isRunEnc = 1;
        inputFileName = input;
        outputFileName = output;
        pthread_create(&handleEnc, nullptr, threadEnc, (void*)this);
    }
}

void VS_ENC_VA::stopThread() {
    if(isRunEnc){
        isRunEnc = 0;
        pthread_join(handleEnc, nullptr);
        av_frame_free(&sw_frame);
        av_frame_free(&hw_frame);
        avcodec_free_context(&avctx);
        av_buffer_unref(&hw_device_ctx);
    }

}
