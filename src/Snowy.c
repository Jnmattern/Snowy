#include <pebble.h>

#define DELAY 50
#define MAXSPEED 3
#define FUZZYNESS 2
#define MAXSTEPS 10

//#define SHOWDATE

static Window *window;
static Layer *rootLayer;
static Layer *layer;
static GFont fontHour, fontDate;
static char *pebbleLocale;
static char hourText[6] = "     ";
static char dateText[15] = "               ";
static AppTimer *timer = NULL;
static GRect bgHourRect[4];

#if defined(SHOWDATE)
static GRect bgDateRect[4];
#endif

#if defined(PBL_RECT)

#define WIDTH 144
#define HEIGHT 168
#define NUMSNOWFLAKES 200
static GRect hourRect = { { 0, 60 }, {144, 80} };
#if defined(SHOWDATE)
static GRect dateRect = { { 0, 120 }, {144, 48} };
#endif

#elif defined(PBL_ROUND)

#define WIDTH 180
#define HEIGHT 180
#define NUMSNOWFLAKES 250
static GRect hourRect = { { 0, 66 }, {180, 80} };
#if defined(SHOWDATE)
static GRect dateRect = { { 0, 126 }, {180, 48} };
#endif

#endif

static uint8_t h[WIDTH];

typedef struct {
	GPoint p;
	uint8_t vs, c, r;
} SnowFlake;

SnowFlake snowflakes[NUMSNOWFLAKES];

static inline void newSnowFlake(int i, bool start) {
	SnowFlake *s = &snowflakes[i];
	s->p.x = rand()%WIDTH;
	s->p.y = start?rand()%h[s->p.x]:0;
	s->vs = (uint8_t)(1 + rand()%MAXSPEED);
	s->c = rand()%MAXSTEPS;
	s->r = (uint8_t)((int)s->vs*2/MAXSPEED);
}

static void initHeights() {
	int i;
	for (i=0; i<WIDTH; i++) {
		h[i] = HEIGHT;
	}
}

static void initSnowFlakes() {
	int i;
	for (i=0; i<NUMSNOWFLAKES; i++) {
		newSnowFlake(i, true);
	}
}

static inline void reset() {
	initHeights();
	initSnowFlakes();
}

static void initRects() {
	int i, dx, dy;
	for (i=0; i<4; i++) {
		dx = 2*(i%2) - 1;
		dy = 2*(i/2) - 1;
		bgHourRect[i] = GRect(hourRect.origin.x + 3*dx, hourRect.origin.y + 3*dy, hourRect.size.w, hourRect.size.h);
#if defined(SHOWDATE)
    bgDateRect[i] = GRect(dateRect.origin.x + 2*dx, dateRect.origin.y + 2*dy, dateRect.size.w, dateRect.size.h);
#endif
  }
}

static void stackSnowFlake(int i) {
	i = (i+WIDTH)%WIDTH;
	int l = (i+WIDTH-1)%WIDTH;
	int r = (i+1)%WIDTH;
	
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
		if (h[i] > 0) {
			h[i]--;
		} else {
			reset();
		}
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
			s->p.x += WIDTH;
		}
		
		if (s->p.x >= WIDTH) {
			s->p.x -= WIDTH;
		}

		s->p.y += s->vs;
		
		graphics_fill_circle(ctx, s->p, s->r);
	}
	
	for (i=0; i<WIDTH; i++) {
		graphics_draw_line(ctx, GPoint(i, h[i]), GPoint(i, HEIGHT));
	}
	
	graphics_context_set_text_color(ctx, GColorBlack);
	for (i=0; i<4; i++) {
		graphics_draw_text(ctx, hourText, fontHour, bgHourRect[i], GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
#if defined(SHOWDATE)
    graphics_draw_text(ctx, dateText, fontDate, bgDateRect[i], GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
#endif
	}
	graphics_context_set_text_color(ctx, GColorWhite);
	graphics_draw_text(ctx, hourText, fontHour, hourRect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
#if defined(SHOWDATE)
  graphics_draw_text(ctx, dateText, fontDate, dateRect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
#endif
}

static void timerCallback(void *data) {
	layer_mark_dirty(layer);
	
	timer = app_timer_register(DELAY, timerCallback, NULL);
}

static inline void setText() {
  time_t now = time(NULL);
  struct tm *theTime = localtime(&now);

	clock_copy_time_string(hourText, 6);
#if defined(SHOWDATE)
  strftime(dateText, sizeof(dateText), "%a %e %b", theTime);
#endif
}
	
static void minuteChange(struct tm *tick_time, TimeUnits units_changed) {
	setText();
}

static void tapHandler(AccelAxisType axis, int32_t direction) {
	reset();
}

static void init(void) {
  pebbleLocale = setlocale(LC_ALL, "");

	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_stack_push(window, true);
	
	rootLayer = window_get_root_layer(window);
	
	srand(time(NULL));
	
	layer = layer_create(GRect(0, 0, WIDTH, HEIGHT));
	layer_set_update_proc(layer, updateScreen);
	layer_add_child(rootLayer, layer);

  fontHour = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BORIS_47));
#if defined(SHOWDATE)
  fontDate = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BORIS_18));
#endif

	setText();
	initRects();
	reset();

	timer = app_timer_register(DELAY, timerCallback, NULL);
	
	tick_timer_service_subscribe(MINUTE_UNIT, minuteChange);
	
	accel_data_service_subscribe(0, NULL);
	accel_tap_service_subscribe(tapHandler);
}

static void deinit(void) {
	accel_tap_service_unsubscribe();
	accel_data_service_unsubscribe();
	if (timer != NULL) app_timer_cancel(timer);
	tick_timer_service_unsubscribe();
	layer_destroy(layer);
  fonts_unload_custom_font(fontHour);
  fonts_unload_custom_font(fontDate);
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
