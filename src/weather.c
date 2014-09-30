#include "pebble.h"

#define DEBUG_ON
#include "debug.h"

#include "ultimate_watch.h"
#include "weather.h"
#include "openweather.h"

static OpenweatherCallback weather_callback = NULL;

static char temperature[6]; // "-99oC"
static TextLayer *temperature_layer;
static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

extern Layer *hands_layer;

static void weather_update_proc(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Wetaher callback received");

  if (forecast_data.day && forecast_data.cnt != 0) {

    // Icon
    if (icon_bitmap) {
      gbitmap_destroy(icon_bitmap);
    }
    uint32_t icon_resource = find_icon_resource(weather_icons_16, forecast_data.day->weather.icon);
    icon_bitmap = gbitmap_create_with_resource(icon_resource);
    bitmap_layer_set_bitmap(icon_layer, icon_bitmap);

    // Temperature
    snprintf(temperature, sizeof(temperature), "%s\u00B0C", forecast_data.day->temp.day);
    text_layer_set_text(temperature_layer, temperature);
  } else LOG_DEBUG("Weather data not available yet");
}

void weather_window_load(Window *window) {
  if (!INC_WEATHER) return;

 /* Weather layers */
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer); 

  // icon 
  icon_layer = bitmap_layer_create(GRect((bounds.size.w / 2) - ICON_W - 5, (bounds.size.h / 2) + 23 , ICON_W, ICON_H));
  //bitmap_layer_set_compositing_mode(icon_layer, GCompOpAssignInverted);
  // layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));
  // Add it below watch hands
  layer_insert_below_sibling(bitmap_layer_get_layer(icon_layer), hands_layer);
 
  // temperature
  temperature_layer = text_layer_create(GRect((bounds.size.w / 2), (bounds.size.h / 2) + 27, TEMP_W, TEMP_H));
  text_layer_set_text_color(temperature_layer, GColorWhite);
  text_layer_set_background_color(temperature_layer, GColorClear);
  text_layer_set_font(temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(temperature_layer, GTextAlignmentLeft);
  // layer_add_child(window_layer, text_layer_get_layer(temperature_layer));
  // Add it below watch hands
  layer_insert_below_sibling(text_layer_get_layer(temperature_layer), hands_layer);

  weather_callback = openweather_register_callback(weather_update_proc);
}

void weather_window_unload(Window *window) {
  if (!INC_WEATHER) return;

  if (weather_callback) openweather_deregister_callback(weather_callback);

  if (icon_bitmap) {
    gbitmap_destroy(icon_bitmap);
  }

  text_layer_destroy(temperature_layer);
  bitmap_layer_destroy(icon_layer);
}

void weather_init(Window *window) {
  if (!INC_WEATHER) return;
}

void weather_deinit(void) {
  if (!INC_WEATHER) return;
}
