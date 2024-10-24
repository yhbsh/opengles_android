#include <jni.h>

#include <GLES/egl.h>
#include <GLES/gl.h>

#include <aaudio/AAudio.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <android/log.h>
#include <android/native_activity.h>

#include <pthread.h>

#include <math.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#include <unistd.h>

#define LOG(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ENGINE", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "ENGINE", __VA_ARGS__))
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "ENGINE", __VA_ARGS__))

typedef struct {
    ANativeActivity *activity;
    ANativeWindow *window;

    void *egl_display;
    void *egl_surface;
    void *egl_context;
    void *egl_config;

    bool running;
    bool is_playing_audio;
    pthread_t thread;
    pthread_t audio_thread;
} AndroidApp;

AndroidApp app = {0};

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

    EGLint attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
    EGLint numConfigs;
    if (!eglChooseConfig(app.egl_display, attribs, &app.egl_config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("cannot choose EGL config");
        eglTerminate(app.egl_display);
        exit(0);
    }

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE}; // GLES 2.0
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

void *audio_playback_task(void *arg) {
    (void)arg;

    app.is_playing_audio = true;
    AAsset *asset = AAssetManager_open(app.activity->assetManager, "file.wav", AASSET_MODE_UNKNOWN);
    if (!asset) {
        LOGE("cannot load file.wav");
        exit(0);
    }

    int64_t size = AAsset_getLength(asset);
    uint8_t *buffer = (uint8_t *)malloc(size);

    if (AAsset_read(asset, buffer, size) < 0) {
        LOGE("cannot read file.wav");
        exit(0);
    }

    ma_result ret;
    ma_decoder decoder;
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);

    if ((ret = ma_decoder_init_memory(buffer, size, &config, &decoder)) != MA_SUCCESS) {
        LOGE("cannot initialize decoder %s", ma_result_description(ret));
        free(buffer);
        AAsset_close(asset);
        exit(0);
    }

    AAudioStream *stream = NULL;
    AAudioStreamBuilder *builder;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("cannot create stream builder: %s", AAudio_convertResultToText(result));
        exit(0);
    }

    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(builder, 2);
    AAudioStreamBuilder_setSampleRate(builder, 44100);

    result = AAudioStreamBuilder_openStream(builder, &stream);
    if (result != AAUDIO_OK) {
        LOGE("cannot open stream: %s", AAudio_convertResultToText(result));
        AAudioStreamBuilder_delete(builder);
        exit(0);
    }

    result = AAudioStream_requestStart(stream);
    if (result != AAUDIO_OK) {
        LOGE("cannot start stream: %s", AAudio_convertResultToText(result));
        AAudioStream_close(stream);
        AAudioStreamBuilder_delete(builder);
        exit(0);
    }

    float audio_buffer[1024 * 2];
    ma_uint64 frames_read;

    while (app.is_playing_audio) {
        ret = ma_decoder_read_pcm_frames(&decoder, audio_buffer, 1024, &frames_read);
        if (ret == MA_AT_END) {
            if ((ret = ma_decoder_seek_to_pcm_frame(&decoder, 0)) != MA_SUCCESS) {
                LOGE("cannot reset decoder: %s", ma_result_description(ret));
                break;
            }
            continue;
        }

        if (ret != MA_SUCCESS) {
            LOGE("cannot read PCM frames: %s", ma_result_description(ret));
            break;
        }

        result = AAudioStream_write(stream, audio_buffer, frames_read, INT64_MAX);
        if (result < 0) {
            LOGE("cannot write stream: %s", AAudio_convertResultToText(result));
            break;
        }
    }

    AAudioStream_requestStop(stream);
    AAudioStream_close(stream);
    AAudioStreamBuilder_delete(builder);
    ma_decoder_uninit(&decoder);
    free(buffer);
    AAsset_close(asset);

    return NULL;
}

void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *w) {
    LOG("onNativeWindowCreated");

    (void)activity;

    app.window = w;
    if (app.window == NULL) {
        LOGE("no window available");
        return;
    }

    app.running = true;

    pthread_create(&app.thread, NULL, render_task, NULL);
    if (!app.is_playing_audio) {
        pthread_create(&app.audio_thread, NULL, audio_playback_task, NULL);
    }
}

void onNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window) {
    (void)activity;
    (void)window;
    LOG("onNativeWindowDestroyed");

    app.running = false;
    pthread_join(app.thread, NULL);

    if (!eglMakeCurrent(app.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
        LOGE("cannot unbind EGL context");
    }

    if (!eglDestroySurface(app.egl_display, app.egl_surface)) {
        LOGE("cannot destroy EGL surface");
    }

    if (!eglDestroyContext(app.egl_display, app.egl_context)) {
        LOGE("cannot destroy EGL context");
    }

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

    app.activity = activity;
    app.activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    app.activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
}
