#include "pebble.h"
#include <stdio.h>
#include <stdlib.h>

#define DEBUG_ON
#include "debug.h"

#include "openweather.h"
#include "jsonparser.h"

static AppSync sync;
static uint8_t sync_buffer[2048];
static AppTimer* update_timer = NULL;
static uint32_t update_interval = OPENWEATHER_REFRESH_INTERVAL;

static OpenweatherCallback callback_stack[OPENWEATHER_MAX_CALLBACKS] = {NULL, NULL};
static uint8_t callback_num = 0;

enum WeatherKey {
  OPENWEATHER_KEY    = 0, // TUPLE_CSTRING
};

ForecastType forecast_data = {NULL, 0, NULL};
/*
 * Icons
 */
#define OPENWEATHER_NUM_ICONS 19

const WeatherIcon weather_icons_16[OPENWEATHER_NUM_ICONS] = {
  {"xxx", RESOURCE_ID_IMAGE_16_EMPTY}, // default empty icon  
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

const WeatherIcon weather_icons_32[OPENWEATHER_NUM_ICONS] = {
  {"xxx", RESOURCE_ID_IMAGE_32_UPDATE},
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

uint32_t find_icon_resource(const WeatherIcon* icons, const char* icon) {
  for (unsigned int i = 0; i < OPENWEATHER_NUM_ICONS; i++)
  {
    if (strcmp(icons[i].icon, icon) == 0)
    {
      return icons[i].resource_id;
    }
  }
  // If not found, return default icon
  return icons[0].resource_id;
}

/*
 * Conversion of number to string
 */

// http://forums.getpebble.com/discussion/4603/sprintf-alternative
static char *itoa(int num)
{
  static char buff[20] = {};
  int i = 0, temp_num = num, length = 0;
  char *string = buff;
  
  if(num >= 0) {
    // count how many characters in the number
    while(temp_num) {
      temp_num /= 10;
      length++;
    }
    
    // assign the number to the buffer starting at the end of the 
    // number and going to the begining since we are doing the
    // integer to character conversion on the last number in the
    // sequence
    for(i = 0; i < length; i++) {
      buff[(length-1)-i] = '0' + (num % 10);
      num /= 10;
    }
    buff[i] = '\0'; // can't forget the null byte to properly end our string
  }
  else
    return "Unsupported Number";
  
  return string;
}

/*
 * Messages
 */

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

static void set_timer(uint32_t interval);


OpenweatherCallback openweather_register_callback(OpenweatherCallback callback) {
  if (callback_num >= OPENWEATHER_MAX_CALLBACKS) {
    LOG_DEBUG("Too many callback, already have %d", callback_num);
    return NULL;
  } else {
    callback_stack[callback_num++] = callback;
    LOG_DEBUG("Callback registered %d", callback_num);
    callback();
    return callback;
  }
}

void openweather_deregister_callback(OpenweatherCallback callback) {
  for (int i = 0; i < callback_num; ++i)
  {
    if (callback_stack[i] == callback) {
      for (int j = i + 1; j < callback_num; ++j)
      {
        callback_stack[j - 1] = callback_stack[j];
      }
      LOG_DEBUG("Callback deregistered %d", callback_num);
      callback_stack[callback_num--] = NULL;
      break;
    }
  }
}

void openweather_deregister_all_callbacks(void) {
  for (int i = 0; i < OPENWEATHER_MAX_CALLBACKS; ++i)
  {
    callback_stack[i] = NULL;
  }
  callback_num = 0;
}

static void openweather_issue_callbacks(void) {
  LOG_DEBUG("Number of callbacks %d", callback_num);
  for (int i = 0; i < callback_num; ++i)
  {
    if (callback_stack[i]) callback_stack[i]();
  }
}

static void set_string(char** dest, const char* source, uint16_t length) {      
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Set string: size %d dest %s source %s", length, (*dest)?(*dest):"NULL", source?source:"NULL");
  if (*dest) free(*dest);
  *dest = malloc(length + 1);
  if (*dest) strncpy(*dest, source, length);
}

inline uint16_t count_until(char c, char* source, uint16_t length) {
  for (uint16_t i = 0; i < length; ++i)
  {
    if (source[i] == c) return i;
  }
  return length;
}

void object_callback(JSP_ValueType type, char* label, uint16_t label_length, char* value, uint16_t value_length) {
/*  char* l = calloc(label_length + 1, sizeof(char)); 
  char* v = calloc(value_length + 1, sizeof(char)); 

  snprintf(l, label_length + 1, "%s", label); 
  snprintf(v, value_length + 1, "%s", value); 

  APP_LOG(APP_LOG_LEVEL_INFO, "object_callback(%s, %s, %d, %s, %d)", type_labels[type], l, label_length, v, value_length);

  free(v); 
  free(l); 
*/
  if (forecast_data.day) {
    if (strncmp(label, "day", 3) == 0) {

      if (forecast_data.cnt < OPENWEATHER_FCAST_DAYS) {

        char buf[10];
        strncpy(buf, value, value_length);
        forecast_data.day[forecast_data.cnt].temp.day = atoi(buf);
        
        forecast_data.day->weather.icon = "01d";
        forecast_data.cnt += 1;
      }

    }
  }  
}

void array_callback(JSP_ValueType type, char* value, uint16_t value_length) {
/*  char* v = calloc(value_length + 1, sizeof(char)); 

  snprintf(v, value_length + 1, "%s", value); 

  APP_LOG(APP_LOG_LEVEL_INFO, "array_callback(%s, %s, %d)", type_labels[type], v, value_length);

  free(v); */
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d %s", app_message_error, translate_error(app_message_error));

  // Increase the update rate
  if (app_message_error == APP_MSG_SEND_TIMEOUT ||
      app_message_error == APP_MSG_NOT_CONNECTED ||
      app_message_error == APP_MSG_APP_NOT_RUNNING) {
      set_timer(OPENWEATHER_GET_INTERVAL);
  }
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync received: %lu", key);

  // Set the timer to normal rate
  if (update_interval != OPENWEATHER_REFRESH_INTERVAL) set_timer(OPENWEATHER_REFRESH_INTERVAL);

  // Interpret the forecast
  if (key == OPENWEATHER_KEY) {
    if (new_tuple->value->cstring)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Data: size %u ***%s***", (unsigned int)strlen(new_tuple->value->cstring), new_tuple->value->cstring);
    else 
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Data: NULL");

    forecast_data.cnt = 0;

    JSP_ErrorType result = json_parser(new_tuple->value->cstring);
    if (result == JSP_OK) openweather_issue_callbacks();
    else LOG_DEBUG("Json parser error 0x%x", result);

  } else LOG_ERROR("Unknown key received %lu", key);
}

static void update_proc(void *data) {
  Tuplet value = TupletCString(0, "");

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter) {
    dict_write_tuplet(iter, &value);
    dict_write_end(iter);

    app_message_outbox_send();
  }

  // reset automatic update with current interval
  set_timer(0);
}

static void set_timer(uint32_t interval) {
  // reset the timer
  // use 0 as intrval if you do not want to chang the interval

  if (update_timer) {
    app_timer_cancel(update_timer);
  }
  if (interval) {
    update_interval = interval;
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Update interval set to %u", (unsigned int)update_interval);  
  update_timer = app_timer_register(update_interval, update_proc, NULL);
}

void openweather_init(void) {
  // init forecast structure
  forecast_data.day = calloc(OPENWEATHER_FCAST_DAYS, sizeof(ForecastDayType));

  // init json parser
  json_register_callbacks(object_callback, array_callback);

  // Open channel
  const int inbound_size = app_message_inbox_size_maximum();
  const int outbound_size = app_message_outbox_size_maximum();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Determined sizes: Inbox %u Outbox %u", inbound_size, outbound_size);  

  app_message_open(inbound_size, outbound_size);

  // Init sync
  Tuplet initial_values[] = {
    TupletCString(OPENWEATHER_KEY, "")
  };
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);

  // Set update
  set_timer(OPENWEATHER_REFRESH_INTERVAL);

}

void openweather_deinit(void) {
  app_message_deregister_callbacks();
  if (update_timer) app_timer_cancel(update_timer);
  app_sync_deinit(&sync);
  openweather_deregister_all_callbacks();
  forecast_data.cnt = 0;
  if (forecast_data.day) free(forecast_data.day);
}
