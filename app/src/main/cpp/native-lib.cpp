#include <jni.h>
#include <string>

//http://blog.csdn.net/leixiaohua1020/article/details/25422685
extern "C"
{
#include <UserArguments.h>
#include "libavformat/avformat.h"
#include "UserArguments.h"
#include <libavutil/opt.h>
#include <android/log.h>
#include <libyuv.h>
#include "libyuv/rotate.h"
};

#include "threadsafe_queue.cpp"

using namespace libyuv;
threadsafe_queue<uint8_t *> frame_queue;

void android_log(void *ptr, int level, const char *fmt, va_list vl) {
    switch (level) {
        case AV_LOG_VERBOSE:
            __android_log_vprint(ANDROID_LOG_DEBUG, "native-lib", fmt, vl);
            break;
        case AV_LOG_INFO:
            __android_log_vprint(ANDROID_LOG_INFO, "native-lib", fmt, vl);
            break;
        case AV_LOG_WARNING:
            __android_log_vprint(ANDROID_LOG_WARN, "native-lib", fmt, vl);
            break;
        case AV_LOG_ERROR:
            __android_log_vprint(ANDROID_LOG_ERROR, "native-lib", fmt, vl);
            break;
        case AV_LOG_FATAL:
        case AV_LOG_PANIC:
            __android_log_vprint(ANDROID_LOG_FATAL, "native-lib", fmt, vl);
            break;
        case AV_LOG_QUIET:
            __android_log_vprint(ANDROID_LOG_SILENT, "native-lib", fmt, vl);
            break;
        default:
            break;
    }
}

int startSendOneFrame(uint8_t *buf);

struct UserArguments *arguments;
extern "C"
JNIEXPORT void JNICALL
Java_com_example_administrator_mrecord_utils_PicutureNativeUtils_pushData(JNIEnv *env, jclass type,
                                                                          jbyteArray data_) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    startSendOneFrame((uint8_t *) data);
    env->ReleaseByteArrayElements(data_, data, 0);
}

#define  VIDEO_STREAM 0
#define  AUDIO_STREAM 1

int create_avstream(struct UserArguments *arguments, AVFormatContext *ofmt_ctx, int type) {
    int ret = 0;
    AVDictionary *param = 0;

    AVStream *out_stream = avformat_new_stream(ofmt_ctx, 0);
    if (!out_stream) {
        av_log(NULL, AV_LOG_FATAL, "Failed allocating output stream\n");
        ret = AVERROR_UNKNOWN;
    }
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL,
               "Failed to copy context from input to output stream codec context\n");
        return ret;
    }
    out_stream->codecpar->codec_tag = 0;
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    AVCodecContext *pCodecCtx = out_stream->codec;

    if (type == VIDEO_STREAM) {
        ofmt_ctx->video_codec_id = AV_CODEC_ID_H264;
        //pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
        pCodecCtx->codec_id = AV_CODEC_ID_H264;
        pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        pCodecCtx->width = arguments->out_width;
        pCodecCtx->height = arguments->out_height;
        pCodecCtx->bit_rate = arguments->video_bit_rate;
        pCodecCtx->gop_size = arguments->frame_rate*2;
        pCodecCtx->thread_count = 12;
        pCodecCtx->time_base.num = 1;
        pCodecCtx->time_base.den = arguments->frame_rate;
        pCodecCtx->qmin = 10;
        pCodecCtx->qmax = 51;
//        out_stream->time_base = AVRational{1, arguments->frame_rate};
        //Optional Param
        pCodecCtx->max_b_frames = 3;

        //H.264
        if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
//        av_dict_set(&param, "tune", "animation", 0);
//        av_dict_set(&param, "profile", "baseline", 0);
            av_dict_set(&param, "tune", "animation", 0);
            av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);
            av_dict_set(&param, "profile", "baseline", 0);
        }
    } else if (type == AUDIO_STREAM) {
        ofmt_ctx->audio_codec_id = AV_CODEC_ID_AAC;

        pCodecCtx->codec_id = AV_CODEC_ID_AAC;
        pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
        pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
        pCodecCtx->sample_rate = arguments->audio_sample_rate;
        pCodecCtx->channel_layout = AV_CH_LAYOUT_MONO;
        pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
        //todo bit_rate
        pCodecCtx->bit_rate = arguments->audio_bit_rate;
//        pCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    AVCodec *pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) {
        av_log(NULL, AV_LOG_FATAL, "Can not find encoder! ");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open encoder! ");
        return -1;
    }
    av_log(NULL, AV_LOG_FATAL, "编码器打开成功 type：%d", type);
    av_dump_format(ofmt_ctx, 0, ofmt_ctx->filename, 1);

    return 0;
}

