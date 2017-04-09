#include "main.h"

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

static const int16_t MARGIN = 4;

static ForecastData forecast[2];
static ClaySettings settings;

// Initialize the default forecast
static void default_forecast(){
    forecast[0].FadedRating = forecast[1].FadedRating = 0;
    forecast[0].SolidRating = forecast[1].SolidRating = 0;
    forecast[0].SwellPeriod = forecast[1].SwellPeriod = 0;
    strcpy(forecast[0].SwellHeight, "0m");
    strcpy(forecast[1].SwellHeight, "0m");
    forecast[0].SwellDirection = forecast[1].SwellDirection = 0;
    forecast[0].WindSpeed = forecast[1].WindSpeed = 0;
    forecast[0].WindDirection = forecast[1].WindDirection = 0;
}

// Initialize the default settings
static void default_settings(){
    settings.FavouriteHour = 12;    
    settings.Color = 0;
    settings.Spot = 177;
}

// Read forecast from persistent storage
static void load_forecast(){
    // Load the default forecast
    default_forecast();
    // Read forecast from persistent storage, if they exist
    persist_read_data(FORECAST_KEY, &forecast, sizeof(forecast));
}

// Save forecast to persistent storage
static void save_forecast(){
    persist_write_data(FORECAST_KEY, &forecast, sizeof(forecast));
}

