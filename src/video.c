#include <GLES3/gl3.h>
#include <android/log.h>
#include <jni.h>

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

typedef struct {
    pthread_t thread;
    pthread_mutex_t lock;

    /* FFmpeg */
    AVFormatContext *format_context;
    AVCodecContext *vcodec_context;
    AVCodecContext *acodec_context;
    AVPacket *packet;
    AVFrame *frame;
    AVFrame *tmp_frame;
    struct SwsContext *sws_context;

    int next_frame_available;

    /* OpenGL */
    GLuint program, vao, texture;

} Engine;

static Engine engine = {0};

void checkCompileErrors(GLuint shader, const char *type) {
    GLint success;
    GLchar infoLog[1024];
    if (strcmp(type, "PROGRAM") != 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            LOGI("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s", type, infoLog);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            LOGI("ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s", type, infoLog);
        }
    }
}

void *init_ffmpeg(void *arg) {

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

    engine.vcodec_context = avcodec_alloc_context3(vcodec);
    if (!engine.vcodec_context) {
        LOGI("[ERROR]: avcodec_alloc_context3");
        exit(1);
    }
    if ((ret = avcodec_parameters_to_context(engine.vcodec_context, vstream->codecpar)) < 0) {
        LOGI("[ERROR]: avcodec_parameters_to_context: %s", av_err2str(ret));
        exit(1);
    }

    if ((ret = avcodec_open2(engine.vcodec_context, vcodec, NULL)) < 0) {
        LOGI("[ERROR]: avcodec_open2: %s", av_err2str(ret));
        exit(1);
    }

    const AVCodec *acodec = NULL;
    if ((ret = av_find_best_stream(engine.format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &acodec, 0)) < 0) {
        LOGI("[ERROR]: av_find_best_stream %s", av_err2str(ret));
        exit(1);
    }

    AVStream *astream = engine.format_context->streams[ret];

    engine.acodec_context = avcodec_alloc_context3(acodec);
    if (!engine.acodec_context) {
        LOGI("[ERROR]: aacodec_alloc_context3");
        exit(1);
    }
    if ((ret = avcodec_parameters_to_context(engine.acodec_context, astream->codecpar)) < 0) {
        LOGI("[ERROR]: aacodec_parameters_to_context: %s", av_err2str(ret));
        exit(1);
    }
    if ((ret = avcodec_open2(engine.acodec_context, acodec, NULL)) < 0) {
        LOGI("[ERROR]: aacodec_open2: %s", av_err2str(ret));
        exit(1);
    }

    engine.packet = av_packet_alloc();
    if (!engine.packet) {
        LOGI("[ERROR]: av_packet_alloc");
        exit(1);
    }

    engine.tmp_frame = av_frame_alloc();
    engine.frame     = av_frame_alloc();
    if (!engine.tmp_frame || !engine.frame) {
        LOGI("[ERROR]: av_frame_alloc");
        exit(1);
    }

    int64_t start         = av_gettime_relative();
    int64_t last_pts_time = start;

    while (1) {
        ret = av_read_frame(engine.format_context, engine.packet);
        if (ret == AVERROR_EOF) {
            av_seek_frame(engine.format_context, vstream->index, 0, 0);
            avcodec_flush_buffers(engine.vcodec_context);
        }
        if (ret == AVERROR(EAGAIN)) continue;

        if (vstream && engine.packet->stream_index == vstream->index) {
            if ((ret = avcodec_send_packet(engine.vcodec_context, engine.packet)) < 0) {
                LOGI("[ERROR]: avcodec_send_packet: %s\n", av_err2str(ret));
                exit(1);
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(engine.vcodec_context, engine.tmp_frame);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;

                int64_t fpts  = av_rescale_q(engine.tmp_frame->pts, vstream->time_base, AV_TIME_BASE_Q);
                int64_t curr  = av_gettime_relative();
                int64_t delay = fpts - (av_gettime_relative() - start);

                if (delay > 0) {
                    unsigned int sleep_time = (delay > UINT_MAX) ? UINT_MAX : (unsigned int)delay;
                    av_usleep(sleep_time);
                }

                if (!engine.sws_context) {
                    engine.sws_context = sws_getContext(engine.tmp_frame->width, engine.tmp_frame->height, engine.tmp_frame->format, // Input format
                                                        engine.tmp_frame->width, engine.tmp_frame->height, AV_PIX_FMT_RGBA,          // Output format
                                                        SWS_BICUBIC, NULL, NULL, NULL);
                }
                if ((ret = sws_scale_frame(engine.sws_context, engine.frame, engine.tmp_frame)) < 0) {
                    LOGI("[ERROR]: sws_scale_frame: %s\n", av_err2str(ret));
                    exit(1);
                }

                pthread_mutex_lock(&engine.lock);
                engine.next_frame_available = 1;
                pthread_mutex_unlock(&engine.lock);

                last_pts_time = curr;
            }
        } else if (astream && engine.packet->stream_index == astream->index) {
            // LOGI("Audio Packet: %d\n", engine.packet->size);
        }

        av_packet_unref(engine.packet);
    }

    return NULL;
}

JNIEXPORT void JNICALL Java_com_example_gles3_MainActivity_init(JNIEnv *env, jclass clazz) {
    pthread_create(&engine.thread, NULL, init_ffmpeg, NULL);

    // Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkCompileErrors(vertexShader, "VERTEX");

    // Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkCompileErrors(fragmentShader, "FRAGMENT");

    // Shader Program
    engine.program = glCreateProgram();
    glAttachShader(engine.program, vertexShader);
    glAttachShader(engine.program, fragmentShader);
    glLinkProgram(engine.program);
    checkCompileErrors(engine.program, "PROGRAM");

    // Clean up shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Setup vertex data and buffers and configure vertex attributes
    static GLfloat vertices[] = {
        // positions  // texture coords
        -1.0f, +1.0f, +0.0f, +0.0f, // top-left
        -1.0f, -1.0f, +0.0f, +1.0f, // bottom-left
        +1.0f, +1.0f, +1.0f, +0.0f, // top-right
        +1.0f, -1.0f, +1.0f, +1.0f  // bottom-right
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

    // Generate texture
    glGenTextures(1, &engine.texture);
    glBindTexture(GL_TEXTURE_2D, engine.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

JNIEXPORT void JNICALL Java_com_example_gles3_MainActivity_step(JNIEnv *env, jclass clazz) {
    pthread_mutex_lock(&engine.lock);
    if (engine.next_frame_available) {
        // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(engine.program);
        glBindVertexArray(engine.vao);
        glBindTexture(GL_TEXTURE_2D, engine.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, engine.frame->width, engine.frame->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, engine.frame->data[0]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);

        // LOGI("Frame: %s\n", av_get_pix_fmt_name(engine.frame->format));
    }
    pthread_mutex_unlock(&engine.lock);
}

JNIEXPORT void JNICALL Java_com_example_gles3_MainActivity_resize(JNIEnv *env, jclass clazz, jint width, jint height) {
    glViewport(0, 0, width, height);
    LOGI("Viewport resized to user dimensions %d x %d", width, height);
}
