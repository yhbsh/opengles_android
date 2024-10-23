#include <jni.h>

#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadataTags.h>
#include <camera/NdkCaptureRequest.h>

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ENGINE", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "ENGINE", __VA_ARGS__))
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "ENGINE", __VA_ARGS__))

typedef struct {
    ACameraManager *manager;
    ACameraDevice *device;
    ACameraCaptureSession *captureSession;
    ACaptureRequest *captureRequest;
    ACameraOutputTarget *outputTarget;
    ACaptureSessionOutput *sessionOutput;
    ACaptureSessionOutputContainer *outputContainer;
} Camera;

typedef struct AndroidApp {
    ANativeWindow *window;
    bool running;
    pthread_t thread;
} AndroidApp;

void *camera_task(void *arg) {
    int ret;

    AndroidApp *app = (AndroidApp *)arg;

    Camera camera = {0};
    camera.manager = ACameraManager_create();
    assert(camera.manager && "Failed to create camera manager");

    // 1. list all available cameras
    ACameraIdList *cameraIdList = NULL;
    ret = ACameraManager_getCameraIdList(camera.manager, &cameraIdList);
    assert(ret == ACAMERA_OK && cameraIdList && "Failed to get camera ID list");

    // 2. open the first available camera
    ACameraDevice_StateCallbacks camera_state_callbacks = {0};
    ret = ACameraManager_openCamera(camera.manager, cameraIdList->cameraIds[0], &camera_state_callbacks, &camera.device);
    assert(ret == ACAMERA_OK && camera.device && "Failed to open camera device");

    // 3. create capture session output
    ret = ACaptureSessionOutput_create(app->window, &camera.sessionOutput);
    assert(ret == ACAMERA_OK && camera.sessionOutput && "Failed to create session output");

    // 4. create output container and add session output
    ret = ACaptureSessionOutputContainer_create(&camera.outputContainer);
    assert(ret == ACAMERA_OK && camera.outputContainer && "Failed to create output container");

    ret = ACaptureSessionOutputContainer_add(camera.outputContainer, camera.sessionOutput);
    assert(ret == ACAMERA_OK && "Failed to add session output to container");

    // 5. create capture request
    ret = ACameraDevice_createCaptureRequest(camera.device, TEMPLATE_PREVIEW, &camera.captureRequest);
    assert(ret == ACAMERA_OK && camera.captureRequest && "Failed to create capture request");

    // 6. create output target and add to capture request
    ret = ACameraOutputTarget_create(app->window, &camera.outputTarget);
    assert(ret == ACAMERA_OK && camera.outputTarget && "Failed to create output target");

    ret = ACaptureRequest_addTarget(camera.captureRequest, camera.outputTarget);
    assert(ret == ACAMERA_OK && "Failed to add target to capture request");

    // 7. create capture session
    const ACameraCaptureSession_stateCallbacks capture_session_callbacks = {0};
    ret = ACameraDevice_createCaptureSession(camera.device, camera.outputContainer, &capture_session_callbacks, &camera.captureSession);
    assert(ret == ACAMERA_OK && camera.captureSession && "Failed to create capture session");

    // 8. start capture session
    ret = ACameraCaptureSession_setRepeatingRequest(camera.captureSession, NULL, 1, &camera.captureRequest, NULL);
    assert(ret == ACAMERA_OK && "Failed to start capture session");

    while (app->running) {
        // nothing
    }

    if (cameraIdList) {
        ACameraManager_deleteCameraIdList(cameraIdList);
    }

    if (camera.captureSession) {
        ACameraCaptureSession_stopRepeating(camera.captureSession);
        ACameraCaptureSession_close(camera.captureSession);
    }
    if (camera.captureRequest) {
        ACaptureRequest_free(camera.captureRequest);
    }
    if (camera.outputTarget) {
        ACameraOutputTarget_free(camera.outputTarget);
    }
    if (camera.sessionOutput) {
        ACaptureSessionOutput_free(camera.sessionOutput);
    }
    if (camera.outputContainer) {
        ACaptureSessionOutputContainer_free(camera.outputContainer);
    }
    if (camera.device) {
        ACameraDevice_close(camera.device);
    }
    if (camera.manager) {
        ACameraManager_delete(camera.manager);
    }

    return NULL;
}

void on_window_init(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowCreated");

    AndroidApp *app = (AndroidApp *)activity->instance;

    app->window = window;
    app->running = true;

    pthread_create(&app->thread, NULL, camera_task, app);
}

void on_window_deinit(ANativeActivity *activity, ANativeWindow *window) {
    (void)window;
    LOGI("onNativeWindowDestroyed");

    AndroidApp *app = (AndroidApp *)activity->instance;

    app->running = false;

    pthread_join(app->thread, NULL);

    app->window = NULL;
}

JNIEXPORT void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    (void)savedState;
    (void)savedStateSize;

    AndroidApp *app = malloc(sizeof(AndroidApp));
    memset(app, 0, sizeof(AndroidApp));

    activity->callbacks->onNativeWindowCreated = on_window_init;
    activity->callbacks->onNativeWindowDestroyed = on_window_deinit;
    activity->instance = app;
}
