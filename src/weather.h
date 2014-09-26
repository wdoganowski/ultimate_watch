#include "pebble.h"

// Sizes 
#define DATE_DAY_W 27
#define DATE_DAY_H 20
#define DATE_NUM_W 18
#define DATE_NUM_H 20
#define CITY_W (bounds.size.w - TEMP_W)
#define CITY_H 20 
#define TEMP_W 40
#define TEMP_H 20
#define ICON_W 30
#define ICON_H 30

#define WEATHER_REFRESH_INTERVAL (30 * 60 * 1000) // 30 minutes in miliseconds

void weather_window_load(Window *window);
void weather_window_unload(Window *window);

void weather_init(Window *window);
void weather_deinit(void);
