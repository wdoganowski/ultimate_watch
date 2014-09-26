#include "pebble.h"

#include "ultimate_watch.h"
#include "weather.h"

static TextLayer *temperature_layer;
static TextLayer *city_layer;
static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

static AppSync sync;
static uint8_t sync_buffer[64];

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
  WEATHER_CITY_KEY = 0x2,         // TUPLE_CSTRING
};

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_IMAGE_SUN, //0
  RESOURCE_ID_IMAGE_CLOUD, //1
  RESOURCE_ID_IMAGE_RAIN, //2
  RESOURCE_ID_IMAGE_SNOW //3
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case WEATHER_ICON_KEY:
      if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }
      icon_bitmap = gbitmap_create_with_resource(WEATHER_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      break;

    case WEATHER_TEMPERATURE_KEY:
      // App Sync keeps new_tuple in sync_buffer, so we may use it directly
      text_layer_set_text(temperature_layer, new_tuple->value->cstring);
      break;

    case WEATHER_CITY_KEY:
      text_layer_set_text(city_layer, new_tuple->value->cstring);
      break;
  }
}

static void weather_update_proc(void *data) {
  Tuplet value = TupletInteger(1, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();

  // set automatic update
  app_timer_register(WEATHER_REFRESH_INTERVAL, weather_update_proc, NULL);
}

void weather_window_load(Window *window) {
  if (!INC_WEATHER) return;

 /* Weather layers */
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer); 

  // icon 
  icon_layer = bitmap_layer_create(GRect(bounds.size.w - ICON_W, bounds.size.h - ICON_H - TEMP_H, ICON_W, ICON_H));
  layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));

  // temperature
  temperature_layer = text_layer_create(GRect(bounds.size.w - TEMP_W, bounds.size.h - TEMP_H, TEMP_W, TEMP_H));
  text_layer_set_text_color(temperature_layer, GColorWhite);
  text_layer_set_background_color(temperature_layer, GColorClear);
  text_layer_set_font(temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(temperature_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(temperature_layer));

  // city
  city_layer = text_layer_create(GRect(bounds.origin.x, bounds.size.h - CITY_H, CITY_W, CITY_H));
  text_layer_set_text_color(city_layer, GColorWhite);
  text_layer_set_background_color(city_layer, GColorClear);
  text_layer_set_font(city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(city_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(city_layer));

  Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 0),
    TupletCString(WEATHER_TEMPERATURE_KEY, "?\u00B0C"),
    TupletCString(WEATHER_CITY_KEY, "Location Not Set"),
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);

  weather_update_proc(NULL);

}

void weather_window_unload(Window *window) {
  if (!INC_WEATHER) return;

  app_sync_deinit(&sync);

  if (icon_bitmap) {
    gbitmap_destroy(icon_bitmap);
  }

  text_layer_destroy(city_layer);
  text_layer_destroy(temperature_layer);
  bitmap_layer_destroy(icon_layer);
}

void weather_init(Window *window) {
  if (!INC_WEATHER) return;

  const int inbound_size = 64;
  const int outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
}

void weather_deinit(void) {
  if (!INC_WEATHER) return;
}
