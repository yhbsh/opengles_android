#include <jni.h>

#include <jni.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <math.h>

#define LOG_TAG "Native Code"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

GLuint prog;
GLuint VAO;
GLuint VBO;

JNIEXPORT void JNICALL
Java_com_example_gles3_MainActivity_init(JNIEnv *env, jclass clazz) {
    const char *vsrc =
            "#version 300 es                                  \n"
            "layout(location = 0) in vec4 vPosition;          \n"
            "uniform mat4 uRotationMatrix;                    \n"
            "void main()                                      \n"
            "{                                                \n"
            "   gl_Position = uRotationMatrix * vPosition;    \n"
            "}                                                \n";

    const char *fsrc =
            "#version 300 es                              \n"
            "precision mediump float;                     \n"
            "out vec4 fragColor;                          \n"
            "void main()                                  \n"
            "{                                            \n"
            "   fragColor = vec4 (1.0, 0.2, 0.2, 1.0);    \n"
            "}                                            \n";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsrc, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsrc, NULL);
    glCompileShader(fs);

    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
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
Java_com_example_gles3_MainActivity_step(JNIEnv *env, jclass clazz) {

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

    GLuint rotationLoc = glGetUniformLocation(prog, "uRotationMatrix");

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(prog);
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, rotationMatrix);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

JNIEXPORT void JNICALL
Java_com_example_gles3_MainActivity_resize(JNIEnv *env, jclass clazz, jint width, jint height) {
    glViewport(0, 0, width, height);
}
