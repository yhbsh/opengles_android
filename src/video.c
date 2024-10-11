#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>

#include "android_native_app_glue.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>

#include <pthread.h>
#include <string.h>

#define LOG_TAG "Engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

const char *vertexShaderSource = "#version 300 es\n"
                                 "layout(location = 0) in vec4 aPosition;\n"
                                 "layout(location = 1) in vec2 aTexCoord;\n"
                                 "out vec2 vTexCoord;\n"
                                 "void main() {\n"
                                 "   gl_Position = aPosition;\n"
                                 "   vTexCoord = aTexCoord;\n"
                                 "}\n";

const char *fragmentShaderSource = "#version 300 es\n"
                                   "precision mediump float;\n"
                                   "in vec2 vTexCoord;\n"
                                   "out vec4 FragColor;\n"
                                   "uniform sampler2D uTexture;\n"
                                   "void main() {\n"
                                   "   FragColor = texture(uTexture, vTexCoord);\n"
                                   "}\n";
typedef struct Engine {
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t cond;

    /* FFmpeg */
    AVFormatContext *format_context;
    AVCodecContext *codec_context;
    AVPacket *pkt;
    AVFrame *frame;
    AVFrame *tmp_frame;
    struct SwsContext *sws_context;

    /* OpenGL */
    GLuint program, vao, texture;

    /* EGL */
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
} Engine;

static Engine engine = {0};

void init_ffmpeg(void);
void init_opengl(struct android_app *app);
void cleanup_ffmpeg(void);
void cleanup_opengl(void);