// Read settings from persistent storage
static void load_settings(){
    // Load the default settings
    default_settings();
    // Read settings from persistent storage, if they exist
    persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Save settings to persistent storage
static void save_settings(){
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

static char **parse_data(char *data){
    ProcessingState *state = data_processor_create(data, '|');
    uint8_t num_strings = data_processor_count(state);
    char **strings = malloc(sizeof(char *) * num_strings);
    for (uint8_t n = 0; n < num_strings; n += 1)
    {
	    strings[n] = data_processor_get_string(state);
    }
    return strings;
}

void copy_string(char d[], char s[]){
   int c = 0;
 
   while (s[c] != '\0') {
      d[c] = s[c];
      c++;
   }
   d[c] = '\0';
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context){
    APP_LOG(APP_LOG_LEVEL_INFO, "Message received");

    // Read settings
    Tuple *fav_hour_tuple = dict_find(iterator, MESSAGE_KEY_FavouriteHour);

    if (fav_hour_tuple){
		// Update favourite hour
		int new_favhour = fav_hour_tuple->value->int32;
        APP_LOG(APP_LOG_LEVEL_INFO, "Hour: %d -> %d", settings.FavouriteHour, new_favhour);
		if (new_favhour != settings.FavouriteHour)
		{
			settings.FavouriteHour = new_favhour;	
		}

        // Update color
        Tuple *color_tuple = dict_find(iterator, MESSAGE_KEY_Color);
		int new_color = color_tuple->value->int32;
        APP_LOG(APP_LOG_LEVEL_INFO, "Color: %d -> %d", settings.Color, new_color);
		if (new_color != settings.Color)
		{
			settings.Color  = new_color;	
		}

        // Update spot
        Tuple *spot_tuple = dict_find(iterator, MESSAGE_KEY_Spot);
		int new_spot = spot_tuple->value->int32;
        APP_LOG(APP_LOG_LEVEL_INFO, "Spot: %d -> %d", settings.Spot, new_spot);
		if (new_spot != settings.Spot)
		{
			settings.Spot  = new_spot;	
		}

        save_settings();
    }
    else{
        // Read forecast
        Tuple *faded_rating_one = dict_find(iterator, MESSAGE_KEY_FadedRating);
        Tuple *faded_rating_two = dict_find(iterator, MESSAGE_KEY_FadedRating+1);
        Tuple *solid_rating_one = dict_find(iterator, MESSAGE_KEY_SolidRating);
        Tuple *solid_rating_two = dict_find(iterator, MESSAGE_KEY_SolidRating+1);
        Tuple *swell_period_one = dict_find(iterator, MESSAGE_KEY_SwellPeriod);
        Tuple *swell_period_two = dict_find(iterator, MESSAGE_KEY_SwellPeriod+1);
        Tuple *swell_heights_tuple = dict_find(iterator, MESSAGE_KEY_SwellHeight);
        Tuple *swell_direction_one = dict_find(iterator, MESSAGE_KEY_SwellDirection);
        Tuple *swell_direction_two = dict_find(iterator, MESSAGE_KEY_SwellDirection+1);
        Tuple *wind_speed_one = dict_find(iterator, MESSAGE_KEY_WindSpeed);
        Tuple *wind_speed_two = dict_find(iterator, MESSAGE_KEY_WindSpeed+1);
        Tuple *wind_direction_one = dict_find(iterator, MESSAGE_KEY_WindDirection);
        Tuple *wind_direction_two = dict_find(iterator, MESSAGE_KEY_WindDirection+1);

        // If all data is available, use it
        if (faded_rating_one && faded_rating_two && solid_rating_one && solid_rating_two 
            && swell_period_one && swell_period_two && swell_heights_tuple 
            && swell_direction_one && swell_direction_two && wind_speed_one 
            && wind_speed_two && wind_direction_one && wind_direction_two)
        {
            APP_LOG(APP_LOG_LEVEL_INFO, "forecast received"); 
            // Update ratings
            forecast[0].FadedRating = faded_rating_one->value->int32;
            forecast[1].FadedRating = faded_rating_two->value->int32;
            forecast[0].SolidRating = solid_rating_one->value->int32;
            forecast[1].SolidRating = solid_rating_two->value->int32;

            // Update Swell
            static char swell_period_buffer_first[8];
            static char swell_period_buffer_second[8];
            
            forecast[0].SwellPeriod = swell_period_one->value->int32;
                snprintf(swell_period_buffer_first, sizeof(swell_period_buffer_first),
                    "%ds", forecast[0].SwellPeriod);

            forecast[1].SwellPeriod = swell_period_two->value->int32;
            snprintf(swell_period_buffer_second, sizeof(swell_period_buffer_second),
                "%ds", forecast[1].SwellPeriod);

            // Update height
            char **strings = parse_data(swell_heights_tuple->value->cstring);

            // Write height data to buffer
            snprintf(forecast[0].SwellHeight, sizeof(forecast[0].SwellHeight),
                "%sm", strings[0]);
            snprintf(forecast[1].SwellHeight, sizeof(forecast[1].SwellHeight),
                "%sm", strings[1]);

            forecast[0].SwellDirection = swell_direction_one->value->int32;
            forecast[1].SwellDirection = swell_direction_two->value->int32;

            static char swell_buffer_first[16];
            static char swell_buffer_second[16];
            
            // Assemble full first swell and display
            snprintf(swell_buffer_first, sizeof(swell_buffer_first), "%s %s",
                forecast[0].SwellHeight, swell_period_buffer_first);
            text_layer_set_text(s_swell_first_layer, swell_buffer_first);

            // Assemble full second swell and display
            snprintf(swell_buffer_second, sizeof(swell_buffer_first), "%s %s",
                forecast[1].SwellHeight, swell_period_buffer_second);
            text_layer_set_text(s_swell_second_layer, swell_buffer_second);
            
            // Update Wind
            static char wind_speed_buffer_first[8];
            static char wind_speed_buffer_second[8];
            
            forecast[0].WindSpeed = wind_speed_one->value->int32;
            snprintf(wind_speed_buffer_first, sizeof(wind_speed_buffer_first),
                "%dkmh", forecast[0].WindSpeed);
            text_layer_set_text(s_wind_first_layer, wind_speed_buffer_first);

            forecast[1].WindSpeed = wind_speed_two->value->int32;
            snprintf(wind_speed_buffer_second, sizeof(wind_speed_buffer_second),
                "%dkmh", forecast[1].WindSpeed);
            text_layer_set_text(s_wind_second_layer, wind_speed_buffer_second);

            forecast[0].WindDirection = wind_direction_one->value->int32;
            forecast[1].WindDirection = wind_direction_two->value->int32;

            // Save forecast
            save_forecast();
        }
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context){
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context){
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context){
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void update_time(){
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Write the current hours and minutes into a buffer
    static char s_buffer[8];
    strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

    // Display this time on the TextLayer
    text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
    update_time();

    // Get weather update every 60 minutes
    if (tick_time->tm_min % 60 == 0)
    {
	    notify_application();
    }
}

static void horizontal_ruler_update_proc(Layer *layer, GContext *ctx){
    const GRect bounds = layer_get_bounds(layer);

    // y relative to layer's bounds to support clipping after some vertical scrolling
    const int16_t yy = bounds.size.h / 2;

    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx, GPoint(0, yy), GPoint(bounds.size.w, yy));
}

static void rating_first_update_proc(Layer *layer, GContext *ctx){
    // First of all set black background
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorBlack);

    int width = 20;
    int i = 0;
    for (i = 0; i < forecast[0].SolidRating; ++i)
    {
	    graphics_draw_bitmap_in_rect(ctx, s_star_full_bitmap, 
            GRect(width * i, 0, width, width - 1));
    }

    for (int j = 0; j < forecast[0].FadedRating; ++j)
    {
	    graphics_draw_bitmap_in_rect(ctx, s_star_empty_bitmap, 
            GRect(width * (i + j), 0, width, width - 1));
    }
}

static void rating_second_update_proc(Layer *layer, GContext *ctx){
    // First of all set black background
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorBlack);

    int width = 20;
    int i = 0;
    for (i = 0; i < forecast[1].SolidRating; ++i)
    {
	    graphics_draw_bitmap_in_rect(ctx, s_star_full_bitmap, 
            GRect(width * i, 0, width, width - 1));
    }

    for (int j = 0; j < forecast[1].FadedRating; ++j)
    {
	    graphics_draw_bitmap_in_rect(ctx, s_star_empty_bitmap, 
            GRect(width * (i + j), 0, width, width - 1));
    }
}

static void wave_update_proc(Layer *layer, GContext *ctx){
    graphics_draw_bitmap_in_rect(ctx, s_wave_bitmap, GRect(0, 6, 20, 14));
}

static void wind_update_proc(Layer *layer, GContext *ctx){
    graphics_draw_bitmap_in_rect(ctx, s_wind_bitmap, GRect(0, 6, 20, 14));
}

static void layer_add_first_forecast(Layer *window_layer, GRect bounds){
    // First Forecast Layer
    s_forecast_first_layer = layer_create(GRect(MARGIN, 35, bounds.size.w - 2 * MARGIN, 72));

    // Create first Horizontal ruler
    s_ruler_first_layer = layer_create(GRect(0, 0, bounds.size.w, 5));
    layer_set_update_proc(s_ruler_first_layer, horizontal_ruler_update_proc);
    layer_add_child(s_forecast_first_layer, s_ruler_first_layer);

    //Create first forecast canvas
    s_rating_first_canvas = layer_create(GRect(-1, 5, bounds.size.w-5, 20));
    layer_add_child(s_forecast_first_layer, s_rating_first_canvas);
    layer_set_update_proc(s_rating_first_canvas, rating_first_update_proc);

    // First wave canvas
    s_wave_canvas = layer_create(GRect(-4, 24, 20, 20));
    layer_add_child(s_forecast_first_layer, s_wave_canvas);
    layer_set_update_proc(s_wave_canvas, wave_update_proc);

    // First swell layer
    s_swell_first_layer = text_layer_create(GRect(23, 23, bounds.size.w-23, 21)); 

    // Get Swell from forecast
    static char swell_period_buffer_first[8];
    snprintf(swell_period_buffer_first, sizeof(swell_period_buffer_first),
        "%ds", forecast[0].SwellPeriod);
    static char swell_buffer_first[16];
    snprintf(swell_buffer_first, sizeof(swell_buffer_first), "%s %s",
        forecast[0].SwellHeight, swell_period_buffer_first);

    // Style the text and display
    text_layer_set_background_color(s_swell_first_layer, GColorClear);
    text_layer_set_text_color(s_swell_first_layer, GColorWhite);
    text_layer_set_text_alignment(s_swell_first_layer, GTextAlignmentLeft);
    text_layer_set_text(s_swell_first_layer, swell_buffer_first);

    // First wind canvas
    s_wind_canvas = layer_create(GRect(-4, 43, 20, 20));
    layer_add_child(s_forecast_first_layer, s_wind_canvas);
    layer_set_update_proc(s_wind_canvas, wind_update_proc);

    // First wind layer
    s_wind_first_layer = text_layer_create(GRect(23, 42, bounds.size.w-23, 21));

    // Get wind from forecast
    static char wind_speed_buffer_first[8];
    snprintf(wind_speed_buffer_first, sizeof(wind_speed_buffer_first),
        "%dkmh", forecast[0].WindSpeed);

    text_layer_set_background_color(s_wind_first_layer, GColorClear);
    text_layer_set_text_color(s_wind_first_layer, GColorWhite);
    text_layer_set_text_alignment(s_wind_first_layer, GTextAlignmentLeft);
    text_layer_set_text(s_wind_first_layer, wind_speed_buffer_first);

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
    s_rating_second_canvas = layer_create(GRect(-1, 5, bounds.size.w-5, 20));
    layer_add_child(s_forecast_second_layer, s_rating_second_canvas);
    layer_set_update_proc(s_rating_second_canvas, rating_second_update_proc);

    // Second wave canvas
    s_wave_canvas = layer_create(GRect(-4, 24, 20, 20));
    layer_add_child(s_forecast_second_layer, s_wave_canvas);
    layer_set_update_proc(s_wave_canvas, wave_update_proc);

    // Second swell layer
    s_swell_second_layer = text_layer_create(GRect(23, 23, bounds.size.w-23, 21));

    // Get Swell from forecast
    static char swell_period_buffer_second[8];
    snprintf(swell_period_buffer_second, sizeof(swell_period_buffer_second),
        "%ds", forecast[1].SwellPeriod);
    static char swell_buffer_second[16];
    snprintf(swell_buffer_second, sizeof(swell_buffer_second), "%s %s",
        forecast[1].SwellHeight, swell_period_buffer_second);

    // Style the text
    text_layer_set_background_color(s_swell_second_layer, GColorClear);
    text_layer_set_text_color(s_swell_second_layer, GColorWhite);
    text_layer_set_text_alignment(s_swell_second_layer, GTextAlignmentLeft);
    text_layer_set_text(s_swell_second_layer, swell_buffer_second);

    // Second wind canvas
    s_wind_canvas = layer_create(GRect(-4, 43, 20, 20));
    layer_add_child(s_forecast_second_layer, s_wind_canvas);
    layer_set_update_proc(s_wind_canvas, wind_update_proc);

    // second wind layer
    s_wind_second_layer = text_layer_create(GRect(23, 42, bounds.size.w-23, 21));

    // Get wind from forecast
    static char wind_speed_buffer_second[8];
    snprintf(wind_speed_buffer_second, sizeof(wind_speed_buffer_second),
        "%dkmh", forecast[1].WindSpeed);

    text_layer_set_background_color(s_wind_second_layer, GColorClear);
    text_layer_set_text_color(s_wind_second_layer, GColorWhite);
    text_layer_set_text_alignment(s_wind_second_layer, GTextAlignmentLeft);
    text_layer_set_text(s_wind_second_layer, wind_speed_buffer_second);

    // Apply forecast font and add textlayers to window
    text_layer_set_font(s_swell_second_layer, s_forecast_font);
    text_layer_set_font(s_wind_second_layer, s_forecast_font);
    layer_add_child(s_forecast_second_layer, text_layer_get_layer(s_swell_second_layer));
    layer_add_child(s_forecast_second_layer, text_layer_get_layer(s_wind_second_layer));

    // Add second forecast layer to window layer
    layer_add_child(window_layer, s_forecast_second_layer);
}

static void main_window_load(Window *window){
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

static void main_window_unload(Window *window){
    // Destroy TextLayers
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_swell_first_layer);
    text_layer_destroy(s_wind_first_layer);
    text_layer_destroy(s_swell_second_layer);
    text_layer_destroy(s_wind_second_layer);

    // Destroy rulers
    layer_destroy(s_ruler_first_layer);
    layer_destroy(s_ruler_second_layer);

    // Destroy bitmaps
    gbitmap_destroy(s_star_empty_bitmap);
    gbitmap_destroy(s_star_full_bitmap);
    gbitmap_destroy(s_wave_bitmap);
    gbitmap_destroy(s_wind_bitmap);

    // Destroy forecast layers
    layer_destroy(s_rating_first_canvas);
    layer_destroy(s_rating_second_canvas);
    layer_destroy(s_forecast_first_layer);
    layer_destroy(s_forecast_second_layer);
    layer_destroy(s_wave_canvas);
    layer_destroy(s_wind_canvas);
}

static void init(){
    load_forecast();
    load_settings();

    // Create main Window element and assign to pointer
    s_main_window = window_create();

    // Set the background color
    window_set_background_color(s_main_window, GColorBlack);

    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers){
						  .load = main_window_load,
						  .unload = main_window_unload});

    // Create GBitmaps
    s_star_empty_bitmap = gbitmap_create_with_resource(RESOURCE_ID_STAR_EMPTY_WHITE_20);
    s_star_full_bitmap = gbitmap_create_with_resource(RESOURCE_ID_STAR_FULL_WHITE_20);
    s_wave_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WAVE_WHITE_20);
    s_wind_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WIND_WHITE_20);

	// Show the Window on the watch, with animated=true
	window_stack_push(s_main_window, true);

    // Make sure the time is displayed from the start
    update_time();

    // Register with TickTimerService
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    // Register message callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    // Open AppMessage
    const int inbox_size = 256;
    const int outbox_size = 128;
    app_message_open(inbox_size, outbox_size);
}

static void deinit(){
    // Destroy Window
    window_destroy(s_main_window);
}

int main(void){
    init();
    app_event_loop();
    deinit();
}