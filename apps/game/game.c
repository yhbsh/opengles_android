#include <jni.h>

#include <GLES/egl.h>
#include <GLES3/gl3.h>

#include <android/asset_manager.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>

#include <math.h>
#include <pthread.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ENGINE", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "ENGINE", __VA_ARGS__))
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "ENGINE", __VA_ARGS__))

typedef struct {
    ANativeWindow *window;
    ANativeActivity *activity;

    /* EGL */
    void *egl_display;
    void *egl_surface;
    void *egl_context;
    void *egl_config;

    bool running;
    pthread_t thread;
} AndroidApp;

AndroidApp app = {0};

const char *load_shader(const char *file_path) {
    AAsset *asset = AAssetManager_open(app.activity->assetManager, file_path, AASSET_MODE_BUFFER);
    if (!asset) {
        return NULL;
    }

    off_t asset_length = AAsset_getLength(asset);

    char *shader_code = (char *)malloc(asset_length + 1);
    if (!shader_code) {
        AAsset_close(asset);
        return NULL;
    }

    AAsset_read(asset, shader_code, asset_length);

    shader_code[asset_length] = '\0';

    AAsset_close(asset);

    return shader_code;
}

void createRotationMatrixX(float angle, float *matrix) {
    float cosTheta = cosf(angle);
    float sinTheta = sinf(angle);

    // clang-format off
    matrix[0] = 1.0f;  matrix[4] = 0.0f;     matrix[8]  = 0.0f;     matrix[12] = 0.0f;
    matrix[1] = 0.0f;  matrix[5] = cosTheta; matrix[9]  = -sinTheta; matrix[13] = 0.0f;
    matrix[2] = 0.0f;  matrix[6] = sinTheta; matrix[10] = cosTheta; matrix[14] = 0.0f;
    matrix[3] = 0.0f;  matrix[7] = 0.0f;     matrix[11] = 0.0f;     matrix[15] = 1.0f;
    // clang-format on
}

void createRotationMatrixY(float angle, float *matrix) {
    float cosTheta = cosf(angle);
    float sinTheta = sinf(angle);

    // clang-format off
    matrix[0] = cosTheta;  matrix[4] = 0.0f; matrix[8]  = sinTheta;  matrix[12] = 0.0f;
    matrix[1] = 0.0f;      matrix[5] = 1.0f; matrix[9]  = 0.0f;      matrix[13] = 0.0f;
    matrix[2] = -sinTheta; matrix[6] = 0.0f; matrix[10] = cosTheta;  matrix[14] = 0.0f;
    matrix[3] = 0.0f;      matrix[7] = 0.0f; matrix[11] = 0.0f;      matrix[15] = 1.0f;
    // clang-format on
}

void createRotationMatrixZ(float angle, float *matrix) {
    float cosTheta = cosf(angle);
    float sinTheta = sinf(angle);

    // clang-format off
    matrix[0] = cosTheta; matrix[4] = -sinTheta; matrix[8]  = 0.0f; matrix[12] = 0.0f;
    matrix[1] = sinTheta; matrix[5] = cosTheta;  matrix[9]  = 0.0f; matrix[13] = 0.0f;
    matrix[2] = 0.0f;     matrix[6] = 0.0f;      matrix[10] = 1.0f; matrix[14] = 0.0f;
    matrix[3] = 0.0f;     matrix[7] = 0.0f;      matrix[11] = 0.0f; matrix[15] = 1.0f;
    // clang-format on
}

