#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

#define MAX_BINS 128

static int bins[MAX_BINS] = {0};
static int bin_max = 1;
static int total_signals = 0;
static int mode = 0;

const char* mode_names[] = {"Narrow", "Ultra-Narrow", "Precise"};

void draw_histogram(Canvas* canvas, int width, int height) {
    int bar_width = width / MAX_BINS;
    for (int i = 0; i < MAX_BINS; ++i) {
        int bar_height = (bins[i] * height) / bin_max;
        canvas_draw_box(canvas, i * bar_width, height - bar_height, bar_width, bar_height);
    }
    canvas_draw_str(canvas, width - 60, 10, mode_names[mode]);
}

void input_callback(InputEvent* event, void* ctx) {
    if (event->type == InputTypeShort && event->key == InputKeyOk) {
        mode = (mode + 1) % 3;
    }
}

int32_t subghz_histogram_app(void* p) {
    UNUSED(p);
    Gui* gui = furi_record_open("gui");
    ViewPort* vp = view_port_alloc();
    view_port_draw_callback_set(vp, draw_histogram);
    view_port_input_callback_set(vp, input_callback, NULL);
    gui_add_view_port(gui, vp, GuiLayerFullscreen);

    while (1) {
        int freq_bin = rand() % MAX_BINS;
        bins[freq_bin]++;
        if (bins[freq_bin] > bin_max) bin_max = bins[freq_bin];
        total_signals++;
        view_port_update(vp);
        furi_delay_ms(250);
    }

    gui_remove_view_port(gui, vp);
    view_port_free(vp);
    furi_record_close("gui");
    return 0;
}