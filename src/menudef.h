#pragma once

#include "pebble.h"
#include "menu.h"

#include "forecastwindow.h"

// Menu content

static const struct MenuInfo menu_info = {
  2, (MenuSection[]) {
    {
      3, "Information", (MenuCell[]) {
        {"Weather", "Forecast for one week", NULL, forecast_window_show},
        {"Item 2", NULL, NULL, NULL},
        {"Item 3", NULL, NULL, NULL}
      }
    },
    {
      1, "Settings", (MenuCell[]) {
        {"Last item", NULL, NULL, NULL}
      }
    }
  }
};