bool isEnd = 0;

void *startVideoEncode(void *args) {

    struct UserArguments *arguments = (struct UserArguments *) args;
    AVFormatContext *ofmt_ctx = arguments->pFmt_ctx;
    AVPacket enc_pkt;

    AVFrame *pFrame = av_frame_alloc();
    int picture_size = avpicture_get_size(ofmt_ctx->streams[0]->codec->pix_fmt,
                                          ofmt_ctx->streams[0]->codec->width,
                                          ofmt_ctx->streams[0]->codec->height);
    uint8_t *buf = (uint8_t *) av_malloc(picture_size);
    avpicture_fill((AVPicture *) pFrame, buf, ofmt_ctx->streams[0]->codec->pix_fmt,
                   ofmt_ctx->streams[0]->codec->width,
                   ofmt_ctx->streams[0]->codec->height);
    int out_y_size = ofmt_ctx->streams[0]->codec->width * ofmt_ctx->streams[0]->codec->height;


    int got_packet_ptr = 0;
    avformat_write_header(ofmt_ctx, NULL);
    int frameCount = 0;
    while (!isEnd) {
        // 错误信息：Provided packet is too small, needs to be 1647 解决办法：重新初始化packet
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        uint8_t *ele = *frame_queue.wait_and_pop().get();//yv21
        pFrame->pts = frameCount * (ofmt_ctx->streams[0]->time_base.den) /
                      ((ofmt_ctx->streams[0]->time_base.num) * arguments->frame_rate);
//        pFrame->pts = frameCount;
        av_log(NULL, AV_LOG_FATAL, "pts %lld \n",  pFrame->pts );

        int src_width = arguments->in_height;
        int src_height = arguments->in_width;
        // NV12 video size
        int NV12_Size = src_width * src_height * 3 / 2;
        int NV12_Y_Size = src_width * src_height;

        // YUV420 video size
        int I420_Size = src_width * src_height * 3 / 2;
        int I420_Y_Size = src_width * src_height;
        //加1的目的： 取整   以免画面出现绿边  去跨度一种简单方法
        int I420_U_Size = ((src_width + 1) >> 1) * ((src_height + 1) >> 1);
        int I420_V_Size = I420_U_Size;


//    (unsigned char *) Dst_data = (unsigned char *) malloc(
//            (I420_Size) * sizeof(unsigned char));  //I420

        unsigned char *Y_data_Src = (unsigned char *) ele;
        unsigned char *UV_data_Src = Y_data_Src + NV12_Y_Size;
        int src_stride_y = src_width;
        int src_stride_uv = src_width;

        //dst: buffer address of Y channel U channel and V channel
        unsigned char *Y_data_Dst = pFrame->data[0];
        unsigned char *U_data_Dst = pFrame->data[2];
        unsigned char *V_data_Dst = pFrame->data[1];
        int Dst_Stride_Y;
        int Dst_Stride_U;
        int Dst_Stride_V;

        int rotate = 270;
        if (rotate == 90 || rotate == 270) {
            Dst_Stride_Y = src_height;//480
            Dst_Stride_U = src_height >> 1;
            Dst_Stride_V = Dst_Stride_U;
        } else {
            Dst_Stride_Y = src_width;
            Dst_Stride_U = src_width >> 1;
            Dst_Stride_V = Dst_Stride_U;
        }

        NV12ToI420RotateMirror(Y_data_Src, src_stride_y,
                               UV_data_Src, src_stride_uv,
                               Y_data_Dst, Dst_Stride_Y,
                               U_data_Dst, Dst_Stride_U,
                               V_data_Dst, Dst_Stride_V,
                               src_width, src_height,
                               (libyuv::RotationMode) rotate);
        int ret = avcodec_encode_video2(ofmt_ctx->streams[0]->codec, &enc_pkt, pFrame,
                                        &got_packet_ptr);
        if (ret < 0) {
            av_log(NULL, AV_LOG_FATAL, "Failed to encode! ret:%d\n", ret);
            continue;
        }
        if (got_packet_ptr >= 0) {
            enc_pkt.stream_index = ofmt_ctx->streams[0]->index;
            av_log(NULL, AV_LOG_FATAL, "encode succetss  ! \n");
//            pkt.pts=pkt.dts=frameCount;
            frameCount++;
            av_log(NULL, AV_LOG_FATAL, "frameCount %d \n",frameCount);

        } else{
            av_log(NULL, AV_LOG_FATAL, "encode failed  ! \n");
        }
        ret = av_write_frame(ofmt_ctx, &enc_pkt);
        if (frameCount > 150) {
            break;
        }
    }
    //Write file trailer
    av_write_trailer(ofmt_ctx);
    av_log(NULL, AV_LOG_FATAL, " 编码完毕 \n");
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_administrator_mrecord_LiveEngine_prepareRecord(JNIEnv *env, jobject instance,
                                                                jstring videoPath_) {
    const char *videoPath = env->GetStringUTFChars(videoPath_, 0);
    __android_log_print(ANDROID_LOG_DEBUG, "native-lib", "videoPath :%s", videoPath);
    arguments = (struct UserArguments *) malloc(sizeof(UserArguments));
    int video_bit_rate = 400000;
    int frame_rate = 25;
    int in_width = 720;
    int in_height = 1280;
//    int in_width = 480;
//    int in_height = 800;
    int out_height = in_height;
    int out_width = in_width;
    arguments->video_bit_rate = video_bit_rate;
    arguments->frame_rate = frame_rate;
    arguments->audio_bit_rate = 40000;
    arguments->audio_sample_rate = 44100;
    arguments->in_width = in_width;
    arguments->in_height = in_height;
    arguments->out_height = out_height;
    arguments->out_width = out_width;


    av_register_all();
    av_log_set_callback(android_log);
    AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
    if (codec == NULL) {
        __android_log_print(ANDROID_LOG_DEBUG, "native-lib", ": %s", "打开libfdk_aac 失败");
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, "native-lib", ": %s", "打开libfdk_aac  成功");

    }
    AVCodec *codec2 = avcodec_find_encoder_by_name("libx264");
    if (codec2 == NULL) {
        __android_log_print(ANDROID_LOG_DEBUG, "native-lib", ": %s", "打开libx264 失败");
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, "native-lib", ": %s", "打开libx264 成功");
    }
    AVOutputFormat *ofmt = NULL;
    //输入对应一个AVFormatContext，输出对应一个AVFormatContext
    //（Input AVFormatContext and Output AVFormatContext）
    AVFormatContext *ofmt_ctx = NULL;
    const char *out_filename;
    int ret, i;
    out_filename = videoPath;//输出文件名（Output file URL）
    av_log(NULL, AV_LOG_FATAL, "out_filename: %s", out_filename);
    av_register_all();
    //输出（Output）
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        av_log(NULL, AV_LOG_FATAL, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
//        goto end;
    }
    ofmt = ofmt_ctx->oformat;
    //Open output URL
    if (avio_open(&ofmt_ctx->pb, videoPath, AVIO_FLAG_READ_WRITE) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open output file! \n");
        return;
    }

    create_avstream(arguments, ofmt_ctx, VIDEO_STREAM);
    create_avstream(arguments, ofmt_ctx, AUDIO_STREAM);


    arguments->pFmt_ctx = ofmt_ctx;

    //输出一下格式------------------

    pthread_t thread;
    pthread_create(&thread, NULL, startVideoEncode, arguments);

    //打开输出文件（Open output file）
