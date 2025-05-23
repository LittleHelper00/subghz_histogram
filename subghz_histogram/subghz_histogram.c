#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <subghz/subghz.h>
#include <subghz/protocols/raw.h>
#include <stdlib.h>

#define MAX_FREQS 100
#define FREQ_START 300000000
#define FREQ_END   928000000

typedef enum {
    MODE_ULTRA_NARROW = 5000,
    MODE_NARROW = 50000,
    MODE_PRECISE = 100000
} ScanMode;

typedef struct {
    uint32_t frequency;
    uint8_t count;
} FreqData;

static FreqData freq_data[MAX_FREQS];
static uint8_t freq_count = 0;
static uint32_t total_signals = 0;
static uint8_t max_count = 1;
static ScanMode current_mode = MODE_NARROW;
static const char* mode_names[] = {"Ultra-Narrow", "Narrow", "Precise"};
static uint8_t current_mode_index = 1;
static bool show_mode_overlay = false;
static uint32_t overlay_timer = 0;

void add_frequency(uint32_t freq) {
    for(uint8_t i = 0; i < freq_count; ++i) {
        if(freq_data[i].frequency == freq) {
            freq_data[i].count++;
            if(freq_data[i].count > max_count) max_count = freq_data[i].count;
            total_signals++;
            return;
        }
    }
    if(freq_count < MAX_FREQS) {
        freq_data[freq_count].frequency = freq;
        freq_data[freq_count].count = 1;
        freq_count++;
        total_signals++;
    }
}

void draw_histogram(Canvas* canvas, void* ctx) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    if(show_mode_overlay) {
        canvas_draw_str_aligned(canvas, 64, 5, AlignCenter, AlignTop, mode_names[current_mode_index]);
    }
    char total_buf[20];
    snprintf(total_buf, sizeof(total_buf), "Total: %lu", total_signals);
    canvas_draw_str_aligned(canvas, 123, 5, AlignRight, AlignTop, total_buf);

    if(freq_count == 0) return;

    uint8_t bar_width = 128 / freq_count;
    for(uint8_t i = 0; i < freq_count; ++i) {
        uint8_t height = (uint8_t)((float)freq_data[i].count / max_count * 50);
        canvas_draw_box(canvas, i * bar_width, 63 - height, bar_width - 1, height);
    }
}

void input_callback(InputEvent* event, void* ctx) {
    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        current_mode_index = (current_mode_index + 1) % 3;
        current_mode = (ScanMode)(MODE_ULTRA_NARROW + (current_mode_index * 45000));
        show_mode_overlay = true;
        overlay_timer = furi_get_tick() + 1500;
    }
}

int32_t subghz_histogram_app(void* p) {
    UNUSED(p);
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_histogram, NULL);
    view_port_input_callback_set(view_port, input_callback, NULL);
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    SubGhz* subghz = subghz_alloc();
    subghz_start(subghz);

    uint32_t freq = FREQ_START;
    while(freq <= FREQ_END) {
        subghz_set_frequency(subghz, freq);
        furi_delay_ms(80);
        SubGhzDecoder* decoder = subghz_get_decoder(subghz);
        if(subghz_decoder_received(decoder)) {
            add_frequency(freq);
            view_port_update(view_port);
        }
        if(show_mode_overlay && furi_get_tick() > overlay_timer) {
            show_mode_overlay = false;
            view_port_update(view_port);
        }
        freq += current_mode;
    }

    subghz_stop(subghz);
    subghz_free(subghz);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close("gui");

    return 0;
}