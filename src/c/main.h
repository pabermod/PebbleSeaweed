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
  int Spot;
} ClaySettings;

static const int16_t MARGIN = 4;

static Window *s_main_window;
static TextLayer *s_time_layer;

static GFont s_time_font;
static GFont s_forecast_font;

static GBitmap *s_star_empty_bitmap;
static GBitmap *s_star_full_bitmap;
static GBitmap *s_wave_bitmap;
static GBitmap *s_wind_bitmap;

static Layer *s_forecast_first_layer;
static Layer *s_forecast_second_layer;
static Layer *s_rating_first_canvas;
static Layer *s_rating_second_canvas;
static Layer *s_wave_canvas;
static Layer *s_wind_canvas;
static TextLayer *s_swell_first_layer;
static TextLayer *s_wind_first_layer;
static TextLayer *s_swell_second_layer;
static TextLayer *s_wind_second_layer;
static Layer *s_ruler_first_layer;
static Layer *s_ruler_second_layer;
static Layer *s_battery_layer;

static ForecastData forecast[2];
static ClaySettings settings;
static int s_battery_level;

static void default_forecast();
static void default_settings();
static void load_forecast();
static void save_forecast();
static void load_settings();
static void notify_application();
static void save_settings();
static char **parse_data(char *data);
void copy_string(char d[], char s[]);
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
static void wind_update_proc(Layer *layer, GContext *ctx);
static void layer_add_first_forecast(Layer *window_layer, GRect bounds);
static void layer_add_second_forecast(Layer *window_layer, GRect bounds);
static void layer_add_time_text_layer(Layer *window_layer, GRect bounds);
static void layer_add_battery_layer(Layer *window_layer, GRect bounds);
static void battery_callback(BatteryChargeState state);
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void init(void);
static void deinit(void);