//    if (!(ofmt->flags & AVFMT_NOFILE)) {
//        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
//        if (ret < 0) {
//            printf( "Could not open output file '%s'", out_filename);
//            goto end;
//        }
//    }
//    //写文件头（Write file header）
//    ret = avformat_write_header(ofmt_ctx, NULL);
//    if (ret < 0) {
//        printf( "Error occurred when opening output file\n");
//        goto end;
//    }
    int frame_index = 0;
//    while (1) {
//        /* copy packet */
//        //转换PTS/DTS（Convert PTS/DTS）
//        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
//        pkt.pos = -1;
//        //写入（Write）
//        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
//        if (ret < 0) {
//            printf( "Error muxing packet\n");
//            break;
//        }
//        printf("Write %8d frames to output file\n",frame_index);
//        av_free_packet(&pkt);
//        frame_index++;
//    }
//    //写文件尾（Write file trailer）
//    av_write_trailer(ofmt_ctx);
//    end:
//    /* close output */
//    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
//        avio_close(ofmt_ctx->pb);
//    avformat_free_context(ofmt_ctx);
//    if (ret < 0 && ret != AVERROR_EOF) {
//        av_log(NULL, AV_LOG_FATAL, "Error occurred.\n");
//    }

    env->ReleaseStringUTFChars(videoPath_, videoPath);
}


/**
 * 发送一帧到编码队列
 * @param buf
 * @return
 */
int startSendOneFrame(uint8_t *buf) {
    int in_y_size = arguments->in_width * arguments->in_height;

    uint8_t *new_buf = (uint8_t *) malloc(in_y_size * 3 / 2);
    memcpy(new_buf, buf, in_y_size * 3 / 2);
    frame_queue.push(new_buf);
    return 0;
}