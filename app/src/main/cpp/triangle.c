#include <jni.h>

#include <jni.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <math.h>

#define LOG_TAG "Native Code"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

GLuint shaderProgram;
GLuint VAO;

JNIEXPORT void JNICALL
Java_com_example_triangle_MainActivity_init(JNIEnv *env, jclass clazz) {
    LOGI("Native method init() called.");

    const char *vertexShaderCode =
            "#version 300 es                                  \n"
            "layout(location = 0) in vec4 vPosition;          \n"
            "uniform mat4 uRotationMatrix;                    \n"
            "void main()                                      \n"
            "{                                                \n"
            "   gl_Position = uRotationMatrix * vPosition;    \n"
            "}                                                \n";

    const char *fragmentShaderCode =
            "#version 300 es                              \n"
            "precision mediump float;                     \n"
            "out vec4 fragColor;                          \n"
            "void main()                                  \n"
            "{                                            \n"
            "   fragColor = vec4 (1.0, 0.0, 0.0, 1.0);    \n"
            "}                                            \n";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLfloat vertices[] = {
            -0.5f, -0.288675f, 0.0f,
            0.5f, -0.288675f, 0.0f,
            0.0f, 0.57735f, 0.0f
    };
    GLuint VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

JNIEXPORT void JNICALL
Java_com_example_triangle_MainActivity_step(JNIEnv *env, jclass clazz) {
    LOGI("Native method step() called.");

    static float angle = 0.0f;
    angle += 0.02f;

    float cosAngle = cosf(angle);
    float sinAngle = sinf(angle);

    GLfloat rotationMatrix[] = {
            cosAngle, -sinAngle, 0.0, 0.0,
            sinAngle, cosAngle, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0
    };

    GLuint rotationLoc = glGetUniformLocation(shaderProgram, "uRotationMatrix");
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);

    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, rotationMatrix);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

JNIEXPORT void JNICALL
Java_com_example_triangle_MainActivity_resize(JNIEnv *env, jclass clazz, jint width,
                                              jint height) {
    LOGI("Native method resize() called.");
    glViewport(0, 0, width, height);
}
