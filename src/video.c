#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>

#include <jni.h>

#include <GLES/egl.h>
#include <GLES3/gl3.h>

#include <android/log.h>
#include <android/native_activity.h>

#include <pthread.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#define LOG(...) ((void)__android_log_print(ANDROID_LOG_INFO, "Engine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "Engine", __VA_ARGS__))

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void av_log_callback(void *ptr, int level, const char *fmt, va_list args) {
    (void)ptr;
    pthread_mutex_lock(&log_mutex);

    char log_buffer[2048];
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);

    switch (level) {
    case AV_LOG_ERROR: LOGE("%s", log_buffer); break;
    default: LOG("%s", log_buffer); break;
    }

    pthread_mutex_unlock(&log_mutex);
}

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

typedef struct {
    ANativeWindow *window;

    /* FFmpeg */
    AVFormatContext *format_context;
    AVCodecContext *codec_context;
    AVPacket *pkt;
    AVFrame *frame;
    AVFrame *tmp_frame;
    struct SwsContext *sws_context;

    /* EGL */
    void *egl_display;
    void *egl_surface;
    void *egl_context;
    void *egl_config;

    /* GLES */
    GLuint VAO, program, texture;

    bool running;
    pthread_t thread;
} AndroidApp;

AndroidApp app = {0};

