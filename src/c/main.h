#pragma once
#include <pebble.h>
#include <@smallstoneapps/data-processor/data-processor.h>

#define FORECAST_KEY 1
#define SETTINGS_KEY 2

// A structure containing our settings
typedef struct ForecastData
{
  int FadedRating;
  int SolidRating;
  int SwellPeriod;
  char SwellHeight[8];
  int SwellDirection;
  int WindSpeed;
  int WindDirection;
} ForecastData;

// A structure containing our settings
typedef struct ClaySettings
{
  int Color;
  int FavouriteHour;
} ClaySettings;

static void default_settings();
static void load_settings();
static void save_settings();
static void notify_application();
static void inbox_received_callback(DictionaryIterator *iter, void *context);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);
static void update_time();
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void horizontal_ruler_update_proc(Layer *layer, GContext *ctx);
static void rating_first_update_proc(Layer *layer, GContext *ctx);
static void rating_second_update_proc(Layer *layer, GContext *ctx);
static void wave_update_proc(Layer *layer, GContext *ctx);
static void layer_add_first_forecast(Layer *window_layer, GRect bounds);
static void layer_add_second_forecast(Layer *window_layer, GRect bounds);
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void init(void);
static void deinit(void);