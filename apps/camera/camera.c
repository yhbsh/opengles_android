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
#include <unistd.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ENGINE", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "ENGINE", __VA_ARGS__))
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "ENGINE", __VA_ARGS__))

typedef struct AndroidApp {
    ANativeActivity *activity;
    ANativeWindow *window;
    bool running;
    pthread_t thread;

} AndroidApp;

AndroidApp app = {0};

void *camera_task(void *arg) {
    (void)arg;
    int ret;

    // Step 1: Create a Camera Manager
    ACameraManager *manager = ACameraManager_create();
    assert(manager != NULL && "Failed to create camera manager");

    // Step 2: Get the Camera ID List
    ACameraIdList *ids_list = NULL;
    ret = ACameraManager_getCameraIdList(manager, &ids_list);
    LOGI("ACameraManager_getCameraIdList returned: %d", ret);
    assert(ret == ACAMERA_OK && ids_list != NULL && "Failed to get camera ID list");

    // Step 3: Open the Camera Device
    ACameraDevice_StateCallbacks camera_callbacks = {0};
    ACameraDevice *device = NULL;
    ret = ACameraManager_openCamera(manager, ids_list->cameraIds[0], &camera_callbacks, &device);
    LOGI("ACameraManager_openCamera returned: %d", ret);
    assert(ret == ACAMERA_OK && device != NULL && "Failed to open camera device");

    // Step 4: Create a Capture Session Output
    ACaptureSessionOutput *capture_session_output = NULL;
    ret = ACaptureSessionOutput_create(app.window, &capture_session_output);
    LOGI("ACaptureSessionOutput_create returned: %d", ret);
    assert(ret == ACAMERA_OK && capture_session_output != NULL && "Failed to create capture session output");

    // Step 5: Create a Session Output Container and Add Output
    ACaptureSessionOutputContainer *capture_session_output_container = NULL;
    ret = ACaptureSessionOutputContainer_create(&capture_session_output_container);
    LOGI("ACaptureSessionOutputContainer_create returned: %d", ret);
    assert(ret == ACAMERA_OK && capture_session_output_container != NULL && "Failed to create session output container");

    ret = ACaptureSessionOutputContainer_add(capture_session_output_container, capture_session_output);
    LOGI("ACaptureSessionOutputContainer_add returned: %d", ret);
    assert(ret == ACAMERA_OK && "Failed to add session output");

    // Step 6: Create a Capture Request (Preview)
    ACaptureRequest *capture_request = NULL;
    ret = ACameraDevice_createCaptureRequest(device, TEMPLATE_PREVIEW, &capture_request);
    LOGI("ACameraDevice_createCaptureRequest returned: %d", ret);
    assert(ret == ACAMERA_OK && capture_request != NULL && "Failed to create capture request");

    // Step 7: Create an Output Target and Add to Capture Request
    ACameraOutputTarget *output_target = NULL;
    ret = ACameraOutputTarget_create(app.window, &output_target);
    LOGI("ACameraOutputTarget_create returned: %d", ret);
    assert(ret == ACAMERA_OK && output_target != NULL && "Failed to create output target");

    ret = ACaptureRequest_addTarget(capture_request, output_target);
    LOGI("ACaptureRequest_addTarget returned: %d", ret);
    assert(ret == ACAMERA_OK && "Failed to add target to capture request");

    // Step 8: Create the Capture Session
    ACameraCaptureSession *capture_session = NULL;
    ACameraCaptureSession_stateCallbacks session_state_callbacks = {.context = NULL, .onActive = NULL, .onReady = NULL, .onClosed = NULL};
    ret = ACameraDevice_createCaptureSession(device, capture_session_output_container, &session_state_callbacks, &capture_session);
    LOGI("ACameraDevice_createCaptureSession returned: %d", ret);
    assert(ret == ACAMERA_OK && capture_session != NULL && "Failed to create capture session");

    LOGI("Successfully created capture session");

    // Step 9: Start the Capture Session
    ret = ACameraCaptureSession_setRepeatingRequest(capture_session, NULL, 1, &capture_request, NULL);
    LOGI("ACameraCaptureSession_setRepeatingRequest returned: %d", ret);
    assert(ret == ACAMERA_OK && "Failed to start capture session");

    return NULL;
}

void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowCreated");

    (void)activity;
    app.window = window;
    app.running = true;

    pthread_create(&app.thread, NULL, camera_task, NULL);
}

void onNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window) {
    (void)activity;
    LOGI("onNativeWindowDestroyed");
    app.running = false;
    pthread_join(app.thread, NULL);
    app.window = NULL;
}

JNIEXPORT void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    (void)savedState;
    (void)savedStateSize;

    app.activity = activity;
    app.activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    app.activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
}
