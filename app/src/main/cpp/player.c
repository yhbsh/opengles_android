#include <jni.h>

#include <GLES3/gl3.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <android/log.h>
#include <errno.h>
#include <libavutil/error.h>

#define LOG_TAG "VideoPlayer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static const char *url = "rtmp://192.168.1.107:1935/live/stream";

const char *vs =
    "#version 320 es\n"
    "layout (location = 0) in vec4 position;\n"
    "layout (location = 1) in vec2 texCoord;\n"
    "out vec2 texCoordVarying;\n"
    "void main() {\n"
    "    gl_Position = position;\n"
    "    texCoordVarying = texCoord;\n"
    "}\n";

const char *fs =
    "#version 320 es\n"
    "precision mediump float;\n"
    "in vec2 texCoordVarying;\n"
    "uniform sampler2D textureY;\n"
    "uniform sampler2D textureU;\n"
    "uniform sampler2D textureV;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    vec3 yuv;\n"
    "    vec3 rgb;\n"
    "    yuv.x = texture(textureY, texCoordVarying).r;\n"
    "    yuv.y = texture(textureU, texCoordVarying).r - 0.5;\n"
    "    yuv.z = texture(textureV, texCoordVarying).r - 0.5;\n"
    "    rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.344, 1.772, 1.402, -0.714, 0.0) * yuv;\n"
    "    fragColor = vec4(rgb, 1.0);\n"
    "}\n";

int ret;
AVFormatContext *formatContext = NULL;
AVCodecContext *codecContext = NULL;
AVStream *stream = NULL;
AVFrame *frame = NULL;
AVPacket *packet = NULL;
GLuint textureY, textureU, textureV;
GLuint program;

JNIEXPORT void JNICALL
Java_com_example_triangle_MainActivity_init(JNIEnv *env, jclass clazz) {
    ret = avformat_open_input(&formatContext, url, NULL, NULL);
    if (ret < 0) {
        LOGI("[ERROR]: avformat_open_input(): %s", av_err2str(ret));
        return;
    }
    ret = avformat_find_stream_info(formatContext, NULL);
    if (ret < 0) {
        LOGI("[ERROR]: avformat_find_stream_info(): %s", av_err2str(ret));
        return;
    }

    stream = formatContext->streams[1];
    if (stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
        LOGI("[ERROR]: no video stream, please change index: %s", av_err2str(ret));
        return;
    }

    enum AVCodecID codec_id = stream->codecpar->codec_id;
    const AVCodec *codec = avcodec_find_decoder(codec_id);
    if (codec == NULL) {
        LOGI("[ERROR]: avcodec_find_decoder(): %s", strerror(errno));
        return;
    }

    codecContext = avcodec_alloc_context3(codec);
    if (codecContext == NULL) {
        LOGI("[ERROR]: avcodec_allow_context3(): %s", strerror(errno));
        return;
    }

    ret = avcodec_parameters_to_context(codecContext, stream->codecpar);
    if (ret < 0) {
        LOGI("[ERROR]: avcodec_parameters_to_context(): %s", av_err2str(ret));
        return;
    }

    ret = avcodec_open2(codecContext, codec, NULL);
    if (ret < 0) {
        LOGI("[ERROR]: avcodec_parameters_to_context(): %s", av_err2str(ret));
        return;
    }

    frame = av_frame_alloc();
    if (frame == NULL) {
        LOGI("[ERROR]: av_frame_alloc(): %s", strerror(errno));
        return;
    }
    packet = av_packet_alloc();
    if (packet == NULL) {
        LOGI("[ERROR]: av_packet_alloc(): %s", strerror(errno));
        return;
    }

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vs, NULL);
    glCompileShader(vertex_shader);
    GLint vertexStatus;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertexStatus);
    if (!vertexStatus) {
        GLchar infoLog[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
        LOGI("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fs, NULL);
    glCompileShader(fragment_shader);
    GLint fragmentStatus;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragmentStatus);
    if (!fragmentStatus) {
        GLchar infoLog[512];
        glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
        LOGI("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s", infoLog);
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        LOGI("ERROR::PROGRAM::LINKING_FAILED\n%s", infoLog);
    }
    glUseProgram(program);

    glGenTextures(1, &textureY);
    glGenTextures(1, &textureV);
    glGenTextures(1, &textureU);

    GLfloat vertices[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat texCoords[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(texCoords), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(texCoords), texCoords);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)sizeof(vertices));
    glEnableVertexAttribArray(1);
}

JNIEXPORT void JNICALL
Java_com_example_triangle_MainActivity_step(JNIEnv *env, jclass clazz) {
    ret = av_read_frame(formatContext, packet);
    if (ret < 0 && ret != AVERROR_EOF) {
        LOGI("[ERROR]: av_read_frame(): %s", av_err2str(ret));
        return;
    }
    if (packet->stream_index != stream->index) {
      return;
    }

    ret = avcodec_send_packet(codecContext, packet);
    while (ret >= 0) {
      ret = avcodec_receive_frame(codecContext, frame);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      }

      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, textureY);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width, frame->height, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glUniform1i(glGetUniformLocation(program, "textureY"), 0);

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, textureU);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glUniform1i(glGetUniformLocation(program, "textureU"), 1);

      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, textureV);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glUniform1i(glGetUniformLocation(program, "textureV"), 2);

      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

JNIEXPORT void JNICALL
Java_com_example_triangle_MainActivity_resize(JNIEnv *env, jclass clazz, jint width,
                                              jint height) {

}
