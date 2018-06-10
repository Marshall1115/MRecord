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
threadsafe_queue<uint8_t *> video_frame_queue;
threadsafe_queue<uint8_t *> audio_frame_queue;

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

int startSendOneVideoFrame(uint8_t *buf);

struct UserArguments *arguments;
extern "C"
JNIEXPORT void JNICALL
Java_com_example_administrator_mrecord_utils_PicutureNativeUtils_pushData(JNIEnv *env, jclass type,
                                                                          jbyteArray data_) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    startSendOneVideoFrame((uint8_t *) data);
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
//    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
//        out_stream->codec->flags |= AVFMT_GLOBALHEADER  ;
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

//    AVCodec * codec = avcodec_find_encoder( out_stream->codec->codec_id);
//    AVCodecContext *pCodecCtx =  avcodec_alloc_context3(codec);
//    if (!out_stream->codec->codec) {
//        av_log(NULL, AV_LOG_ERROR, "Error allocating the encoding context.\n");
//        return 0;
//    }
    // input_streams[source_index]->discard = 0;

    AVCodecContext *pCodecCtx = out_stream->codec;

    if (type == VIDEO_STREAM) {
//        ofmt_ctx->video_codec_id = AV_CODEC_ID_H264;
//        pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
        pCodecCtx->codec_id = AV_CODEC_ID_H264;
        pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        pCodecCtx->width = arguments->out_width;
        pCodecCtx->height = arguments->out_height;
        av_log(NULL, AV_LOG_FATAL, "arguments->video_bit_rate %d\n", arguments->video_bit_rate);

        pCodecCtx->bit_rate = arguments->video_bit_rate;
        pCodecCtx->thread_count = 12;
        pCodecCtx->time_base.num = 1;
        pCodecCtx->time_base.den = arguments->frame_rate;
        pCodecCtx->gop_size = arguments->frame_rate ;
        pCodecCtx->qmin = 10;
        pCodecCtx->qmax = 51;
//        out_stream->time_base = AVRational{1, arguments->frame_rate};
        //Optional Param
        pCodecCtx->max_b_frames = 1;//0没有编码延迟


//        pCodecCtx->me_range = 0;
//        pCodecCtx->max_qdiff = 3;
        //todo 学习如下参数
//        pCodecCtx->gop_size = 12;
//        pCodecCtx->me_range = 16;
//        pCodecCtx->max_qdiff = 4;
//        pCodecCtx->qmin = 10;
//        pCodecCtx->qmax = 20;
//        pCodecCtx->qcompress = 0.6;

        pCodecCtx->gop_size = 30;
        pCodecCtx->max_b_frames = 0;
        /* Set relevant parameters of H264 */
        pCodecCtx->qmin = 10;   //default 2
        pCodecCtx->qmax = 31;   //default 31
        pCodecCtx->max_qdiff = 4;
        pCodecCtx->me_range = 16;   //default 0
        pCodecCtx->max_qdiff = 4;   //default 3
        pCodecCtx->qcompress = 0.6; //default 0.5

//        pCodecCtx->qmin = 2;
//        pCodecCtx->qmax = 31;
//        pCodecCtx->qcompress = 0.50000000;

        //H.264 打开注释视频起始位置编码有问题，前几个画面非常不清晰
//        if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
//        av_dict_set(&param, "tune", "animation", 0);
//        av_dict_set(&param, "profile", "baseline", 0);
////            av_dict_set(&param, "tune", "zerolatency", 0);
////            av_opt_set(pCodecCtx->priv_data, "preset", "film", 0);
            av_opt_set(pCodecCtx->priv_data, "preset", "superfast", 0);
            av_dict_set(&param, "profile", "baseline", 0);
        av_dict_set(&param, "crf", "26", 0);
////            av_dict_set(&param, "profile", "main", 0);
//        }
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

    return 0;
}

bool isEnd = 0;
FILE *fpOut = fopen("/storage/emulated/0/out.pcm", "ab+");

