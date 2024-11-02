#include <jni.h>

#include <android/native_window_jni.h>

ANativeWindow *window = NULL;

JNIEXPORT void JNICALL Java_com_example_activity_NativeSurfaceView_init(JNIEnv *env, jobject obj, jobject surface) {
    window = ANativeWindow_fromSurface(env, surface);
    if (window != NULL) {
        ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGBX_8888);
    }
}

JNIEXPORT void JNICALL Java_com_example_activity_NativeSurfaceView_step(JNIEnv *env, jobject obj, jint width, jint height) {
    if (window != NULL) {
        ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBX_8888);
        ANativeWindow_Buffer buffer;
        if (ANativeWindow_lock(window, &buffer, NULL) == 0) {
            uint32_t *pixels = (uint32_t *)buffer.bits;
            int pixelCount = buffer.width * buffer.height;

            for (int i = 0; i < pixelCount; ++i) {
                pixels[i] = 0xFF0000FF;
            }

            ANativeWindow_unlockAndPost(window);
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_activity_NativeSurfaceView_deinit(JNIEnv *env, jobject obj) {
    if (window != NULL) {
        ANativeWindow_release(window);
        window = NULL;
    }
}
