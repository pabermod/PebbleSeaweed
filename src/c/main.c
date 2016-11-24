#include <pebble.h>
#include "main.h"

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_swell_first_layer;
static TextLayer *s_wind_first_layer;

static GFont s_time_font;
static GFont s_weather_font;

static GBitmap *s_star_empty_bitmap;
static GBitmap *s_star_full_bitmap;

static Layer *s_forecast_first_canvas;
static Layer *s_forecast_second_canvas;
static Layer *s_ruler_first_layer;
static Layer *s_ruler_second_layer;

static const int16_t MARGIN = 5;

// An instance of the struct
static ClaySettings settings;

// Initialize the default settings
static void default_settings() {
	settings.FavouriteHour = 13;
}

// Read settings from persistent storage
static void load_settings() {
	// Load the default settings
	default_settings();
	// Read settings from persistent storage, if they exist
	persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Save the settings to persistent storage
static void save_settings() {
	persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void notify_application(){
	// Begin dictionary
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	// Add a key-value pair
	dict_write_uint8(iter, 0, 0);

	// Send the message!
	app_message_outbox_send();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	// Store incoming information
	static char period_buffer[8];
	static char height_buffer[8];
	static char forecast_layer_buffer[32];
	static char favhour_buffer[4];

	// Read fav hour tuple
	Tuple *fav_hour_tuple = dict_find(iterator, MESSAGE_KEY_FavouriteHour);

	if(fav_hour_tuple){
		// Favourite hour changed
		snprintf(favhour_buffer, sizeof(favhour_buffer), "%s", fav_hour_tuple->value->cstring);
		int new_favhour = atoi(favhour_buffer);
		if (new_favhour != settings.FavouriteHour)
		{
			APP_LOG(APP_LOG_LEVEL_INFO, "Favourite Hour is: %d", new_favhour);
			settings.FavouriteHour = new_favhour;
			save_settings();
			notify_application();
		}
	} 
	else {
		Tuple *period_tuple = dict_find(iterator, MESSAGE_KEY_Period);
		Tuple *heights_tuple = dict_find(iterator, MESSAGE_KEY_Height);
		// If all data is available, use it 
		if(period_tuple && heights_tuple) {
		    snprintf(period_buffer, sizeof(period_buffer), "%ds", (int)period_tuple->value->int32);
		    snprintf(height_buffer, sizeof(height_buffer), "%sm", heights_tuple->value->cstring);

		    APP_LOG(APP_LOG_LEVEL_INFO, "period_buffer is %s", period_buffer);
		    APP_LOG(APP_LOG_LEVEL_INFO, "height_buffer is %s", height_buffer);

		    // Assemble full string and display
		    snprintf(forecast_layer_buffer, sizeof(forecast_layer_buffer), "%s, %s", 
		    	height_buffer, period_buffer);
			text_layer_set_text(s_swell_first_layer, forecast_layer_buffer);
			text_layer_set_text(s_wind_first_layer, forecast_layer_buffer);
		}
	} 	
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void update_time() {
	// Get a tm structure
	time_t temp = time(NULL); 
	struct tm *tick_time = localtime(&temp);

	// Write the current hours and minutes into a buffer
	static char s_buffer[8];
	strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
	                                      "%H:%M" : "%I:%M", tick_time);

	// Display this time on the TextLayer
	text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();

	// Get weather update every 30 minutes
	if(tick_time->tm_min % 30 == 0) {
		notify_application();
	}
}

static void horizontal_ruler_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);

  // y relative to layer's bounds to support clipping after some vertical scrolling
  const int16_t yy = bounds.size.h / 2;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, GPoint(0, yy), GPoint(bounds.size.w, yy));
}

static void forecast_first_update_proc(Layer* layer, GContext* ctx){
  // First of all set black background
  graphics_context_set_fill_color(ctx, GColorBlack); 
  graphics_context_set_stroke_color(ctx, GColorBlack);

  int i;
  int width = 20;

  for (i = 0; i < 3; ++i)
  {
    graphics_draw_bitmap_in_rect(ctx, s_star_full_bitmap, GRect(width * i, 0, width, width - 1));
  }

  for (i = 3; i < 5; ++i)
  {
    graphics_draw_bitmap_in_rect(ctx, s_star_empty_bitmap, GRect(width * i, 0, width, width - 1));  
  }
}

