#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h> // Include for intptr_t

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

typedef struct AppState AppState;

typedef enum { EventTypeTick, EventTypeKey } EventType;

typedef struct { EventType type; InputEvent input; } PluginEvent;

typedef struct AppState {
    int x;
    int y;
    int shape_mode;
    bool is_filled;
    bool is_running;
} AppState;

typedef struct { FuriMutex* mutex; AppState* app; } AppContext;

static void render_callback(Canvas* const canvas, void* ctx);
static void input_callback(InputEvent* input_event, void* ctx);
void render(Canvas* canvas, AppState* app);
void input(InputKey key, InputType type, AppState* app);
static void initialize_app_state(AppState* app);
int print(const char* message);

// String utility functions
char* int_to_str(long value) { char* buffer = malloc(32); if(!buffer) return NULL; snprintf(buffer, 32, "%ld", value); return buffer; }

char* str(long value) { return int_to_str(value); }

char* str_s(const char* value) { if(!value) return strdup(""); return strdup(value); }

int range(int limit) { return limit; }

char* str_concat(const char* s1, const char* s2) { if(!s1) s1 = ""; if(!s2) s2 = ""; size_t len1 = strlen(s1); size_t len2 = strlen(s2); char* result = malloc(len1 + len2 + 1); if(!result) return NULL; strcpy(result, s1); strcat(result, s2); return result; }

int print(const char* message) { FURI_LOG_I("FlipScript", "%s", message); return 0; }

void* draw_my_box(Canvas* canvas, AppState* app) {
    if (app->is_filled) {
        canvas_draw_box(canvas, app->x, app->y, 30, 15);
    } else {
        canvas_draw_frame(canvas, app->x, app->y, 30, 15);
    }
return NULL;
}

void* draw_my_circle(Canvas* canvas, AppState* app) {
    if (app->is_filled) {
        canvas_draw_disc(canvas, app->x, app->y, 10);
    } else {
        canvas_draw_circle(canvas, app->x, app->y, 10);
    }
return NULL;
}

// User-defined main function
void user_main(AppState* app) {
    app->is_running = true;
    furi_delay_ms(50);
}

// User-defined render function
void render(Canvas* canvas, AppState* app) {
    canvas_clear(canvas);
    {
        char* text_to_draw = strdup("Shape Drawer");
        canvas_draw_str(canvas, 2, 12, text_to_draw);
        free(text_to_draw);
    }
    if ((app->shape_mode == 0)) {
        draw_my_box(canvas, app);
    } else {
        draw_my_circle(canvas, app);
    }
    {
        char* text_to_draw = strdup("L/R: Toggle Fill | U/D: Shape");
        canvas_draw_str(canvas, 2, 60, text_to_draw);
        free(text_to_draw);
    }
}

// User-defined input handler function
void input(InputKey key, InputType type, AppState* app) {
    if ((type == InputTypePress)) {
        if ((key == InputKeyUp)) {
            app->shape_mode = 1;
        } else if ((key == InputKeyDown)) {
            app->shape_mode = 0;
        } else if ((key == InputKeyLeft)) {
            app->is_filled = false;
        } else if ((key == InputKeyRight)) {
            app->is_filled = true;
        }
    }
}

static void initialize_app_state(AppState* app) {
    memset(app, 0, sizeof(AppState));
    app->x = 25;
    app->y = 25;
    app->shape_mode = 0;
    app->is_filled = false;
    app->is_running = true;
    {
        char* temp_str = strdup("FlipScript Shape Drawer Initialized");
        print(temp_str);
        free(temp_str);
    }
}

static void render_callback(Canvas* const canvas, void* ctx) {
    AppContext* context = (AppContext*)ctx; furi_mutex_acquire(context->mutex, FuriWaitForever); render(canvas, context->app); furi_mutex_release(context->mutex); 
}

static void input_callback(InputEvent* input_event, void* ctx) {
    FuriMessageQueue* event_queue = (FuriMessageQueue*)ctx; furi_assert(event_queue); PluginEvent event = {.type = EventTypeKey, .input = *input_event}; furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

int32_t app_main(void* p) {
    UNUSED(p);

    AppState* app = malloc(sizeof(AppState));
    if(!app) { FURI_LOG_E("flipscript", "Failed to allocate AppState"); return 255; }

    initialize_app_state(app);

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(PluginEvent));
    if(!event_queue) { FURI_LOG_E("flipscript", "Failed to allocate event queue"); free(app); return 255; }

    FuriMutex* app_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    if(!app_mutex) { FURI_LOG_E("flipscript", "Failed to allocate app mutex"); free(app); furi_message_queue_free(event_queue); return 255; }

    AppContext app_context = {.mutex = app_mutex, .app = app};

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, render_callback, &app_context);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    PluginEvent event;
    bool running = true;
    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            furi_mutex_acquire(app_mutex, FuriWaitForever);
            if(event.type == EventTypeKey) {
                input(event.input.key, event.input.type, app);
                if(event.input.key == InputKeyBack && event.input.type == InputTypePress) running = false;
            }
            furi_mutex_release(app_mutex);
        }
        furi_mutex_acquire(app_mutex, FuriWaitForever); user_main(app); furi_mutex_release(app_mutex);
        view_port_update(view_port);
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_mutex_free(app_mutex);
    free(app);

    return 0;
}
