#include "pebble.h"

#include "forecastwindow.h"

static Window *window;

static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;
static TextLayer *td_layer;
static TextLayer *desc_layer;

static AppSync sync;
static uint8_t sync_buffer[64];
static AppTimer* forecast_update_timer = NULL;

static int day = 0; // day 0 = today
static char td_label_buff[11];


static const char * fcast_day[5] = {
  "Today", "Tommorow", "", "", ""
};

enum ForecastKey {
  TYPE_KEY = 0x0, // TUPLE_INT
  FCAST_DT_KEY = 0x4,    // TUPLE_INT
  FCAST_ICON_KEY = 0x5,  // TUPLE_CSTRING
  FCAST_DESC_KEY = 0x6,  // TUPLE_CSTRING
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
  {"01d", RESOURCE_ID_IMAGE_32_01d}, // clear sky
  {"01n", RESOURCE_ID_IMAGE_32_01n}, 
  {"02d", RESOURCE_ID_IMAGE_32_02d}, // few clouds
  {"02n", RESOURCE_ID_IMAGE_32_02n}, 
  {"03d", RESOURCE_ID_IMAGE_32_03d}, // scattered clouds
  {"03n", RESOURCE_ID_IMAGE_32_03n}, 
  {"04d", RESOURCE_ID_IMAGE_32_04d}, // broken clouds
  {"04n", RESOURCE_ID_IMAGE_32_04n}, 
  {"09d", RESOURCE_ID_IMAGE_32_09d}, // shower rain
  {"09n", RESOURCE_ID_IMAGE_32_09n}, 
  {"10d", RESOURCE_ID_IMAGE_32_10d}, // rain
  {"10n", RESOURCE_ID_IMAGE_32_10n}, 
  {"11d", RESOURCE_ID_IMAGE_32_11d}, // thunderstorm
  {"11n", RESOURCE_ID_IMAGE_32_11n}, 
  {"13d", RESOURCE_ID_IMAGE_32_13d}, // snow
  {"13n", RESOURCE_ID_IMAGE_32_13n}, 
  {"50d", RESOURCE_ID_IMAGE_32_50d}, // mist
  {"50n", RESOURCE_ID_IMAGE_32_50n} 
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
  if (forecast_update_timer) {
    app_timer_reschedule(forecast_update_timer, FORECAST_GET_INTERVAL);
  }
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync received: %u", (unsigned int)key);

  // Set the timer to normal rate
  if (forecast_update_timer) {
    app_timer_reschedule(forecast_update_timer, FORECAST_REFRESH_INTERVAL);
  }

  switch (key) {
    case FCAST_DT_KEY: {

      strcpy(td_label_buff, fcast_day[day]);
      if (!strlen(td_label_buff) && new_tuple->value->int32) {
        strftime(td_label_buff, sizeof(td_label_buff), "%a %d %b", localtime((time_t *)&new_tuple->value->uint32));
      }
      APP_LOG(APP_LOG_LEVEL_DEBUG, "DT: %d <%s>", (int)new_tuple->value->int32, td_label_buff);
      text_layer_set_text(td_layer, td_label_buff);
      break;
    }
    
    case FCAST_ICON_KEY:
      if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }
      uint32_t icon_resource = find_icon_resource(new_tuple->value->cstring);
      if (icon_resource != UNKNOWN_ICON) {
        icon_bitmap = gbitmap_create_with_resource(icon_resource);
      }
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      break;

    case FCAST_DESC_KEY:
      // App Sync keeps new_tuple in sync_buffer, so we may use it directly
      text_layer_set_text(desc_layer, new_tuple->value->cstring);
      break;
  }
}

static void forecast_update_proc(void *data) {
  Tuplet value = TupletInteger(TYPE_KEY , TYPE_KEY_FCAST5 + day);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();

  // set automatic update
  forecast_update_timer = app_timer_register(FORECAST_REFRESH_INTERVAL, forecast_update_proc, NULL);
}

static void click_handler(ClickRecognizerRef recognizer, void *context) {
  switch(click_recognizer_get_button_id(recognizer)) {
    case BUTTON_ID_UP: {
      if (day < 4) day++;
      else day = 0;
      forecast_update_proc(NULL);
      break;
    }
    case BUTTON_ID_SELECT: {
      break;
    }
    case BUTTON_ID_DOWN: {
      if (day > 0) day--;
      else day = 4;
      forecast_update_proc(NULL);
      break;
    }
    case NUM_BUTTONS: {
      break;
    }
    case BUTTON_ID_BACK: {
      break;
    }
  }
}

static void config_provider(void *context) {
  //window_single_click_subscribe(BUTTON_ID_BACK, click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, click_handler);
  //window_single_click_subscribe(BUTTON_ID_SELECT, click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, click_handler);
}

static void window_load(Window *window) {
 // Weather layers
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer); 

  // label
  td_layer = text_layer_create(GRect(0, 0, bounds.size.w, 20));
  text_layer_set_text_color(td_layer, GColorWhite);
  text_layer_set_text_color(td_layer, GColorWhite);
  text_layer_set_background_color(td_layer, GColorClear);  
  text_layer_set_font(td_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(td_layer, fcast_day[day]);
  layer_add_child(window_layer, text_layer_get_layer(td_layer));

  // icon 
  icon_layer = bitmap_layer_create(GRect(0, 20, FCAST_ICON_W, FCAST_ICON_H));
  layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));
  icon_bitmap = gbitmap_create_with_resource(weather_icons[day].resource_id);
  bitmap_layer_set_bitmap(icon_layer, icon_bitmap);

  // description 
  desc_layer = text_layer_create(GRect(FCAST_ICON_W + 4, 24, bounds.size.w - (FCAST_ICON_W + 4), 18));
  text_layer_set_text_color(desc_layer, GColorWhite);
  text_layer_set_text_color(desc_layer, GColorWhite);
  text_layer_set_background_color(desc_layer, GColorClear);  
  text_layer_set_font(desc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(desc_layer, "Waiting for data");
  layer_add_child(window_layer, text_layer_get_layer(desc_layer));

  Tuplet initial_values[] = {
    TupletInteger(FCAST_DT_KEY, 0),
    TupletCString(FCAST_ICON_KEY, "01d"),
    TupletCString(FCAST_DESC_KEY, "N/A"),
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);

  forecast_update_proc(NULL);
}

static void window_unload(Window *window) {
}

void forecast_window_show(void) {
  window_stack_push(window, true /* animated */);
}

void forecast_window_init(void) {
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

void forecast_window_deinit(void) {
  window_destroy(window);
}