int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index){
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
          CODEC_CAP_DELAY))
        return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt,
                                     NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame){
            ret=0;
            break;
        }
        av_log(NULL, AV_LOG_FATAL,"Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",enc_pkt.size);
        /* mux encoded frame */
        ret = av_interleaved_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}

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
    int frameCount = 0;
    while (!isEnd) {
        // 错误信息：Provided packet is too small, needs to be 1647 解决办法：重新初始化packet
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        uint8_t *ele = *video_frame_queue.wait_and_pop().get();//yv21
//        pFrame->pts = frameCount * (ofmt_ctx->streams[0]->time_base.den) /
//                      ((ofmt_ctx->streams[0]->time_base.num) * arguments->frame_rate);
        //第二种写法：av_rescale_q(1, out_codec_context->time_base, video_st->time_base);

//        pFrame->pts = frameCount;
        av_log(NULL, AV_LOG_FATAL, "pts %lld \n", pFrame->pts);

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
        pFrame->height = arguments->out_height;
        pFrame->width = arguments->out_width;
        pFrame->format = AV_PIX_FMT_YUV420P;
//        pFrame->key_frame =1;
        AVCodecContext *codec = ofmt_ctx->streams[0]->codec;
        //frameCount*12800/25
        //pFrame 第25帧时， pts = 12800
          pFrame->pts = frameCount * (ofmt_ctx->streams[0]->time_base.den) /
                              ((ofmt_ctx->streams[0]->time_base.num) * 25);
        int ret = avcodec_encode_video2(ofmt_ctx->streams[0]->codec, &enc_pkt, pFrame,
                                        &got_packet_ptr);

        av_log(NULL, AV_LOG_FATAL, " pFrame->key_frame %d ! \n", pFrame->pict_type);

//        enc_pkt.duration=(ofmt_ctx->streams[0]->time_base.den) /
//                         ((ofmt_ctx->streams[0]->time_base.num) * arguments->frame_rate);
        if (ret < 0) {
            av_log(NULL, AV_LOG_FATAL, "Failed to encode! ret:%d\n", ret);
            continue;
        }
        if (got_packet_ptr >= 0) {


//            enc_pkt.pts = av_rescale_q_rnd(enc_pkt.pts, ofmt_ctx->streams[0]->time_base,
//                                           ofmt_ctx->streams[0]->time_base,(AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//            enc_pkt.dts = av_rescale_q_rnd(enc_pkt.dts,  ofmt_ctx->streams[0]->time_base,
//                                           ofmt_ctx->streams[0]->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//
//            enc_pkt.duration = ((ofmt_ctx->streams[0]->time_base.den / ofmt_ctx->streams[0]->time_base.num) / 15);
            av_log(NULL, AV_LOG_FATAL, "Avpacket pts %lld \n", enc_pkt.pts);
            enc_pkt.stream_index = ofmt_ctx->streams[0]->index;
            av_log(NULL, AV_LOG_FATAL, " video encode succetss  ! \n");
//            pkt.pts=pkt.dts=frameCount;
            frameCount++;
            av_log(NULL, AV_LOG_FATAL, "frameCount %d \n", frameCount);

        } else {
            av_log(NULL, AV_LOG_FATAL, " video encode failed  ! \n");
            continue;
        }
//        if(ofmt_ctx->streams[0]->codec->coded_frame->key_frame)
//            enc_pkt.flags |= AV_PKT_FLAG_KEY;
        ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
        av_free_packet(&enc_pkt);

        if (frameCount > 100) {
            isEnd = 1;
            break;
        }
    }

    //Flush Encoder
    int ret = flush_encoder(ofmt_ctx,0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Flushing encoder failed\n");
        return (void *) -1;
    }

    //Write file trailer
    av_write_trailer(ofmt_ctx);
    av_log(NULL, AV_LOG_FATAL, " 编码完毕 \n");
    avcodec_close(ofmt_ctx->streams[0]->codec);
    av_free(pFrame->data);
    av_free(pFrame);
    /* free the streams */
    for (int i = 0; i < ofmt_ctx->nb_streams; i++) {
        av_freep(&ofmt_ctx->streams[i]->codec);
        av_freep(&ofmt_ctx->streams[i]);
    }

    avio_close(ofmt_ctx->pb);
    /* free the stream */
    av_free(ofmt_ctx);
    return 0;
}