void *render_task(void *arg) {
    (void)arg;

    app.egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (app.egl_display == EGL_NO_DISPLAY) {
        LOGE("cannot get EGL display");
        exit(0);
    }

    if (!eglInitialize(app.egl_display, NULL, NULL)) {
        LOGE("cannot initialize EGL");
        exit(0);
    }

    EGLint attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE};
    EGLint numConfigs;
    if (!eglChooseConfig(app.egl_display, attribs, &app.egl_config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("cannot choose EGL config");
        eglTerminate(app.egl_display);
        exit(0);
    }

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    app.egl_context = eglCreateContext(app.egl_display, app.egl_config, EGL_NO_CONTEXT, contextAttribs);
    if (app.egl_context == EGL_NO_CONTEXT) {
        LOGE("cannot create EGL context");
        eglTerminate(app.egl_display);
        exit(0);
    }

    app.egl_surface = eglCreateWindowSurface(app.egl_display, app.egl_config, app.window, NULL);
    if (app.egl_surface == EGL_NO_SURFACE) {
        LOGE("cannot create EGL window surface");
        eglDestroyContext(app.egl_display, app.egl_context);
        eglTerminate(app.egl_display);
        exit(0);
    }

    if (!eglMakeCurrent(app.egl_display, app.egl_surface, app.egl_surface, app.egl_context)) {
        LOGE("cannot make EGL context current");
        eglDestroySurface(app.egl_display, app.egl_surface);
        eglDestroyContext(app.egl_display, app.egl_context);
        eglTerminate(app.egl_display);
        exit(0);
    }

    const char *vertex_shader_source = load_shader("vert.glsl");
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader_source, NULL);
    glCompileShader(vs);

    const char *fragment_shader_source = load_shader("frag.glsl");
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader_source, NULL);
    glCompileShader(fs);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    // clang-format off
    float vertices[] = {
        -0.5f, -0.5f, +0.0f, +1.0f, +0.0f, +0.0f,
        +0.5f, -0.5f, +0.0f, +0.0f, +1.0f, +0.0f,
        +0.0f, +0.5f, +0.0f, +0.0f, +0.0f, +1.0f,
    };
    // clang-format on

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float angle = 0.0f;
    float speed = 0.0005f;

    float rotationX[16];
    float rotationY[16];
    float rotationZ[16];

    while (app.running) {
        glClearColor(0.1f, 0.6f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        angle += speed;
        if (angle > 360.0f) angle -= 360.0f;

        float interpolatedAngle = sinf(angle) * 60.0f;

        createRotationMatrixX(interpolatedAngle, rotationX);
        createRotationMatrixY(interpolatedAngle, rotationY);
        createRotationMatrixZ(interpolatedAngle, rotationZ);

        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "rotationX"), 1, GL_FALSE, rotationX);
        glUniformMatrix4fv(glGetUniformLocation(program, "rotationY"), 1, GL_FALSE, rotationY);
        glUniformMatrix4fv(glGetUniformLocation(program, "rotationZ"), 1, GL_FALSE, rotationZ);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        eglSwapBuffers(app.egl_display, app.egl_surface);
    }
    return NULL;
}

void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowCreated");

    (void)activity;

    app.window = window;
    app.running = true;

    pthread_create(&app.thread, NULL, render_task, NULL);
}

void onNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window) {
    (void)activity;
    (void)window;
    LOGI("onNativeWindowDestroyed");

    app.running = false;
    pthread_join(app.thread, NULL);

    if (!eglMakeCurrent(app.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) LOGE("cannot unbind EGL context");
    if (!eglDestroySurface(app.egl_display, app.egl_surface)) LOGE("cannot destroy EGL surface");
    if (!eglDestroyContext(app.egl_display, app.egl_context)) LOGE("cannot destroy EGL context");
    if (!eglTerminate(app.egl_display)) LOGE("cannot terminate EGL");

    app.egl_context = NULL;
    app.egl_display = NULL;
    app.egl_surface = NULL;
    app.window = NULL;
}

void onNativeWindowResized(ANativeActivity *activity, ANativeWindow *window) {
    (void)activity;
    (void)window;
    LOGI("onNativeWindowResized");

    if (app.egl_display != EGL_NO_DISPLAY && app.egl_surface != EGL_NO_SURFACE) {
        eglMakeCurrent(app.egl_display, app.egl_surface, app.egl_surface, app.egl_context);
    }
}

JNIEXPORT void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    (void)savedState;
    (void)savedStateSize;

    app.activity = activity;
    app.activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    app.activity->callbacks->onNativeWindowResized = onNativeWindowResized;
    app.activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
}
