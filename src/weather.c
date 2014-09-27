#include "pebble.h"

#include "ultimate_watch.h"
#include "weather.h"

static TextLayer *temperature_layer;
#ifdef SHOW_CITY
static TextLayer *city_layer;
#endif
static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

extern Layer *hands_layer;

static AppSync sync;
static uint8_t sync_buffer[64];
static AppTimer* weather_update_timer = NULL;

enum WeatherKey {
  TYPE_KEY = 0x0,         // TUPLE_INT
  WEATHER_ICON_KEY = 0x1,         // TUPLE_CSTRING
  WEATHER_TEMPERATURE_KEY = 0x2,  // TUPLE_CSTRING
  WEATHER_CITY_KEY = 0x3,         // TUPLE_CSTRING
};

enum TypeKey {
  TYPE_KEY_WEATHER = 0x0,
  TYPE_KEY_FCAST5 = 0x01,
};

typedef struct WeatherIcon {
  char icon[4];
  uint32_t resource_id;
} WeatherIcon;

#define UNKNOWN_ICON 0xFF

static const WeatherIcon weather_icons[] = {
  {"01d", RESOURCE_ID_IMAGE_16_01d}, // clear sky
  {"01n", RESOURCE_ID_IMAGE_16_01n}, 
  {"02d", RESOURCE_ID_IMAGE_16_02d}, // few clouds
  {"02n", RESOURCE_ID_IMAGE_16_02n}, 
  {"03d", RESOURCE_ID_IMAGE_16_03d}, // scattered clouds
  {"03n", RESOURCE_ID_IMAGE_16_03n}, 
  {"04d", RESOURCE_ID_IMAGE_16_04d}, // broken clouds
  {"04n", RESOURCE_ID_IMAGE_16_04n}, 
  {"09d", RESOURCE_ID_IMAGE_16_09d}, // shower rain
  {"09n", RESOURCE_ID_IMAGE_16_09n}, 
  {"10d", RESOURCE_ID_IMAGE_16_10d}, // rain
  {"10n", RESOURCE_ID_IMAGE_16_10n}, 
  {"11d", RESOURCE_ID_IMAGE_16_11d}, // thunderstorm
  {"11n", RESOURCE_ID_IMAGE_16_11n}, 
  {"13d", RESOURCE_ID_IMAGE_16_13d}, // snow
  {"13n", RESOURCE_ID_IMAGE_16_13n}, 
  {"50d", RESOURCE_ID_IMAGE_16_50d}, // mist
  {"50n", RESOURCE_ID_IMAGE_16_50n} 
};

static uint32_t find_icon_resource(const char* icon) {
  for (unsigned int i = 0; i < (sizeof(weather_icons)/sizeof(WeatherIcon)); i++)
  {
    if (strcmp(weather_icons[i].icon, icon) == 0)
    {
      return weather_icons[i].resource_id;
    }
  }
  // If not found, return UNKNOWN_ICON
  return UNKNOWN_ICON;
}

// http://stackoverflow.com/questions/21150193/logging-enums-on-the-pebble-watch/21172222#21172222
static char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d %s", app_message_error, translate_error(app_message_error));

  // Increase the update rate
  if (weather_update_timer) {
    app_timer_reschedule(weather_update_timer, WEATHER_GET_INTERVAL);
  }
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync received: %u", (unsigned int)key);

  // Set the timer to normal rate
  if (weather_update_timer) {
    app_timer_reschedule(weather_update_timer, WEATHER_REFRESH_INTERVAL);
  }

  switch (key) {
    case WEATHER_ICON_KEY:
      if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }
      uint32_t icon_resource = find_icon_resource(new_tuple->value->cstring);
      if (icon_resource != UNKNOWN_ICON) {
        icon_bitmap = gbitmap_create_with_resource(icon_resource);
      }
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      break;

    case WEATHER_TEMPERATURE_KEY:
      // App Sync keeps new_tuple in sync_buffer, so we may use it directly
      text_layer_set_text(temperature_layer, new_tuple->value->cstring);
      break;

    case WEATHER_CITY_KEY:
#ifdef SHOW_CITY
      text_layer_set_text(city_layer, new_tuple->value->cstring);
#endif
      break;

  }
}

static void weather_update_proc(void *data) {
  Tuplet value = TupletInteger(TYPE_KEY , TYPE_KEY_WEATHER);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();

  // set automatic update
  weather_update_timer = app_timer_register(WEATHER_REFRESH_INTERVAL, weather_update_proc, NULL);
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

  // city
#ifdef SHOW_CITY
  city_layer = text_layer_create(GRect(bounds.origin.x, bounds.size.h - CITY_H, CITY_W, CITY_H));
  text_layer_set_text_color(city_layer, GColorWhite);
  text_layer_set_background_color(city_layer, GColorClear);
  text_layer_set_font(city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(city_layer, GTextAlignmentLeft);
  // layer_add_child(window_layer, text_layer_get_layer(city_layer));
  // Add it below watch hands
  layer_insert_below_sibling(text_layer_get_layer(city_layer), hands_layer);
#endif

  Tuplet initial_values[] = {
    TupletCString(WEATHER_ICON_KEY, ""),
    TupletCString(WEATHER_TEMPERATURE_KEY, ""),
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

#ifdef SHOW_CITY
  text_layer_destroy(city_layer);
#endif
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