void *startAudioEncode(void *args) {
    int frameCount = 0;
    struct UserArguments *arguments = (struct UserArguments *) args;
    AVFormatContext *ofmt_ctx = arguments->pFmt_ctx;
    AVFrame *pFrame = av_frame_alloc();
    AVCodecContext *codecContext = ofmt_ctx->streams[1]->codec;
    int size = av_samples_get_buffer_size(NULL, codecContext->channels, codecContext->frame_size,
                                          codecContext->sample_fmt, 1);
    av_log(NULL, AV_LOG_FATAL, "av_samples_get_buffer_size:%d\n", size);
    av_log(NULL, AV_LOG_FATAL, ", codecContext->frame_size:%d\n", codecContext->frame_size);

    uint8_t *frame_buf = (uint8_t *) av_malloc(size);
    avcodec_fill_audio_frame(pFrame, codecContext->channels, codecContext->sample_fmt,
                             (const uint8_t *) frame_buf, size, 1);
    AVPacket enc_pkt;
    av_new_packet(&enc_pkt, size);
    int got_packet_ptr = 0;


    while (!isEnd) {

        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        uint8_t *ele = *audio_frame_queue.wait_and_pop().get();//yv21

        pFrame->data[0] = ele;  //PCM Data
        //codecontext timebase 44100
        pFrame->pts = (frameCount++)*codecContext->frame_size;
        pFrame->nb_samples= codecContext->frame_size;
        pFrame->format= codecContext->sample_fmt;
//        if(frameCount>30){
//            fwrite(ele, 1, 2048, fpOut);
//        }else{
//            if(fpOut!=NULL){
//                fclose(fpOut);
//            }
//        }

        int ret = avcodec_encode_audio2(codecContext, &enc_pkt, pFrame, &got_packet_ptr);
        if (ret < 0) {
            av_log(NULL, AV_LOG_FATAL, "Failed to encode! ret:%d\n", ret);
            continue;
        }
        if (got_packet_ptr >= 0) {
            enc_pkt.stream_index = ofmt_ctx->streams[1]->index;
            av_log(NULL, AV_LOG_FATAL, " audio encode succetss  ! \n");
            ret = av_write_frame(ofmt_ctx, &enc_pkt);
            av_free_packet(&enc_pkt);
        } else {
            av_log(NULL, AV_LOG_FATAL, " audio encode failed  ! \n");
            continue;
        }
        if(frameCount>100){
            break;
        }
    }
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
//    int in_width = 720;
//    int in_height = 1280;
    int in_width = 480;
    int in_height = 800;
    int out_height = in_height;
    int out_width = in_width;
    arguments->video_bit_rate = video_bit_rate;
    arguments->frame_rate = frame_rate;
    arguments->audio_bit_rate = 64000;
    arguments->audio_sample_rate = 44100;
    arguments->in_width = in_width;
    arguments->in_height = in_height;
    arguments->out_height = out_height;
    arguments->out_width = out_width;


    av_register_all();
    av_log_set_callback(android_log);
    av_log_set_level(AV_LOG_DEBUG);
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
//    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    //rtmp
    //Network
    avformat_network_init();
    char *rtmp_url = "rtmp://192.168.67.32:1935/live/marshall";
    out_filename = "rtmp://192.168.67.32:1935/live/marshall";
    avformat_alloc_output_context2(&ofmt_ctx, NULL,"flv", out_filename);
    if (!ofmt_ctx) {
        av_log(NULL, AV_LOG_FATAL, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
//        goto end;
    }
    //Guess Format
//        ofmt_ctx->duration = 2000000;

//    ofmt = av_guess_format(NULL, out_filename, NULL);
//    ofmt_ctx->oformat = ofmt;
    //Open output URL
//    if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_READ_WRITE) < 0) {
//        av_log(NULL, AV_LOG_FATAL, "Failed to open output file! \n");
//        return;
//    }

    create_avstream(arguments, ofmt_ctx, VIDEO_STREAM);
    create_avstream(arguments, ofmt_ctx, AUDIO_STREAM);
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    if (!(ofmt_ctx->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log( NULL,AV_LOG_FATAL,"Could not open output URL '%s'", out_filename);
            return;
        }
    }

    avformat_write_header(ofmt_ctx, NULL);

    arguments->pFmt_ctx = ofmt_ctx;

    //输出一下格式------------------

    pthread_t thread;
    pthread_create(&thread, NULL, startVideoEncode, arguments);
    pthread_create(&thread, NULL, startAudioEncode, arguments);

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
int startSendOneVideoFrame(uint8_t *buf) {
    int in_y_size = arguments->in_width * arguments->in_height;

    uint8_t *new_buf = (uint8_t *) malloc(in_y_size * 3 / 2);
    memcpy(new_buf, buf, in_y_size * 3 / 2);
    video_frame_queue.push(new_buf);
    return 0;
}
/**
 * 发送一帧到编码队列
 * @param buf
 * @return
 */
int startSendOneAudioFrame(uint8_t *buf) {
    uint8_t *new_buf = (uint8_t *) malloc(2048);
    memcpy(new_buf, buf, 2048);
    audio_frame_queue.push(new_buf);
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_administrator_mrecord_AudioNativeUtils_pushData(JNIEnv *env, jclass type,
                                                                 jbyteArray buffer_, jint length) {
    jbyte *buffer = env->GetByteArrayElements(buffer_, NULL);
    startSendOneAudioFrame((uint8_t*)buffer);
    env->ReleaseByteArrayElements(buffer_, buffer, 0);
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_administrator_mrecord_AudioNativeUtils_release(JNIEnv *env, jclass type) {

    // TODO

}