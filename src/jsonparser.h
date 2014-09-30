#pragma once

#include "pebble.h"
#include "jsmn.h"

int fill_forecast_struct(const char* js, jsmntok_t * tokens, unsigned int num_tokens, unsigned int i, char* label);