static void main_window_load(Window *window) {
	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	// Create the TextLayer with specific bounds
	s_time_layer = text_layer_create(
	  GRect(MARGIN, -6, bounds.size.w - 2 * MARGIN, 42));

	// Improve the layout to be more like a watchface
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorWhite);
	text_layer_set_text(s_time_layer, "00:00");
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);

	// Create GFont
	s_time_font = fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
	
	// Apply to TextLayer
	text_layer_set_font(s_time_layer, s_time_font);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

	// Create first Horizontal ruler
	s_ruler_first_layer = layer_create(GRect(MARGIN, 36, bounds.size.w - 2 * MARGIN, 10));;
	layer_set_update_proc(s_ruler_first_layer, horizontal_ruler_update_proc);
  	layer_add_child(window_layer, s_ruler_first_layer);

  	//Create first forecast canvas
  	s_forecast_first_canvas = layer_create(GRect(MARGIN, 44, bounds.size.w - 2 * MARGIN, 20));
  	layer_add_child(window_layer, s_forecast_first_canvas);
  	layer_set_update_proc(s_forecast_first_canvas, forecast_first_update_proc);

  	// First swell layer
	s_swell_first_layer = text_layer_create(
	  GRect(MARGIN, 64, bounds.size.w - 2 * MARGIN, 20));

	// Style the text
	text_layer_set_background_color(s_swell_first_layer, GColorClear);
	text_layer_set_text_color(s_swell_first_layer, GColorWhite);
	text_layer_set_text_alignment(s_swell_first_layer, GTextAlignmentLeft);
	text_layer_set_text(s_swell_first_layer, "Loading...");

	// First wind layer
	s_wind_first_layer = text_layer_create(
	  GRect(MARGIN, 86, bounds.size.w - 2 * MARGIN, 20));

	text_layer_set_background_color(s_wind_first_layer, GColorClear);
	text_layer_set_text_color(s_wind_first_layer, GColorWhite);
	text_layer_set_text_alignment(s_wind_first_layer, GTextAlignmentLeft);
	text_layer_set_text(s_wind_first_layer, "Loading...");

	// Create second custom font, apply it and add textlayers to Window
	s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));
	text_layer_set_font(s_swell_first_layer, s_weather_font);
	text_layer_set_font(s_wind_first_layer, s_weather_font);
	layer_add_child(window_layer, text_layer_get_layer(s_swell_first_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_wind_first_layer));

	// Create second Horizontal ruler
	s_ruler_second_layer = layer_create(GRect(MARGIN, 97, bounds.size.w - 2 * MARGIN, 10));;
	layer_set_update_proc(s_ruler_first_layer, horizontal_ruler_update_proc);
  	layer_add_child(window_layer, s_ruler_first_layer);
}

static void main_window_unload(Window *window) {
	// Destroy TextLayers
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_swell_first_layer);
	text_layer_destroy(s_wind_first_layer);

	// Unload Fonts
	fonts_unload_custom_font(s_weather_font);

	// Destroy rulers
 	layer_destroy(s_ruler_first_layer);
 	layer_destroy(s_ruler_second_layer);

 	// Destroy stars
	gbitmap_destroy(s_star_empty_bitmap);
  	gbitmap_destroy(s_star_full_bitmap);

  	// Destroy forecast layers
 	layer_destroy(s_forecast_first_canvas);
 	layer_destroy(s_forecast_second_canvas);
}

static void init() {
	load_settings();

	// Create main Window element and assign to pointer
	s_main_window = window_create();

	// Set the background color
	window_set_background_color(s_main_window, GColorBlack);

	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
	.load = main_window_load,
	.unload = main_window_unload
	});

	// Create Stars GBitmaps
  	s_star_empty_bitmap = gbitmap_create_with_resource(RESOURCE_ID_STAR_EMPTY_WHITE_20);
  	s_star_full_bitmap = gbitmap_create_with_resource(RESOURCE_ID_STAR_FULL_WHITE_20);

	// Show the Window on the watch, with animated=true
	window_stack_push(s_main_window, true);

	// Make sure the time is displayed from the start
	update_time();

	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	// Register callbacks
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);

	// Open AppMessage
	const int inbox_size = 128;
	const int outbox_size = 128;
	app_message_open(inbox_size, outbox_size);
}

static void deinit() {
	// Destroy Window
	window_destroy(s_main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}