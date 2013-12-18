#include <pebble.h>

#define DELAY 50
#define NUMSNOWFLAKES 150
#define MAXSPEED 3
#define FUZZYNESS 2
#define MAXSTEPS 10

static Window *window;
static Layer *rootLayer;
static Layer *layer;
static TextLayer *hour;
static GFont font;
static char hourText[6] = "     ";
static uint8_t h[144];

typedef struct {
	GPoint p;
	uint8_t vs, c, r;
} SnowFlake;

SnowFlake snowflakes[NUMSNOWFLAKES];

static inline void newSnowFlake(int i, bool start) {
	SnowFlake *s = &snowflakes[i];
	s->p.x = rand()%144;
	s->p.y = start?rand()%h[s->p.x]:0;
	s->vs = (uint8_t)(1 + rand()%MAXSPEED);
	s->c = rand()%MAXSTEPS;
	s->r = (uint8_t)((int)s->vs*2/MAXSPEED);
}

static void stackSnowFlake(int i) {
	i %= 144;
	int l = (i+143)%144;
	int r = (i+1)%144;
	
	if ((h[l] > h[i]) && (h[r] > h[i])) {
		if (rand()%2) {
			stackSnowFlake(l);
		} else {
			stackSnowFlake(r);
		}
	} else if (h[l] > h[i]) {
		stackSnowFlake(l);
	} else if (h[r] > h[i]) {
		stackSnowFlake(r);
	} else {
		h[i]--;
	}
}

static void updateScreen(Layer *layer, GContext* ctx) {
	AccelData a;
	SnowFlake *s;
	int i, d, w;
	
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);

	if (accel_service_peek(&a) < 0) {
		w = 0;
	} else {
		w = a.x/250;
	}
  	
	for (i=0; i<NUMSNOWFLAKES; i++) {
		s = &snowflakes[i];
		if (s->p.y >= h[s->p.x]) {
			newSnowFlake(i, false);
			for (d=s->p.x-s->r; d<=s->p.x+s->r; d++) {
				stackSnowFlake(d);
			}
		}
		
		s->c++;
		if (s->c < MAXSTEPS) {
			d = 0;
		} else {
			s->c = 0;
			d = rand()%FUZZYNESS;
			if (rand()%2) d = -d;
		}
		
		s->p.x += d + w;
		if (s->p.x < 0) {
			s->p.x += 144;
		}
		
		if (s->p.x >= 144) {
			s->p.x -= 144;
		}

		s->p.y += s->vs;
		
		graphics_fill_circle(ctx, s->p, s->r);
	}
	
	for (i=0; i<144; i++) {
		graphics_draw_line(ctx, GPoint(i, h[i]), GPoint(i, 168));
	}
}

static void timerCallback(void *data) {
	layer_mark_dirty(layer);
	
	app_timer_register(DELAY, timerCallback, NULL);
}

static void initSnowFlakes() {
	int i;
	for (i=0; i<NUMSNOWFLAKES; i++) {
		newSnowFlake(i, true);
	}		
}

static void initHeights() {
	int i;
	for (i=0; i<144; i++) {
		h[i] = 168;
	}
}

static inline void setHourText() {
	clock_copy_time_string(hourText, 6);
	text_layer_set_text(hour, hourText);
}
	
static void minuteChange(struct tm *tick_time, TimeUnits units_changed) {
	setHourText();
}

static void tapHandler(AccelAxisType axis, int32_t direction) {
	initHeights();
}

static void init(void) {
	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_stack_push(window, true);
	
	rootLayer = window_get_root_layer(window);
	
	srand(time(NULL));
	initHeights();
	initSnowFlakes();

	font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BORIS_47));
	hour = text_layer_create(GRect(0, 64, 144, 80));
	text_layer_set_background_color(hour, GColorClear);
	text_layer_set_text_color(hour, GColorWhite);
	text_layer_set_font(hour, font);
	text_layer_set_text_alignment(hour, GTextAlignmentCenter);
	layer_add_child(rootLayer, text_layer_get_layer(hour));
	setHourText();
	
	layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(layer, updateScreen);
	layer_add_child(rootLayer, layer);
	
	app_timer_register(DELAY, timerCallback, NULL);
	
	tick_timer_service_subscribe(MINUTE_UNIT, minuteChange);
	
	accel_data_service_subscribe(0, NULL);
	accel_tap_service_subscribe(tapHandler);
}

static void deinit(void) {
	accel_tap_service_unsubscribe();
	accel_data_service_unsubscribe();
	tick_timer_service_unsubscribe();
	layer_destroy(layer);
	text_layer_destroy(hour);
	fonts_unload_custom_font(font);
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}