void *decode_thread(void *arg) {
    if (engine.format_context != NULL) return NULL;
    int ret;
    av_log_set_level(AV_LOG_QUIET);

    if ((ret = avformat_open_input(&engine.format_context, "http://storage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4", NULL, NULL)) < 0) {
        LOGI("[ERROR]: avformat_open_input %s", av_err2str(ret));
        exit(1);
    }

    if ((ret = avformat_find_stream_info(engine.format_context, NULL)) < 0) {
        LOGI("[ERROR]: avformat_find_stream_info %s", av_err2str(ret));
        exit(1);
    }

    const AVCodec *vcodec = avcodec_find_decoder_by_name("h264");
    if ((ret = av_find_best_stream(engine.format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0) {
        LOGI("[ERROR]: av_find_best_stream %s", av_err2str(ret));
        exit(1);
    }

    AVStream *vstream = engine.format_context->streams[ret];

    engine.codec_context = avcodec_alloc_context3(vcodec);
    if (!engine.codec_context) {
        LOGI("[ERROR]: avcodec_alloc_context3");
        exit(1);
    }
    if ((ret = avcodec_parameters_to_context(engine.codec_context, vstream->codecpar)) < 0) {
        LOGI("[ERROR]: avcodec_parameters_to_context: %s", av_err2str(ret));
        exit(1);
    }

    if ((ret = avcodec_open2(engine.codec_context, vcodec, NULL)) < 0) {
        LOGI("[ERROR]: avcodec_open2: %s", av_err2str(ret));
        exit(1);
    }

    const AVCodec *acodec = NULL;
    if ((ret = av_find_best_stream(engine.format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &acodec, 0)) < 0) {
        LOGI("[ERROR]: av_find_best_stream %s", av_err2str(ret));
        exit(1);
    }

    AVStream *astream = engine.format_context->streams[ret];

    engine.pkt = av_packet_alloc();
    if (!engine.pkt) {
        LOGI("[ERROR]: av_packet_alloc");
        exit(1);
    }

    engine.tmp_frame = av_frame_alloc();
    engine.frame = av_frame_alloc();
    if (!engine.tmp_frame || !engine.frame) {
        LOGI("[ERROR]: av_frame_alloc");
        exit(1);
    }

    int64_t start = av_gettime_relative();
    int64_t last_pts_time = start;

    while (1) {
        ret = av_read_frame(engine.format_context, engine.pkt);
        if (ret == AVERROR_EOF) {
            av_seek_frame(engine.format_context, vstream->index, 0, 0);
            avcodec_flush_buffers(engine.codec_context);
        }
        if (ret == AVERROR(EAGAIN)) continue;

        if (vstream && engine.pkt->stream_index == vstream->index) {
            if ((ret = avcodec_send_packet(engine.codec_context, engine.pkt)) < 0) {
                LOGI("[ERROR]: avcodec_send_packet: %s", av_err2str(ret));
                exit(1);
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(engine.codec_context, engine.tmp_frame);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;

                int64_t fpts = av_rescale_q(engine.tmp_frame->pts, vstream->time_base, AV_TIME_BASE_Q);
                int64_t curr = av_gettime_relative();
                int64_t delay = fpts - (av_gettime_relative() - start);

                if (delay > 0) {
                    unsigned int sleep_time = (delay > UINT_MAX) ? UINT_MAX : (unsigned int)delay;
                    av_usleep(sleep_time);
                }

                if (!engine.sws_context) {
                    engine.sws_context = sws_getContext(engine.tmp_frame->width, engine.tmp_frame->height, engine.tmp_frame->format, engine.tmp_frame->width,
                                                        engine.tmp_frame->height, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);
                }
                if ((ret = sws_scale_frame(engine.sws_context, engine.frame, engine.tmp_frame)) < 0) {
                    LOGI("[ERROR]: sws_scale_frame: %s", av_err2str(ret));
                    exit(1);
                }

                pthread_mutex_lock(&engine.lock);
                pthread_cond_signal(&engine.cond);
                pthread_mutex_unlock(&engine.lock);

                last_pts_time = curr;
            }
        }

        av_packet_unref(engine.pkt);
    }

    return NULL;
}

void init_ffmpeg() {
    pthread_mutex_init(&engine.lock, NULL);
    pthread_cond_init(&engine.cond, NULL);
    pthread_create(&engine.thread, NULL, decode_thread, NULL);
}

void checkCompileErrors(GLuint shader, const char *type) {
    GLint success;
    GLchar infoLog[1024];
    if (strcmp(type, "PROGRAM") != 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            LOGI("ERROR::SHADER_COMPILATION_ERROR of type: %s%s", type, infoLog);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            LOGI("ERROR::PROGRAM_LINKING_ERROR of type: %s%s", type, infoLog);
        }
    }
}

void init_opengl(struct android_app *app) {
    ANativeWindow *window = app->window;

    engine.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (engine.display == EGL_NO_DISPLAY) {
        LOGI("[ERROR]: cannot get egl display");
        exit(1);
    }

    if (!eglInitialize(engine.display, NULL, NULL)) {
        LOGI("[ERROR]: cannot initialize egl");
        exit(1);
    }

    const EGLint configAttribs[] = {
        // clang-format off
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE,
        // clang-format on
    };

    EGLConfig config;
    EGLint configsNum;
    if (!eglChooseConfig(engine.display, configAttribs, &config, 1, &configsNum)) {
        LOGI("[ERROR]: cannot chose egl config");
        exit(1);
    }

    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

    engine.context = eglCreateContext(engine.display, config, EGL_NO_CONTEXT, contextAttribs);
    if (engine.context == EGL_NO_CONTEXT) {
        LOGI("[ERROR]: cannot create egl context");
        exit(1);
    }

    engine.surface = eglCreateWindowSurface(engine.display, config, window, NULL);
    if (engine.surface == EGL_NO_SURFACE) {
        LOGI("[ERROR]: cannot create egl window surface");
        exit(1);
    }

    if (!eglMakeCurrent(engine.display, engine.surface, engine.surface, engine.context)) {
        LOGI("[ERROR]: cannot make egl context current");
        exit(1);
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkCompileErrors(vertexShader, "VERTEX");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkCompileErrors(fragmentShader, "FRAGMENT");

    engine.program = glCreateProgram();
    glAttachShader(engine.program, vertexShader);
    glAttachShader(engine.program, fragmentShader);
    glLinkProgram(engine.program);
    checkCompileErrors(engine.program, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    static GLfloat vertices[] = {
        // clang-format off
        -1.0f, +1.0f, +0.0f, +0.0f,
        -1.0f, -1.0f, +0.0f, +1.0f,
        +1.0f, +1.0f, +1.0f, +0.0f,
        +1.0f, -1.0f, +1.0f, +1.0f,
        // clang-format on
    };

    GLuint vbo;
    glGenVertexArrays(1, &engine.vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(engine.vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenTextures(1, &engine.texture);
    glBindTexture(GL_TEXTURE_2D, engine.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void cleanup_ffmpeg(void) {
    if (engine.pkt) av_packet_free(&engine.pkt);
    if (engine.frame) av_frame_free(&engine.frame);
    if (engine.tmp_frame) av_frame_free(&engine.tmp_frame);
    if (engine.codec_context) avcodec_free_context(&engine.codec_context);
    if (engine.codec_context) avcodec_free_context(&engine.codec_context);
    if (engine.format_context) avformat_close_input(&engine.format_context);
    if (engine.sws_context) sws_freeContext(engine.sws_context);
}

void cleanup_opengl(void) {
    if (engine.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine.context != EGL_NO_CONTEXT) eglDestroyContext(engine.display, engine.context);
        if (engine.surface != EGL_NO_SURFACE) eglDestroySurface(engine.display, engine.surface);
        eglTerminate(engine.display);
    }
    engine.display = EGL_NO_DISPLAY;
    engine.context = EGL_NO_CONTEXT;
    engine.surface = EGL_NO_SURFACE;
}

void handle_cmd(struct android_app *app, int32_t cmd) {
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window != NULL) init_opengl(app);
        break;
    case APP_CMD_TERM_WINDOW: {
        cleanup_opengl();
        break;
    }
    case APP_CMD_DESTROY: {
        pthread_mutex_lock(&engine.lock);
        cleanup_ffmpeg();
        cleanup_opengl();
        pthread_mutex_unlock(&engine.lock);
        pthread_mutex_destroy(&engine.lock);
        exit(0);
        break;
    }
    }
}

void android_main(struct android_app *app) {
    init_ffmpeg();

    app->onAppCmd = handle_cmd;
    int events;
    struct android_poll_source *source;

    while (1) {
        while (ALooper_pollAll(0, NULL, &events, (void **)&source) >= 0) {
            if (source != NULL) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                pthread_mutex_lock(&engine.lock);
                cleanup_ffmpeg();
                cleanup_opengl();
                pthread_mutex_unlock(&engine.lock);
                pthread_mutex_destroy(&engine.lock);
                pthread_cond_destroy(&engine.cond);
                return;
            }
        }

        pthread_mutex_lock(&engine.lock);
        pthread_cond_wait(&engine.cond, &engine.lock);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(engine.program);
        glBindVertexArray(engine.vao);
        glBindTexture(GL_TEXTURE_2D, engine.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, engine.frame->width, engine.frame->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, engine.frame->data[0]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        eglSwapBuffers(engine.display, engine.surface);

        pthread_mutex_unlock(&engine.lock);
    }
}
