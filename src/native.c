#include <jni.h>

#include <GLES/egl.h>
#include <GLES/gl.h>

#include <android/log.h>
#include <android/native_activity.h>

#include <stdlib.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "Engine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "Engine", __VA_ARGS__))
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "Engine", __VA_ARGS__))

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
void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowCreated");
}
void onNativeWindowResized(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowResized");
}
void onNativeWindowRedrawNeeded(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowRedrawNeeded");
}
void onNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowDestroyed");
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
