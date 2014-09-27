#include "pebble.h"

#define FCAST_ICON_H 32
#define FCAST_ICON_W 32

#define FORECAST_REFRESH_INTERVAL (30 * 60 * 1000) // 30 minutes in miliseconds
#define FORECAST_GET_INTERVAL (10 * 1000) // Used when there was a refresh error 

void forecast_window_show(void);
void forecast_window_init(void);
void forecast_window_deinit(void);