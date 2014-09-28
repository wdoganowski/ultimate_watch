#include "pebble.h"

#include "mainwindow.h"
#include "menu.h"
#include "forecastwindow.h"
#include "openweather.h"

static void init(void) {
  main_window_init();
  menu_window_init();
  forecast_window_init();
  openweather_init();
}

static void deinit(void) {
  openweather_deinit();
  forecast_window_deinit();
  menu_window_deinit();
  main_window_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
