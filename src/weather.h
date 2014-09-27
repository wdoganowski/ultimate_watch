#include "pebble.h"

// Sizes 

#define CITY_W (bounds.size.w - TEMP_W)
#define CITY_H 20 
#define TEMP_W 40
#define TEMP_H 20
#define ICON_W 32
#define ICON_H 32

#define WEATHER_REFRESH_INTERVAL (30 * 60 * 1000) // 30 minutes in miliseconds
#define WEATHER_GET_INTERVAL (1 * 60 * 1000) // Used when there was a refresh error 

void weather_window_load(Window *window);
void weather_window_unload(Window *window);

void weather_init(Window *window);
void weather_deinit(void);
