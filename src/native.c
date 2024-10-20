#include <jni.h>

#include <GLES/egl.h>
#include <GLES/gl.h>

#include <android/log.h>
#include <android/native_activity.h>

#include <pthread.h>

#include <math.h>

#include <stdbool.h>
#include <stdlib.h>

#include <unistd.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "Engine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "Engine", __VA_ARGS__))
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "Engine", __VA_ARGS__))

bool isRendering = false;
pthread_t thread;

ANativeWindow *nativeWindow = NULL;

void *render_loop(void *arg) {
    ANativeActivity *activity = (ANativeActivity *)arg;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, NULL, NULL);

    EGLint attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);

    EGLSurface surface = eglCreateWindowSurface(display, config, nativeWindow, NULL);
    eglMakeCurrent(display, surface, surface, context);

    GLint width, height;
    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    float dt = 0.0f;

    while (isRendering) {
        // Use sine waves to smoothly transition the background color
        float red = (sinf(dt) * 0.5f) + 0.5f;             // Range [0, 1]
        float green = (cosf(dt) * 0.5f) + 0.5f;           // Range [0, 1]
        float blue = (sinf(dt + 3.14159f) * 0.5f) + 0.5f; // Range [0, 1]

        // Set the background color using these smoothly changing values
        glClearColor(red, green, blue, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        eglSwapBuffers(display, surface);
        usleep(8000); // ~60 FPS

        // Increment time
        dt += 0.01f;
    }

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);

    return NULL;
}

void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowCreated");

    nativeWindow = window;

    isRendering = true;
    pthread_create(&thread, NULL, render_loop, activity);
}

void onNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowDestroyed");

    isRendering = false;
    pthread_join(thread, NULL);
    nativeWindow = NULL;
}

void onStart(ANativeActivity *activity) {
    LOGI("onStart");
}

void onResume(ANativeActivity *activity) {
    LOGI("onResume");
}

void onDestroy(ANativeActivity *activity) {
    LOGI("onDestroy");
}

void *onSaveInstanceState(ANativeActivity *activity, size_t *outSize) {
    LOGI("onSaveInstanceState");
    return NULL;
}

void onPause(ANativeActivity *activity) {
    LOGI("onPause");
}

void onStop(ANativeActivity *activity) {
    LOGI("onStop");
}

void onWindowFocusChanged(ANativeActivity *activity, int hasFocus) {
    LOGI("onWindowFocusChanged");
}

void onNativeWindowResized(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowResized");
}

void onNativeWindowRedrawNeeded(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowRedrawNeeded");
}

void onInputQueueCreated(ANativeActivity *activity, AInputQueue *queue) {
    LOGI("onInputQueueCreated");
}

void onInputQueueDestroyed(ANativeActivity *activity, AInputQueue *queue) {
    LOGI("onInputQueueDestroyed");
}

void onContentRectChanged(ANativeActivity *activity, const ARect *rect) {
    LOGI("onContentRectChanged");
}

void onConfigurationChanged(ANativeActivity *activity) {
    LOGI("onConfigurationChanged");
}

void onLowMemory(ANativeActivity *activity) {
    LOGI("onLowMemory");
}

JNIEXPORT void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    activity->callbacks->onDestroy = onDestroy;
    activity->callbacks->onStart = onStart;
    activity->callbacks->onResume = onResume;
    activity->callbacks->onSaveInstanceState = onSaveInstanceState;
    activity->callbacks->onPause = onPause;
    activity->callbacks->onStop = onStop;
    activity->callbacks->onConfigurationChanged = onConfigurationChanged;
    activity->callbacks->onLowMemory = onLowMemory;
    activity->callbacks->onWindowFocusChanged = onWindowFocusChanged;
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    activity->instance = NULL;
}
