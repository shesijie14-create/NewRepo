struct adc_device *p_adc_usb = NULL;
struct adc_device *p_adc_bat = NULL;

static struct lv_time {
	uint8 lv_hour;
	uint8 lv_min;
	uint8 lv_sec;
	uint8 res;
};

typedef enum
{
	CHARGER_IN,
    CHARGER_OUT, 
} CHARGE_OPT;
	
static uint8 charger_state = CHARGER_OUT;

lv_timer_t * record_timer;
struct lv_time time_msg;
struct lv_time time_msg_ext;
	
static lv_obj_t *status_bar_right = NULL;
static lv_obj_t *status_bar_left = NULL;
static lv_obj_t *status_bar_mid = NULL;

struct status_bar_pos_t
{
    char pos[20]; // 一个图标其实占3个字节 + '\0'
};

	// 图标数组（最多显示5个）
static struct status_bar_pos_t status_bar_pos_right[6] = {
	{.pos = '\0'},			  // 0					 "\xEF\x80\x8D"
	{.pos = '\0'},		 // 1						 "\xEF\x8B\xAD"
	{.pos = LV_SYMBOL_MY_SD}, 		// 2						"\xEF\x8a\x87" 
	{.pos = LV_SYMBOL_MY_MIC}, 	// 3 每个图标占3个字节				"\xEF\x9F\x82" 
	{.pos = LV_SYMBOL_MY_BAT_90}, // index 4  //状态栏3右对齐		 	 "\xEF\x89\x80" 
	{.pos = '\0'},
};

struct date_time_s
{
	int16 year;
	int16 month;
	int16 day;
	int16 hour;
	int16 minute;
	int16 second;
	int8 order;
	int8 flag_modify;
};


#define STATE_PROMPTS_SIZE(_state_prompts) (sizeof(_state_prompts) / sizeof(_state_prompts[0]))

static char status_bar_buf_right[STATE_PROMPTS_SIZE(status_bar_pos_right) * 4] = {0}; //

#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

//LV_FONT_DECLARE(hrong_icon);
LV_FONT_DECLARE(hy_zy_font);


#define battery_level0 3400
#define battery_level1 3800
#define battery_level2 4200

#define vbus_dectect_threshold 4000


static void btn_main_menu_handle(lv_event_t *e);
static void btn_setting_handler(lv_event_t * e);
static void btn_playback_rec_handle(lv_event_t *e);
static void btn_playback_photo_handle(lv_event_t *e);




























