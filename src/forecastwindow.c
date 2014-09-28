#include "pebble.h"

#include "forecastwindow.h"
#include "openweather.h"

static Window *window;

static OpenweatherCallback forecast_callback = NULL;

static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;
static TextLayer *td_layer;
static TextLayer *desc_layer;

static int day = 0; // day 0 = today
static char td_label_buff[11];

static const char * fcast_day[5] = {
  "Today", "Tommorow", "", "", ""
};

static void forecast_update_proc() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Forecast callback received");
/*
 switch (key) {
    case FCAST_DT_KEY: {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "DT: %u", (int)new_tuple->value->int32);

      strcpy(td_label_buff, fcast_day[day]);
      if (!strlen(td_label_buff) && new_tuple->value->int32) {
        strftime(td_label_buff, sizeof(td_label_buff), "%a %d %b", localtime((time_t *)&new_tuple->value->uint32));
      }
      APP_LOG(APP_LOG_LEVEL_DEBUG, "DT: %d <%s>", (int)new_tuple->value->int32, td_label_buff);
      text_layer_set_text(td_layer, td_label_buff);
      break;
    }
    
    case FCAST_ICON_KEY:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "ICON: %s", new_tuple->value->cstring);

      if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }
      uint32_t icon_resource = find_icon_resource(new_tuple->value->cstring);
      icon_bitmap = gbitmap_create_with_resource(icon_resource);
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      break;

    case FCAST_DESC_KEY:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "DESC: %s", new_tuple->value->cstring);

      text_layer_set_text(desc_layer, new_tuple->value->cstring);
      break;
  }*/
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
  icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_32_UPDATE);
  bitmap_layer_set_bitmap(icon_layer, icon_bitmap);

  // description 
  desc_layer = text_layer_create(GRect(FCAST_ICON_W + 4, 24, bounds.size.w - (FCAST_ICON_W + 4), 18));
  text_layer_set_text_color(desc_layer, GColorWhite);
  text_layer_set_text_color(desc_layer, GColorWhite);
  text_layer_set_background_color(desc_layer, GColorClear);  
  text_layer_set_font(desc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(desc_layer, "Waiting for connection");
  layer_add_child(window_layer, text_layer_get_layer(desc_layer));

  forecast_callback = openweather_register_callback(forecast_update_proc);
}

static void window_unload(Window *window) {
  if (forecast_callback) openweather_deregister_callback(forecast_callback);

  if (icon_bitmap) {
    gbitmap_destroy(icon_bitmap);
  }

  text_layer_destroy(td_layer);
  bitmap_layer_destroy(icon_layer);
  text_layer_destroy(desc_layer);
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
  /* Reuse the inbox already initialized */

  // Set key handler
  window_set_click_config_provider(window, config_provider);
}

void forecast_window_deinit(void) {
  window_destroy(window);
}
