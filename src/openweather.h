#pragma once

#include "pebble.h"

#define OPENWEATHER_REFRESH_INTERVAL (10 * 60 * 1000) // 10 minutes in miliseconds
#define OPENWEATHER_GET_INTERVAL (10 * 1000) // Used when there was a refresh error 

#define OPENWEATHER_MAX_CALLBACKS 2
#define OPENWEATHER_FCAST_DAYS 5

typedef struct WeatherIcon {
  char icon[4];
  uint32_t resource_id;
} WeatherIcon;

typedef struct ForecastDayType {
  unsigned long dt;
  struct temp {
    char* day;
    char* min;
    char* max;
    char* night;
    char* eve;
    char* morn;
  } temp;
  unsigned int pressure;
  unsigned int humidity;
  struct weather {
    //unsigned int id;
    char* main;
    char* description;
    char* icon;
  } weather;
  unsigned int speed;
  unsigned int deg;
  unsigned int clouds;
} ForecastDayType;

typedef struct ForecastType {
  char* city;
  unsigned int cnt;
  ForecastDayType * day;
} ForecastType;

extern ForecastType forecast_data;

extern const WeatherIcon weather_icons_16[];
extern const WeatherIcon weather_icons_32[];

uint32_t find_icon_resource(const WeatherIcon* icons, const char* icon);

typedef void(* OpenweatherCallback)(void);

OpenweatherCallback openweather_register_callback(OpenweatherCallback callback);
void openweather_deregister_callback(OpenweatherCallback callback);

void openweather_init(void);
void openweather_deinit(void);
