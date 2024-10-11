#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>

#include "android_native_app_glue.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "Engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

typedef struct {
    /* OpenGL */
    GLuint prog, VAO, texture;

    /* EGL */
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

} Engine;

Engine engine = {0};

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
    const char *vsrc = "#version 300 es                                  \n"
                       "layout(location = 0) in vec4 vPosition;          \n"
                       "uniform mat4 uRotationMatrix;                    \n"
                       "void main()                                      \n"
                       "{                                                \n"
                       "   gl_Position = uRotationMatrix * vPosition;    \n"
                       "}                                                \n";

    const char *fsrc = "#version 300 es                              \n"
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

    engine.prog = glCreateProgram();
    glAttachShader(engine.prog, vs);
    glAttachShader(engine.prog, fs);
    glLinkProgram(engine.prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLuint VBO;
    float vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};
    glGenVertexArrays(1, &engine.VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(engine.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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
    case APP_CMD_DESTROY:
        cleanup_opengl();
        exit(0);
        break;
    }
}

void android_main(struct android_app *app) {
    app->onAppCmd = handle_cmd;

    int events;
    struct android_poll_source *source;
    struct timespec now;

    while (1) {
        while (ALooper_pollAll(0, NULL, &events, (void **)&source) >= 0) {
            if (source != NULL) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                cleanup_opengl();
                return;
            }
        }

        static float angle = 0.0f;
        angle += 0.02f;

        float cosAngle = cosf(angle);
        float sinAngle = sinf(angle);
        GLfloat rotationMatrix[] = {cosAngle, -sinAngle, 0.0, 0.0, sinAngle, cosAngle, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
        GLuint rotationLoc = glGetUniformLocation(engine.prog, "uRotationMatrix");

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(engine.prog);
        glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, rotationMatrix);
        glBindVertexArray(engine.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        eglSwapBuffers(engine.display, engine.surface);
    }
}
