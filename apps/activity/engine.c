#include <jni.h>

#include <android/native_window_jni.h>

ANativeWindow *window = NULL;

JNIEXPORT void JNICALL Java_com_example_activity_CustomView_init(JNIEnv *env, jobject obj, jobject surface) {
    window = ANativeWindow_fromSurface(env, surface);
    if (window != NULL) {
        ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGBX_8888);
    }
}

JNIEXPORT void JNICALL Java_com_example_activity_CustomView_step(JNIEnv *env, jobject obj, jint width, jint height) {
    if (window != NULL) {
        ANativeWindow_Buffer buffer;

        int ret = ANativeWindow_lock(window, &buffer, NULL);
        if (ret != 0) return;

        uint32_t *pixels = (uint32_t *)buffer.bits;

        // Loop through each pixel and fill with a YUV pattern
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                // Set Y, U, and V values with a pattern
                int Y = x * 255 / buffer.width; // Gradient in Y channel
                int U = (y % 256) - 128;        // Cyclic pattern in U channel
                int V = ((x + y) % 256) - 128;  // Combined pattern in V channel

                // Convert YUV to RGB
                int R = Y + 1.402 * V;
                int G = Y - 0.344 * U - 0.714 * V;
                int B = Y + 1.772 * U;

                // Clamp RGB values to [0, 255]
                R = R < 0 ? 0 : (R > 255 ? 255 : R);
                G = G < 0 ? 0 : (G > 255 ? 255 : G);
                B = B < 0 ? 0 : (B > 255 ? 255 : B);

                // Pack RGB values into the pixel buffer as ARGB (0xAARRGGBB)
                pixels[y * buffer.width + x] = (0xFF << 24) | (R << 16) | (G << 8) | B;
            }
        }

        ANativeWindow_unlockAndPost(window);
    }
}

JNIEXPORT void JNICALL Java_com_example_activity_CustomView_deinit(JNIEnv *env, jobject obj) {
    if (window != NULL) {
        ANativeWindow_release(window);
        window = NULL;
    }
}
