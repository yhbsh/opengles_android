#include <jni.h>

#include <GLES/egl.h>
#include <GLES/gl.h>

#include <android/log.h>
#include <android/native_activity.h>

#include <pthread.h>

#include <math.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "Engine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "Engine", __VA_ARGS__))
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "Engine", __VA_ARGS__))

typedef struct {
    ANativeWindow *window;

    void *egl_display;
    void *egl_surface;
    void *egl_context;
    void *egl_config;

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

    // Choose config for GLES v2
    EGLint attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
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

    app.running = true;

    GLint width, height;
    eglQuerySurface(app.egl_display, app.egl_surface, EGL_WIDTH, &width);
    eglQuerySurface(app.egl_display, app.egl_surface, EGL_HEIGHT, &height);
    glViewport(0, 0, width, height);

    float dt = 0.0f;

    while (app.running) {
        float r = (sinf(dt + 0) * 0.5f) + 0.5f;
        float g = (cosf(dt + 0) * 0.5f) + 0.5f;
        float b = (sinf(dt + 3) * 0.5f) + 0.5f;

        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!eglSwapBuffers(app.egl_display, app.egl_surface)) {
            EGLint error = eglGetError();
            LOGE("eglSwapBuffers failed: 0x%x", error);

            if (error == EGL_BAD_SURFACE) {
                LOGE("Re-creating EGL surface due to EGL_BAD_SURFACE");

                eglDestroySurface(app.egl_display, app.egl_surface);
                app.egl_surface = eglCreateWindowSurface(app.egl_display, app.egl_config, app.window, NULL);

                if (app.egl_surface == EGL_NO_SURFACE) {
                    LOGE("Failed to re-create EGL surface");
                    app.running = false;
                    break;
                }

                eglMakeCurrent(app.egl_display, app.egl_surface, app.egl_surface, app.egl_context);
            }
        }

        dt += 0.01f;
    }

    return NULL;
}

void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *w) {
    LOGI("onNativeWindowCreated");

    (void)activity;

    app.window = w;
    if (app.window == NULL) {
        LOGE("no window available");
        return;
    }

    pthread_create(&app.thread, NULL, run_main, NULL);
}

void onNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window) {
    (void)activity;
    (void)window;
    LOGI("onNativeWindowDestroyed");

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
