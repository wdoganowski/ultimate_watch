#include "pebble.h"

#include "mainwindow.h"
#include "menu.h"
#include "weatherwindow.h"

static void init(void) {
  //main_window_init();
  menu_window_init();
  weather_window_init();
}

static void deinit(void) {
  weather_window_deinit();
  menu_window_deinit();
  //main_window_deinit();
}

int main(void) {
  init();
  menu_window_show();
  app_event_loop();
  deinit();
}
