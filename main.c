#include <raylib.h>

#define MINIAUDIO_IMPLEMENTATION
#include "./miniaudio.h"

#define SOMUI_IMPLEMENTATION
#include "./somui.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define SAMPLE_RATE 44100
#define CHANNELS_COUNT 1

#define PURE_GREEN (Color) { 0, 255, 0, 255 }

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define clamp(v, minv, maxv) min((maxv), max((minv), (v)))

// I need to be able to have multiple sliders
// and still keep the Immediate UI style
// slider ids start at 1
// active_slider = 0 means no slider is active
size_t slider_count = 0;
size_t active_slider = 0;

typedef void (*Slider_Background_Func)(UI_Rect rect);

typedef struct {
    float value;
    size_t id;
} Slider;

Slider slider(float initial_value) {
    return (Slider) {
        .value = initial_value,
        .id = ++slider_count,
    };
}

// Who said global variables are bad?
float audio_volume = 0.0f;
float interpolated_audio_volume = 0.0f;

float lerp(float a, float b, float f) {
    return a * (1.0f - f) + (b * f);
}

void data_callback(ma_device *device, void *output, const void *input, ma_uint32 count) {
    (void) device;
    (void) output;

    const float *data = input;

    float sum = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum += data[i] * data[i];
    }

    audio_volume = sqrtf(sum / (float) count);
}

void avatar_widget(UI_Rect rect, Texture2D idle, Texture2D speaking, float volume_threshold, float scale) {
    Vector2 pos = {
        rect.x + (rect.w - (idle.width * scale)) / 2,
        rect.y + (rect.h - (idle.height * scale)) / 2,
    };

    Texture2D texture = (audio_volume >= volume_threshold) ? speaking : idle;
    DrawRectangle(rect.x, rect.y, rect.w, rect.h, PURE_GREEN);
    DrawTextureEx(texture, pos, 0, scale, WHITE);
}

void slider_widget(UI_Rect rect, Slider *slider, Slider_Background_Func func) {
    float slider_factor = 0.65f; // From 0 to 1
    float slider_width = rect.w * slider_factor;
    int tri_base_size = 12;

    int mark = rect.h - (rect.h * slider->value);

    UI_Rect slider_rect = { rect.x, rect.y, slider_width, rect.h };

    Vector2 p1 = { rect.x + slider_width, rect.y + mark };
    Vector2 p2 = { p1.x + rect.w - slider_width, p1.y - tri_base_size };
    Vector2 p3 = { p1.x + rect.w - slider_width, p1.y + tri_base_size };

    Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointTriangle(mouse, p2, p1, p3) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        active_slider = slider->id;
    }

    if (active_slider == slider->id) {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            active_slider = 0;
        }
        float new_pos = rect.h - (mouse.y - rect.y);
        new_pos = clamp(new_pos, 0, rect.h);
        slider->value = new_pos / (float) rect.h;
    }

    if (func != NULL) func(slider_rect);

    DrawTriangle(p2, p1, p3, WHITE);
    DrawLine(rect.x, rect.y + mark, rect.x + slider_width, rect.y + mark, WHITE);
    DrawRectangleLines(slider_rect.x, slider_rect.y, slider_rect.w, slider_rect.h, WHITE);
}

void audio_slider_background(UI_Rect rect) {
    // Maybe interpolating by 0.2 is too slow and not a great representation of the audio
    // but whatever, it looks good
    interpolated_audio_volume = lerp(interpolated_audio_volume, audio_volume, 0.2f);
    int volume_rect_h = rect.h * interpolated_audio_volume;
    DrawRectangle(rect.x, rect.y + rect.h - volume_rect_h, rect.w, volume_rect_h, RED);
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Quavatar");
    SetTargetFPS(60);

    ma_device device = { 0 };
    ma_device_config device_config = ma_device_config_init(ma_device_type_capture);
    device_config.capture.format = ma_format_f32;
    device_config.capture.channels = CHANNELS_COUNT;
    device_config.sampleRate = SAMPLE_RATE;
    device_config.dataCallback = data_callback;

    if (ma_device_init(NULL, &device_config, &device) != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to initialize audio capture device\n");
        exit(1);
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to start audio device\n");
        exit(1);
    }

    UI_Stack ui = { 0 };

    Texture2D idle = LoadTexture("./imgs/idle.png");
    Texture2D speaking = LoadTexture("./imgs/speaking.png");

    Slider audio_slider = slider(0.5f);
    Slider scale_slider = slider(0.5f);

    while (!WindowShouldClose()) {
        int w = GetRenderWidth();
        int h = GetRenderHeight();

        UI_Rect ui_rect = { 0, 0, w, h };
        int gap = 10;

        BeginDrawing();
        ClearBackground(BLACK);

        ui_layout_begin(&ui, ui_rect, UI_HORI, ui_marginv(gap), gap, 2);

        ui_layout_begin(&ui, ui_layout_rect(&ui), UI_HORI, ui_marginv(0), gap, 10);
        slider_widget(ui_layout_rect(&ui), &audio_slider, audio_slider_background);
        slider_widget(ui_layout_rect(&ui), &scale_slider, NULL);
        ui_layout_end(&ui);

        avatar_widget(
            ui_layout_rect(&ui),
            idle, speaking,
            audio_slider.value, scale_slider.value);
        ui_layout_end(&ui);

        EndDrawing();
    }

    ui_stack_free(&ui);
    ma_device_uninit(&device);

    UnloadTexture(idle);
    UnloadTexture(speaking);

    CloseWindow();
    return 0;
}
