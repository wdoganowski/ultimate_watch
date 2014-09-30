#include "pebble.h"

#include "jsmn.h"
#include "openweather.h"

#define log_die(s) {APP_LOG(APP_LOG_LEVEL_ERROR,s); return 1;}

// based on http://alisdair.mcdiarmid.org/2012/08/14/jsmn-example.html

static bool json_token_streq(const char *js, jsmntok_t *t, char *s)
{
  return (strncmp(js + t->start, s, t->end - t->start) == 0
          && strlen(s) == (size_t) (t->end - t->start));
}

static char * json_token_tostr(const char *js, jsmntok_t *t)
{
  char *local = (char *)js;
  local[t->end] = '\0';
  return local + t->start;
}

// num_tokens - number of tokens in object, 0 for objects and primitives
// label is a pointer of the label of the value, object or array, null in case new label is expected
static unsigned int depth = 0;
static char labels[3/*max depth*/][20/*max label size*/];
int fill_forecast_struct(const char* js, jsmntok_t * tokens, unsigned int num_tokens, unsigned int i, char* label) 
{
  unsigned int error = 0;

  if (depth > ARRAY_LENGTH(labels)) log_die("We are too deep");

  do
  {
    jsmntok_t *t = &tokens[i];
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "fill_forecast_struct depth %d %s i %d num %d label %s type %d", depth, depth?labels[depth]-1:"root", i, num_tokens, label?label:"none", t->type);
  
    if (!label && t->type!=JSMN_STRING) log_die("Label must be a string");

    switch (t->type)
    {
      case JSMN_PRIMITIVE:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "JSMN_PRIMITIVE - %s { %s : %s }", depth?labels[depth-1]:"root", label?label:"none", json_token_tostr(js, t));
        break;
      case JSMN_STRING:
          if (!label) {
            char* new_label = json_token_tostr(js, t);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "JSMN_STRING - label %s", new_label);
            error = fill_forecast_struct(js, &tokens[++i], 0, 0, new_label);
          } else {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "JSMN_STRING - %s { %s : %s }", depth?labels[depth-1]:"root", label, json_token_tostr(js, t));
          }
        break;
      case JSMN_OBJECT:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "JSMN_OBJECT label %s %s", depth?labels[depth]-1:"root", label?label:"none");
        snprintf(labels[depth++], 20, label);
        error = fill_forecast_struct(js, &tokens[++i], t->size, 0, NULL);
        depth--;
        break;
      case JSMN_ARRAY:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "JSMN_ARRAY label %s %s", depth?labels[depth-1]:"root", label?label:"none");
        snprintf(labels[depth++], 20, label);
        error = fill_forecast_struct(js, &tokens[++i], t->size, 0, NULL);
        depth--;
        break;
      default:
        log_die("Invalid state");
    }
  } while (!error && ++i <= num_tokens);

  return error;
}

/*enum states {
  HEADER, HEADER_CONTENT, 
  CITY, CITY_CONTENT, 
  LIST
};

int fill_forecast_struct(char* js, jsmntok_t * tokens, unsigned int num_tokens)
{
  int error = 0;
  // state is the current state of the parser
  int state = HEADER;

  for (size_t i = 0; i < num_tokens; ++i)
  {
    jsmntok_t *t = &tokens[i];

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Dump %d %d %d", i, t->type, t->size);
  }

  for (size_t i = 0, j = 1; j > 0; i++, j--)
  {
    jsmntok_t *t = &tokens[i];

    // Add the items in current array or object
    if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
      j += t->size;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Loop %d %d", i, j);

    switch (state) {
      case HEADER:
        if (t->type == JSMN_OBJECT) state = HEADER_CONTENT;
        else log_die("The header should be an object");
        break;

      case HEADER_CONTENT:
        if (j % 2 == 0) { // label
          if (t->type == JSMN_STRING && json_token_streq(js, t, "city")) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "City");
            state = CITY;
            break;
          } else if (t->type == JSMN_STRING && json_token_streq(js, t, "list")) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "City");
            state = LIST;
            break;
          }
        }

      case CITY:
        if (t->type == JSMN_OBJECT) state = CITY_CONTENT;
        else log_die("The header should be an object");
        break;  

     case CITY_CONTENT:
        if (j % 2 == 0) { // label
          if (t->type == JSMN_STRING && json_token_streq(js, t, "name")) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Name");
            break;
          }
        }        
    }
  */  
/*    switch (t->type)
    {
      case JSMN_PRIMITIVE:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "JSMN_PRIMITIVE - value %s", json_token_tostr(js, t));
        break;
      case JSMN_OBJECT:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "JSMN_OBJECT");
        break;
      case JSMN_ARRAY:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "JSMN_ARRAY");
        break;
      case JSMN_STRING:
        // if j is even, we have a label
        if (j % 2 == 0) {
          APP_LOG(APP_LOG_LEVEL_DEBUG, "JSMN_STRING - label %s", json_token_tostr(js, t));
        } else {
          APP_LOG(APP_LOG_LEVEL_DEBUG, "JSMN_STRING - value %s", json_token_tostr(js, t));
        }
        break;
      default:
        log_die("Invalid state");
    }
  
  
  }
  return error;
}*/
