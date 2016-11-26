#include "main.h"

static Window *s_main_window;
static TextLayer *s_time_layer;

static GFont s_time_font;
static GFont s_forecast_font;

static GBitmap *s_star_empty_bitmap;
static GBitmap *s_star_full_bitmap;

static Layer *s_forecast_first_layer;
static Layer *s_forecast_second_layer;
static Layer *s_rating_first_canvas;
static Layer *s_rating_second_canvas;
static TextLayer *s_swell_first_layer;
static TextLayer *s_wind_first_layer;
static TextLayer *s_swell_second_layer;
static TextLayer *s_wind_second_layer;
static Layer *s_ruler_first_layer;
static Layer *s_ruler_second_layer;

static const int16_t MARGIN = 4;

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

static char** parse_data(char* data) {
  ProcessingState* state = data_processor_create(data, '|');
  uint8_t num_strings = data_processor_count(state);
  char** strings = malloc(sizeof(char*) * num_strings);
  for (uint8_t n = 0; n < num_strings; n += 1) {
    strings[n] = data_processor_get_string(state);
  }
  return strings;
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	// Read fav hour tuple
	Tuple *fav_hour_tuple = dict_find(iterator, MESSAGE_KEY_FavouriteHour);

	if(fav_hour_tuple){
		// Favourite hour changed

		static char favhour_buffer[4];

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
			static char swell_period_buffer_first[8];
			static char swell_period_buffer_second[8];

			// Write period data to buffer
		    snprintf(swell_period_buffer_first, sizeof(swell_period_buffer_first), 
				"%ds", (int)period_tuple->value->int32);
			snprintf(swell_period_buffer_second, sizeof(swell_period_buffer_second), 
				"%ds", (int)period_tuple->value->int32+1);
			
			static char swell_height_buffer_first[8];
			static char swell_height_buffer_second[8];

			// Get height data
			char** strings = parse_data(heights_tuple->value->cstring);
			// Write height data to buffer
			snprintf(swell_height_buffer_first, sizeof(swell_height_buffer_first), 
				"%sm", strings[0]);
			snprintf(swell_height_buffer_second, sizeof(swell_height_buffer_second), 
				"%sm", strings[1]);

			static char swell_buffer_first[16];
			static char swell_buffer_second[16];

		    // Assemble full first swell and display			
		    snprintf(swell_buffer_first, sizeof(swell_buffer_first), "%s %s", 
				swell_height_buffer_first, swell_period_buffer_first);
			text_layer_set_text(s_swell_first_layer, swell_buffer_first);
			//text_layer_set_text(s_wind_first_layer, swell_buffer_first);

		    // Assemble full second swell and display	
		    snprintf(swell_buffer_second, sizeof(swell_buffer_first), "%s %s", 
		    	swell_height_buffer_second, swell_period_buffer_second);
			text_layer_set_text(s_swell_second_layer, swell_buffer_second);
			//text_layer_set_text(s_wind_second_layer, swell_buffer_first);
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

static void layer_add_first_forecast(Layer *window_layer, GRect bounds){
	// First Forecast Layer
	s_forecast_first_layer = layer_create(GRect(MARGIN, 35, bounds.size.w - 2 * MARGIN, 72));

	// Create first Horizontal ruler
	s_ruler_first_layer = layer_create(GRect(0, 0, bounds.size.w, 5));
	layer_set_update_proc(s_ruler_first_layer, horizontal_ruler_update_proc);
  	layer_add_child(s_forecast_first_layer, s_ruler_first_layer);

  	//Create first forecast canvas
  	s_rating_first_canvas = layer_create(GRect(0, 5, bounds.size.w, 20));
  	layer_add_child(s_forecast_first_layer, s_rating_first_canvas);
  	layer_set_update_proc(s_rating_first_canvas, forecast_first_update_proc);

  	// First swell layer
	s_swell_first_layer = text_layer_create(GRect(0, 23, bounds.size.w, 21));

	// Style the text
	text_layer_set_background_color(s_swell_first_layer, GColorClear);
	text_layer_set_text_color(s_swell_first_layer, GColorWhite);
	text_layer_set_text_alignment(s_swell_first_layer, GTextAlignmentLeft);
	text_layer_set_text(s_swell_first_layer, "Loading...");

	// First wind layer
	s_wind_first_layer = text_layer_create(GRect(0, 42, bounds.size.w, 21));

	text_layer_set_background_color(s_wind_first_layer, GColorClear);
	text_layer_set_text_color(s_wind_first_layer, GColorWhite);
	text_layer_set_text_alignment(s_wind_first_layer, GTextAlignmentLeft);
	text_layer_set_text(s_wind_first_layer, "Loading...");

	// Apply forecast font and add textlayers to window
	text_layer_set_font(s_swell_first_layer, s_forecast_font);
	text_layer_set_font(s_wind_first_layer, s_forecast_font);
	layer_add_child(s_forecast_first_layer, text_layer_get_layer(s_swell_first_layer));
	layer_add_child(s_forecast_first_layer, text_layer_get_layer(s_wind_first_layer));

	// Add first forecast layer to window layer
	layer_add_child(window_layer, s_forecast_first_layer);
}

static void layer_add_second_forecast(Layer *window_layer, GRect bounds){
	// Second Forecast Layer
	s_forecast_second_layer = layer_create(GRect(MARGIN, 100, bounds.size.w - 2 * MARGIN, 72));

	// Create second Horizontal ruler
	s_ruler_second_layer = layer_create(GRect(0, 0, bounds.size.w, 5));
	layer_set_update_proc(s_ruler_second_layer, horizontal_ruler_update_proc);
  	layer_add_child(s_forecast_second_layer, s_ruler_second_layer);

  	//Create first forecast canvas
  	s_rating_second_canvas = layer_create(GRect(0, 5, bounds.size.w, 20));
  	layer_add_child(s_forecast_second_layer, s_rating_second_canvas);
  	layer_set_update_proc(s_rating_second_canvas, forecast_first_update_proc);

  	// Second swell layer
	s_swell_second_layer = text_layer_create(GRect(0, 23, bounds.size.w, 21));

	// Style the text
	text_layer_set_background_color(s_swell_second_layer, GColorClear);
	text_layer_set_text_color(s_swell_second_layer, GColorWhite);
	text_layer_set_text_alignment(s_swell_second_layer, GTextAlignmentLeft);
	text_layer_set_text(s_swell_second_layer, "Loading...");

	// second wind layer
	s_wind_second_layer = text_layer_create(GRect(0, 42, bounds.size.w, 21));

	text_layer_set_background_color(s_wind_second_layer, GColorClear);
	text_layer_set_text_color(s_wind_second_layer, GColorWhite);
	text_layer_set_text_alignment(s_wind_second_layer, GTextAlignmentLeft);
	text_layer_set_text(s_wind_second_layer, "Loading...");

	// Apply forecast font and add textlayers to window
	text_layer_set_font(s_swell_second_layer, s_forecast_font);
	text_layer_set_font(s_wind_second_layer, s_forecast_font);
	layer_add_child(s_forecast_second_layer, text_layer_get_layer(s_swell_second_layer));
	layer_add_child(s_forecast_second_layer, text_layer_get_layer(s_wind_second_layer));

	// Add second forecast layer to window layer
	layer_add_child(window_layer, s_forecast_second_layer);
}

static void main_window_load(Window *window) {
	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	// Create Time textlayer
	s_time_layer = text_layer_create(
	  GRect(MARGIN, -12 + MARGIN, bounds.size.w - 2 * MARGIN, 42));

	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorWhite);
	text_layer_set_text(s_time_layer, "00:00");
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);

	// Create GFont and Apply to TextLayer
	s_time_font = fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);	
	text_layer_set_font(s_time_layer, s_time_font);

	// Create Forecast font
	s_forecast_font = fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

	// Add forecast layers
	layer_add_first_forecast(window_layer, bounds);
	layer_add_second_forecast(window_layer, bounds);
}

static void main_window_unload(Window *window) {
	// Destroy TextLayers
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_swell_first_layer);
	text_layer_destroy(s_wind_first_layer);
	text_layer_destroy(s_swell_second_layer);
	text_layer_destroy(s_wind_second_layer);

	// Unload Fonts
	//fonts_unload_custom_font(s_forecast_font);

	// Destroy rulers
 	layer_destroy(s_ruler_first_layer);
 	layer_destroy(s_ruler_second_layer);

 	// Destroy stars
	gbitmap_destroy(s_star_empty_bitmap);
  	gbitmap_destroy(s_star_full_bitmap);

  	// Destroy forecast layers
 	layer_destroy(s_rating_first_canvas);
 	layer_destroy(s_rating_second_canvas);
	layer_destroy(s_forecast_first_layer);
	layer_destroy(s_forecast_second_layer);
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