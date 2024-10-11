#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>

#include "android_native_app_glue.h"
#include "libavutil/frame.h"

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define LOG_TAG "Engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void android_main(struct android_app *app) {
    SLObjectItf sl_engine_obj = NULL;
    SLEngineItf sl_engine = NULL;
    SLObjectItf out_mix_obj = NULL;
    SLObjectItf player_obj = NULL;
    SLPlayItf player_play = NULL;
    SLAndroidSimpleBufferQueueItf buff_queue = NULL;

    int events;
    struct android_poll_source *source;

    int ret;
    SLresult result;

    // Create the engine
    if ((result = slCreateEngine(&sl_engine_obj, 0, NULL, 0, NULL, NULL)) < 0) {
        LOGE("Error during %s: %d", "slCreateEngine", result);
        exit(0);
    }

    // Realize the engine
    if ((result = (*sl_engine_obj)->Realize(sl_engine_obj, SL_BOOLEAN_FALSE)) != SL_RESULT_SUCCESS) {
        LOGE("Error during %s: %d", "Realize engine", result);
        exit(0);
    }

    // Get the engine interface
    if ((result = (*sl_engine_obj)->GetInterface(sl_engine_obj, SL_IID_ENGINE, &sl_engine)) != SL_RESULT_SUCCESS) {
        LOGE("Error during %s: %d", "Get engine interface", result);
        exit(0);
    }

    // Create the output mix
    if ((result = (*sl_engine)->CreateOutputMix(sl_engine, &out_mix_obj, 0, NULL, NULL)) != SL_RESULT_SUCCESS) {
        LOGE("Error during %s: %d", "Create output mix", result);
        exit(0);
    }

    // Realize the output mix
    if ((result = (*out_mix_obj)->Realize(out_mix_obj, SL_BOOLEAN_FALSE)) != SL_RESULT_SUCCESS) {
        LOGE("Error during %s: %d", "Realize output mix", result);
        exit(0);
    }

    // Data source configuration for buffer queue
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,
                                   1, // Mono channel
                                   SL_SAMPLINGRATE_44_1,
                                   SL_PCMSAMPLEFORMAT_FIXED_16, // 16-bit samples
                                   SL_PCMSAMPLEFORMAT_FIXED_16, // 16-bit samples
                                   SL_SPEAKER_FRONT_CENTER,
                                   SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // Data sink configuration
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, out_mix_obj};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // Create audio player with buffer queue interface
    const SLInterfaceID ids[2] = {SL_IID_PLAY, SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    if ((result = (*sl_engine)->CreateAudioPlayer(sl_engine, &player_obj, &audioSrc, &audioSnk, 2, ids, req)) != SL_RESULT_SUCCESS) {
        LOGE("Error during %s: %d", "Create audio player", result);
        exit(0);
    }

    // Realize the player
    if ((result = (*player_obj)->Realize(player_obj, SL_BOOLEAN_FALSE)) != SL_RESULT_SUCCESS) {
        LOGE("Error during %s: %d", "Realize audio player", result);
        exit(0);
    }

    // Get the play interface
    if ((result = (*player_obj)->GetInterface(player_obj, SL_IID_PLAY, &player_play)) != SL_RESULT_SUCCESS) {
        LOGE("Error during %s: %d", "Get play interface", result);
        exit(0);
    }

    // Get the buffer queue interface
    if ((result = (*player_obj)->GetInterface(player_obj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &buff_queue)) != SL_RESULT_SUCCESS) {
        LOGE("Error during %s: %d", "Get buffer queue interface", result);
        exit(0);
    }

    // Start playing the audio
    if ((result = (*player_play)->SetPlayState(player_play, SL_PLAYSTATE_PLAYING)) != SL_RESULT_SUCCESS) {
        LOGE("Error during %s: %d", "Set play state to PLAYING", result);
        exit(0);
    }

    AVFormatContext *format_context = NULL;

    if ((ret = avformat_open_input(&format_context, "http://storage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4", NULL, NULL)) < 0) {
        LOGE("avformat_open_input: %s", av_err2str(ret));
        exit(0);
    }

    if ((ret = avformat_find_stream_info(format_context, NULL)) < 0) {
        LOGE("avformat_find_stream_info: %s", av_err2str(ret));
        exit(0);
    }

    const AVCodec *codec = NULL;
    if ((ret = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0)) < 0) {
        LOGE("av_find_best_stream: %s", av_err2str(ret));
        exit(0);
    }

    AVStream *stream = format_context->streams[ret];
    AVCodecContext *codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        LOGE("avcodec_alloc_context3");
        exit(0);
    }

    if ((ret = avcodec_parameters_to_context(codec_context, stream->codecpar)) < 0) {
        LOGE("avcodec_parameters_to_context: %s", av_err2str(ret));
        exit(0);
    }

    if ((ret = avcodec_open2(codec_context, codec, NULL)) < 0) {
        LOGE("avcodec_open2: %s", av_err2str(ret));
        exit(0);
    }

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *pcm_frame = av_frame_alloc();

    struct SwrContext *swr_context = NULL;
    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_MONO;
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = 44100;

    // Initialize pcm_frame with the desired output format
    pcm_frame->format = out_sample_fmt;
    pcm_frame->sample_rate = out_sample_rate;
    pcm_frame->ch_layout = out_ch_layout;

    if ((ret = swr_alloc_set_opts2(&swr_context, &out_ch_layout, out_sample_fmt, out_sample_rate, &codec_context->ch_layout, codec_context->sample_fmt, codec_context->sample_rate, 0, NULL)) < 0) {
        printf("swr_alloc_set_opts2: %s\n", av_err2str(ret));
        exit(0);
    }

    // Initialize the resampler
    if ((ret = swr_init(swr_context)) < 0) {
        printf("swr_init: %s\n", av_err2str(ret));
        exit(0);
    }

    int64_t start = av_gettime_relative();
    int64_t last_pts_time = start;

    while (1) {
        while (ALooper_pollAll(0, NULL, &events, (void **)&source) >= 0) {
            if (source != NULL) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                exit(0);
            }
        }

        ret = av_read_frame(format_context, pkt);
        if (ret == AVERROR_EOF) {
            if ((ret = av_seek_frame(format_context, stream->index, 0, 0)) < 0) {
                LOGE("av_seek_frame: %s\n", av_err2str(ret));
                exit(0);
            }
            avcodec_flush_buffers(codec_context);
        }
        if (ret == AVERROR(EAGAIN)) continue;

        if (pkt->stream_index != stream->index) {
            av_packet_unref(pkt);
            continue;
        }

        if ((ret = avcodec_send_packet(codec_context, pkt)) < 0) {
            LOGE("avcodec_send_packet: %s", av_err2str(ret));
            exit(0);
        }

        while ((ret = avcodec_receive_frame(codec_context, frame)) >= 0) {
            int64_t fpts = av_rescale_q(frame->pts, stream->time_base, AV_TIME_BASE_Q);
            int64_t curr = av_gettime_relative();
            int64_t delay = fpts - (av_gettime_relative() - start);

            if (delay > 0) {
                unsigned int sleep_time = (delay > UINT_MAX) ? UINT_MAX : (unsigned int)delay;
                av_usleep(sleep_time);
            }

            // Set up pcm_frame parameters based on the input frame
            pcm_frame->format = out_sample_fmt;
            pcm_frame->sample_rate = out_sample_rate;
            pcm_frame->ch_layout = out_ch_layout;
            pcm_frame->nb_samples = frame->nb_samples;

            // Allocate buffer for pcm_frame
            if ((ret = av_frame_get_buffer(pcm_frame, 0)) < 0) {
                LOGE("av_frame_get_buffer: %s\n", av_err2str(ret));
                exit(0);
            }

            // Perform the actual resampling
            if ((ret = swr_convert_frame(swr_context, pcm_frame, frame)) < 0) {
                LOGE("swr_convert_frame: %s\n", av_err2str(ret));
                exit(0);
            }

            result = (*buff_queue)->Enqueue(buff_queue, pcm_frame->data[0], pcm_frame->linesize[0]);
            if (result != SL_RESULT_SUCCESS) {
                LOGE("Failed to enqueue buffer: %d", result);
            }

            av_frame_unref(frame);
            av_frame_unref(pcm_frame);
        }

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            ret = 0;
        } else if (ret < 0) {
            LOGE("avcodec_send_packet: %s\n", av_err2str(ret));
            exit(0);
        }

        av_packet_unref(pkt);
    }
}
