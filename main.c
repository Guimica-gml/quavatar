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

void avatar_widget(UI_Rect rect, Texture2D idle, Texture2D speaking, float scale) {
    (void) speaking;

    Vector2 pos = {
        rect.x + (rect.w - (idle.width * scale)) / 2,
        rect.y + (rect.h - (idle.height * scale)) / 2,
    };

    DrawRectangle(rect.x, rect.y, rect.w, rect.h, PURE_GREEN);
    DrawTextureEx(idle, pos, 0, scale, WHITE);
}

void audio_widget(UI_Rect rect) {
    // Maybe interpolating by 0.2 is too slow and not a great representation of the audio
    // but whatever, it looks good
    interpolated_audio_volume = lerp(interpolated_audio_volume, audio_volume, 0.2f);
    int volume_rect_h = rect.h * interpolated_audio_volume;
    DrawRectangle(rect.x, rect.y + rect.h - volume_rect_h, rect.w, volume_rect_h, RED);
    DrawRectangleLines(rect.x, rect.y, rect.w, rect.h, WHITE);
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

    float scale = 0.5f;

    while (!WindowShouldClose()) {
        int w = GetRenderWidth();
        int h = GetRenderHeight();

        UI_Rect ui_rect = { 0, 0, w, h };
        int gap = 10;

        BeginDrawing();
        ClearBackground(BLACK);

        ui_layout_begin(&ui, ui_rect, UI_HORI, ui_marginv(gap), gap, 2);

        ui_layout_begin(&ui, ui_layout_rect(&ui), UI_HORI, ui_marginv(0), gap, 8);
        audio_widget(ui_layout_rect(&ui));
        ui_layout_end(&ui);

        avatar_widget(ui_layout_rect(&ui), idle, speaking, scale);
        ui_layout_end(&ui);

        EndDrawing();
    }

    ui_stack_free(&ui);
    ma_device_uninit(&device);

    CloseWindow();
    return 0;
}
