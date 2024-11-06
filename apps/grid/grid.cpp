#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <android/input.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ENGINE", __VA_ARGS__))

typedef struct {
    ANativeWindow *window;
    AInputQueue *input;
    int running;
    pthread_t thread;
} AndroidApp;

static const char *vertex_shader_source = R"(#version 300 es
    precision mediump float;

    layout(location = 0) in vec2 aPos;
    uniform float offset_y;
    uniform float offset_x;

    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        gl_Position.y += offset_y;
        gl_Position.x += offset_x;
    }
)";

static const char *fragment_shader_source = R"(#version 300 es
    precision mediump float;

    out vec4 FragColor;

    void main() {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
)";

// clang-format off
static const GLfloat square_vertices[] = {
    -0.95f, +0.30f, 
    -0.05f, +0.30f, 
    -0.95f, +0.90f, 
    -0.05f, +0.90f,
};
// clang-format on

void *run_main(void *arg) {
    AndroidApp *app = (AndroidApp *)arg;

    void *egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(egl_display, NULL, NULL);
    EGLint attributes[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE};
    int confgs;
    void *egl_config;
    eglChooseConfig(egl_display, attributes, &egl_config, 1, &confgs);
    int context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    void *egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attributes);
    void *egl_surface = eglCreateWindowSurface(egl_display, egl_config, app->window, NULL);
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glUseProgram(program);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);

    GLuint offset_y_location = glGetUniformLocation(program, "offset_y");
    GLuint offset_x_location = glGetUniformLocation(program, "offset_x");

    float scroll_velocity = 0.0f;
    float scroll_offset = 0.0f;
    float last_touch_y = 0.0f;
    int is_scrolling = 0;

    AInputEvent *event = NULL;

    while (app->running) {

        while (AInputQueue_getEvent(app->input, &event) >= 0) {
            if (AInputQueue_preDispatchEvent(app->input, event)) continue;

            if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
                int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
                if (action == AMOTION_EVENT_ACTION_DOWN) {
                    last_touch_y = AMotionEvent_getY(event, 0);
                    is_scrolling = 1;
                } else if (action == AMOTION_EVENT_ACTION_MOVE && is_scrolling) {
                    float current_y = AMotionEvent_getY(event, 0);
                    scroll_velocity = (current_y - last_touch_y) * 0.001f;
                    last_touch_y = current_y;
                } else if (action == AMOTION_EVENT_ACTION_UP) {
                    is_scrolling = 0;
                }
            }

            AInputQueue_finishEvent(app->input, event, 0);
        }

        if (scroll_offset < 0.0f) scroll_offset = 0.0f;
        if (scroll_offset >= 0.0f) scroll_offset -= scroll_velocity;

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        for (float y = 0; y < 20; y += 0.7) {
            for (float x = 0.0; x <= 1.0; x += 1.0) {
                glUniform1f(offset_y_location, scroll_offset - y);
                glUniform1f(offset_x_location, x);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
        eglSwapBuffers(egl_display, egl_surface);

        scroll_velocity *= 0.985f;
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(egl_display, egl_surface);
    eglDestroyContext(egl_display, egl_context);
    eglTerminate(egl_display);

    return NULL;
}

void on_window_init(ANativeActivity *activity, ANativeWindow *window) {
    LOG("on_window_init");

    AndroidApp *app = (AndroidApp *)activity->instance;
    app->window = window;
    app->running = 1;
    pthread_create(&app->thread, NULL, run_main, app);
}

void on_window_deinit(ANativeActivity *activity, ANativeWindow *window) {
    LOG("on_window_deinit");

    AndroidApp *app = (AndroidApp *)activity->instance;
    app->running = 0;
    pthread_join(app->thread, NULL);
    app->window = NULL;
}

void on_input_init(ANativeActivity *activity, AInputQueue *input) {
    AndroidApp *app = (AndroidApp *)activity->instance;
    app->input = input;
    AInputQueue_attachLooper(input, ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS), ALOOPER_EVENT_INPUT, NULL, NULL);
}

void on_input_deinit(ANativeActivity *activity, AInputQueue *input) {
    AndroidApp *app = (AndroidApp *)activity->instance;
    AInputQueue_detachLooper(input);
    app->input = NULL;
}

JNIEXPORT void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    AndroidApp *app = (AndroidApp *)malloc(sizeof(AndroidApp));
    memset(app, 0, sizeof(AndroidApp));

    activity->callbacks->onNativeWindowCreated = on_window_init;
    activity->callbacks->onNativeWindowDestroyed = on_window_deinit;
    activity->callbacks->onInputQueueCreated = on_input_init;
    activity->callbacks->onInputQueueDestroyed = on_input_deinit;
    activity->instance = app;
}
