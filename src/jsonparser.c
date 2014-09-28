#include "pebble.h"

#include "json.h"
#include "openweather.h"

int fill_forecast_struct(json_value * data)
{
  int error = 0;

  switch (data->type) {
    case json_object:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "OBJECT length %d", data->u.object.length);
      //value = PyDict_New();
      for (unsigned int i = 0; i < data->u.object.length; i++) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "  Name %s", data->u.object.values[i].name);
        error = fill_forecast_struct(data->u.object.values[i].value);
        /*
        PyObject * name = PyUnicode_FromString(
            data->u.object.values[i].name);
        PyObject * object_value = convert_value(
            data->u.object.values[i].value);
        PyDict_SetItem(value, name, object_value);
        */
      }
      break;
    case json_array:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "ARRAY length %d", data->u.object.length);
      //value = PyList_New(0);
      for (unsigned int i = 0; i < data->u.array.length; i++) {
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Name %s Value %s", data->u.object.values[i].name, data->u.object.values[i].value);
        /*
        PyObject * array_value = convert_value(
            data->u.array.values[i]);
        PyList_Append(value, array_value);
        */
      }
      break;
    case json_integer:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INTEGER %d", (int)data->u.integer);
      //value = PyInt_FromLong(data->u.integer);
      break;
    case json_double:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "DOUBLE %f", data->u.dbl);
      //value = PyFloat_FromDouble(data->u.dbl);
      break;
    case json_string:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "STRING length %d value %s", data->u.string.length, data->u.string.ptr);
      //value = PyUnicode_FromStringAndSize(data->u.string.ptr, 
      //    data->u.string.length);
      break;
    case json_boolean:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "BOOLEAN %d", data->u.boolean);
      //value = PyBool_FromLong((long)data->u.boolean);
      break;
    default:
      // covers json_null, json_none
      //Py_INCREF(Py_None);
      //value = Py_None;
      error = 1;
      break;
  }
return error;
}

/*int main () {
  json_char json[] = "{\"1\":\"2\",\"3\":\"4\",\"5\":{\"6\":\"7\"}}";
  json_value* result = json_parse (json, sizeof(json));
  if (result) {
    TestType* converted = fill_forecast_struct(result);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "JSON %s Lable %s Value %s", json, converted->label, converted->value);
    json_value_free(result);
  }
  return 0;
}*/
