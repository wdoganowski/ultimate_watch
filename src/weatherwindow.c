#include "pebble.h"

#include "weatherwindow.h"

static Window *window;

static void window_load(Window *window) {
}

static void window_unload(Window *window) {
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void config_provider(void *context) {
  //window_single_click_subscribe(BUTTON_ID_BACK, click_handler);
  //window_single_click_subscribe(BUTTON_ID_UP, click_handler);
  //window_single_click_subscribe(BUTTON_ID_SELECT, click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

void weather_window_show(void) {
  window_stack_push(window, true /* animated */);
}

void weather_window_init(void) {
  // Main window
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_fullscreen(window, true);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  // Init main window

  // Set key handler
  window_set_click_config_provider(window, config_provider);
}

void weather_window_deinit(void) {
  window_destroy(window);
}