void *run_main(void *arg) {
    (void)arg;

    // Get display
    app.egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (app.egl_display == EGL_NO_DISPLAY) {
        LOGE("cannot get EGL display");
        exit(0);
    }

    // Initialize EGL
    if (!eglInitialize(app.egl_display, NULL, NULL)) {
        LOGE("cannot initialize EGL");
        exit(0);
    }

    // Choose config for GLES v3
    EGLint attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE};
    EGLint numConfigs;
    if (!eglChooseConfig(app.egl_display, attribs, &app.egl_config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("cannot choose EGL config");
        eglTerminate(app.egl_display);
        exit(0);
    }

    // Create context for GLES v2
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE}; // GLES 2.0
    app.egl_context = eglCreateContext(app.egl_display, app.egl_config, EGL_NO_CONTEXT, contextAttribs);
    if (app.egl_context == EGL_NO_CONTEXT) {
        LOGE("cannot create EGL context");
        eglTerminate(app.egl_display);
        exit(0);
    }

    // Create window surface
    app.egl_surface = eglCreateWindowSurface(app.egl_display, app.egl_config, app.window, NULL);
    if (app.egl_surface == EGL_NO_SURFACE) {
        LOGE("cannot create EGL window surface");
        eglDestroyContext(app.egl_display, app.egl_context);
        eglTerminate(app.egl_display);
        exit(0);
    }

    // Make current
    if (!eglMakeCurrent(app.egl_display, app.egl_surface, app.egl_surface, app.egl_context)) {
        LOGE("cannot make EGL context current");
        eglDestroySurface(app.egl_display, app.egl_surface);
        eglDestroyContext(app.egl_display, app.egl_context);
        eglTerminate(app.egl_display);
        exit(0);
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    app.program = glCreateProgram();
    glAttachShader(app.program, vertexShader);
    glAttachShader(app.program, fragmentShader);
    glLinkProgram(app.program);

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
    glGenVertexArrays(1, &app.VAO);
    glGenBuffers(1, &vbo);
    glBindVertexArray(app.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenTextures(1, &app.texture);
    glBindTexture(GL_TEXTURE_2D, app.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLint width, height;
    eglQuerySurface(app.egl_display, app.egl_surface, EGL_WIDTH, &width);
    eglQuerySurface(app.egl_display, app.egl_surface, EGL_HEIGHT, &height);
    glViewport(0, 0, width, height);

    int ret;

    av_log_set_callback(av_log_callback);

    if ((ret = avformat_open_input(&app.format_context, "http://storage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4", NULL, NULL)) < 0) {
        LOG("[ERROR]: avformat_open_input %s", av_err2str(ret));
        exit(1);
    }

    if ((ret = avformat_find_stream_info(app.format_context, NULL)) < 0) {
        LOG("[ERROR]: avformat_find_stream_info %s", av_err2str(ret));
        exit(1);
    }

    const AVCodec *vcodec = avcodec_find_decoder_by_name("h264");
    if ((ret = av_find_best_stream(app.format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0) {
        LOG("[ERROR]: av_find_best_stream %s", av_err2str(ret));
        exit(1);
    }

    AVStream *vstream = app.format_context->streams[ret];

    app.codec_context = avcodec_alloc_context3(vcodec);
    if (!app.codec_context) {
        LOG("[ERROR]: avcodec_alloc_context3");
        exit(1);
    }
    if ((ret = avcodec_parameters_to_context(app.codec_context, vstream->codecpar)) < 0) {
        LOG("[ERROR]: avcodec_parameters_to_context: %s", av_err2str(ret));
        exit(1);
    }

    if ((ret = avcodec_open2(app.codec_context, vcodec, NULL)) < 0) {
        LOG("[ERROR]: avcodec_open2: %s", av_err2str(ret));
        exit(1);
    }

    const AVCodec *acodec = NULL;
    if ((ret = av_find_best_stream(app.format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &acodec, 0)) < 0) {
        LOG("[ERROR]: av_find_best_stream %s", av_err2str(ret));
        exit(1);
    }

    app.pkt = av_packet_alloc();
    if (!app.pkt) {
        LOG("[ERROR]: av_packet_alloc");
        exit(1);
    }

    app.tmp_frame = av_frame_alloc();
    app.frame = av_frame_alloc();
    if (!app.tmp_frame || !app.frame) {
        LOG("[ERROR]: av_frame_alloc");
        exit(1);
    }

    int64_t launch_time = av_gettime_relative();

    while (app.running) {
        ret = av_read_frame(app.format_context, app.pkt);
        if (ret == AVERROR_EOF) {
            av_seek_frame(app.format_context, vstream->index, 0, 0);
            avcodec_flush_buffers(app.codec_context);
        }
        if (ret == AVERROR(EAGAIN)) continue;

        if (vstream && app.pkt->stream_index == vstream->index) {
            if ((ret = avcodec_send_packet(app.codec_context, app.pkt)) < 0) {
                LOG("[ERROR]: avcodec_send_packet: %s", av_err2str(ret));
                exit(1);
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(app.codec_context, app.tmp_frame);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;

                int64_t pts = (1000 * 1000 * app.frame->pts * vstream->time_base.num) / vstream->time_base.den;
                int64_t rts = av_gettime_relative() - launch_time;
                if (pts > rts) av_usleep(pts - rts);
                printf("%04ld | PTS %lds %03ldms %03ldus | RTS %lds %03ldms %03ldus | FMT: %s\n", app.codec_context->frame_num, pts / 1000000, (pts % 1000000) / 1000, pts % 1000, rts / 1000000, (rts % 1000000) / 1000, rts % 1000, av_get_pix_fmt_name(app.frame->format));

                if (!app.sws_context) {
                    app.sws_context = sws_getContext(app.tmp_frame->width, app.tmp_frame->height, app.tmp_frame->format, app.tmp_frame->width, app.tmp_frame->height, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);
                }
                if ((ret = sws_scale_frame(app.sws_context, app.frame, app.tmp_frame)) < 0) {
                    LOG("[ERROR]: sws_scale_frame: %s", av_err2str(ret));
                    exit(1);
                }

                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glUseProgram(app.program);
                glBindVertexArray(app.VAO);
                glBindTexture(GL_TEXTURE_2D, app.texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, app.frame->width, app.frame->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, app.frame->data[0]);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindTexture(GL_TEXTURE_2D, 0);
                glBindVertexArray(0);
                eglSwapBuffers(app.egl_display, app.egl_surface);
            }
        }

        av_packet_unref(app.pkt);
    }

    return NULL;
}

void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *w) {
    LOG("onNativeWindowCreated");

    (void)activity;

    app.window = w;
    if (app.window == NULL) {
        LOGE("no window available");
        return;
    }

    app.running = true;
    pthread_create(&app.thread, NULL, run_main, NULL);
}

void onNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window) {
    (void)activity;
    (void)window;
    LOG("onNativeWindowDestroyed");

    app.running = false;
    pthread_join(app.thread, NULL);

    // Unbind EGL context and surface
    if (!eglMakeCurrent(app.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
        LOGE("cannot unbind EGL context");
    }

    // Destroy EGL surface and context
    if (!eglDestroySurface(app.egl_display, app.egl_surface)) {
        LOGE("cannot destroy EGL surface");
    }

    if (!eglDestroyContext(app.egl_display, app.egl_context)) {
        LOGE("cannot destroy EGL context");
    }

    // Terminate EGL
    if (!eglTerminate(app.egl_display)) {
        LOGE("cannot terminate EGL");
    }

    app.egl_context = NULL;
    app.egl_display = NULL;
    app.egl_surface = NULL;
    app.window = NULL;
}

JNIEXPORT void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    (void)savedState;
    (void)savedStateSize;

    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    activity->instance = NULL;
}
