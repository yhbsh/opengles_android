#include <GLES3/gl3.h>
#include <android/log.h>
#include <jni.h>
#include <math.h>
#include <stdlib.h>

#define LOG_TAG "OpenGL Engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

const char *vertSource = "#version 300 es\n"
                         "layout (location = 0) in vec3 aPos;\n"
                         "layout (location = 1) in vec2 aTexCoord;\n"
                         "uniform mat4 uTransform;\n"
                         "out vec2 TexCoord;\n"
                         "void main()\n"
                         "{\n"
                         "   gl_Position = uTransform * vec4(aPos, 1.0);\n"
                         "   TexCoord = aTexCoord;\n"
                         "}\0";

const char *fragSource = "#version 300 es\n"
                         "precision mediump float;\n"
                         "in vec2 TexCoord;\n"
                         "out vec4 FragColor;\n"
                         "uniform sampler2D ourTexture;\n"
                         "void main()\n"
                         "{\n"
                         "   FragColor = texture(ourTexture, TexCoord);\n"
                         "}\n\0";

#define WIDTH 256
#define HEIGHT 256

static GLubyte textureData[WIDTH * HEIGHT * 3];
static float   time = 0.0f;
static GLuint  shaderProgram;
static GLuint  VAO;
static GLuint  texture;

JNIEXPORT void JNICALL Java_com_example_gles3_MainActivity_init(JNIEnv *env, jclass clazz) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 1.0f};

    unsigned int indices[] = {0, 1, 2};

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}

JNIEXPORT void JNICALL Java_com_example_gles3_MainActivity_step(JNIEnv *env, jclass clazz) {
    float waveSpeed     = 0.1f;
    float waveFrequency = 10.0f;
    float waveAmplitude = 128.0f;
    float angle         = time / 5;

    time += waveSpeed;

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            float u = (float) j / WIDTH;
            float v = (float) i / HEIGHT;

            textureData[(i * WIDTH + j) * 3 + 0] = (GLubyte) ((sin(u * waveFrequency + time) + 1) * waveAmplitude);
            textureData[(i * WIDTH + j) * 3 + 1] = (GLubyte) ((sin(v * waveFrequency + time) + 1) * waveAmplitude);
            textureData[(i * WIDTH + j) * 3 + 2] = (GLubyte) ((sin((u + v) * waveFrequency + time) + 1) * waveAmplitude);
        }
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);

    float cosAngle           = cos(angle);
    float sinAngle           = sin(angle);
    float rotationMatrix[16] = {cosAngle, -sinAngle, 0, 0, sinAngle, cosAngle, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    int transformLoc = glGetUniformLocation(shaderProgram, "uTransform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, rotationMatrix);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

JNIEXPORT void JNICALL Java_com_example_gles3_MainActivity_resize(JNIEnv *env, jclass clazz, jint width, jint height) {
    glViewport(0, 0, width, height);
}
