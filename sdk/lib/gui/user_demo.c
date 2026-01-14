#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/scale/hgscale.h"
#include "dev/jpg/hgjpg.h"
#include "dev/lcdc/hglcdc.h"
#include "osal/semaphore.h"
#include "lib/lcd/lcd.h"
#include "lib/lcd/gui.h"
#include "dev/vpp/hgvpp.h"
#include "dev/csi/hgdvp.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "hal/dma.h"
#include "video_app/video_app.h"
#include "resorce/language.h"
#include "lv_demo_widgets.h"
#include "openDML.h"
#include "osal/mutex.h"
#include "avidemux.h"
#include "osal/task.h"
#include <stdlib.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "user_demo.h"

extern gui_msg gui_cfg;
//extern volatile vf_cblk g_vf_cblk;
extern lv_group_t * group_golop;
extern lv_indev_t * indev_keypad;


lv_group_t * group_cur;
lv_group_t * group0_golop;
lv_group_t * group1_golop;
lv_group_t * group2_golop;
lv_group_t * group3_golop;
lv_group_t * group4_golop;
lv_group_t * group5_golop;
lv_group_t * group6_golop;
lv_group_t * group7_golop;
lv_group_t * group8_golop;
lv_group_t * group9_golop;
lv_obj_t * cur_obj;
lv_obj_t * page0_obj;
lv_obj_t * page1_obj;
lv_obj_t * page2_obj;
lv_obj_t * page3_obj;
lv_obj_t * page4_obj;
lv_obj_t * page5_obj;
lv_obj_t * page6_obj;
lv_obj_t * page7_obj;
lv_obj_t * page8_obj;
lv_obj_t * page9_obj;

lv_obj_t * setting_option_btn[LANGUAGE_STR_MAX];
lv_obj_t * resolution_btn[6];    //VGA,720P,1080P,2K,4K,8K
lv_obj_t * continous_btn[5];
lv_obj_t * list_setting_children;
lv_obj_t * setting_next_btn;
lv_obj_t * setting_back_btn;


//extern lv_font_t myfont;
uint8_t page_cur = 0;
uint8_t page_num;

void battery_adc_init(void)
{ 
    p_adc_bat = (struct adc_device*)dev_get(HG_ADC0_DEVID);

    if (!p_adc_bat) {
        return RET_ERR;
    }
    adc_open(p_adc_bat);
    adc_add_channel(p_adc_bat, PC_0);	

}

void battery_adc_getval(void)
{   

	uint32 pc0_adc_val=0;
	uint32 battery_voltage=0;

	static uint8 mycount=0;

	adc_get_value(p_adc_bat, PC_0, &pc0_adc_val);

	battery_voltage=((double)pc0_adc_val/2047)*3*2*1000;
	
	if(charger_state==CHARGER_OUT)
	{
	    if(battery_voltage<=battery_level0)
	   	{
	   		status_bar_right_update(4,LV_SYMBOL_MY_BAT_20);
	   	}
	    else if((battery_voltage>battery_level0)&&(battery_voltage<=battery_level1))
	   	{
	   		status_bar_right_update(4,LV_SYMBOL_MY_BAT_60);
	   	}
		else if((battery_voltage>battery_level1)&&(battery_voltage<=battery_level2))
		{
	   		status_bar_right_update(4,LV_SYMBOL_MY_BAT_90);
		}
		else if(battery_voltage>battery_level2)
		{
	   		status_bar_right_update(4,LV_SYMBOL_MY_BAT_100);
		}
	}
	

}

void usb_adc_init(void)
{ 
    p_adc_usb = (struct adc_device*)dev_get(HG_ADC0_DEVID);

    if (!p_adc_usb) {
        return RET_ERR;
    }
    adc_open(p_adc_usb);
    adc_add_channel(p_adc_usb, PB_9);	

}

void usb_adc_getval(void)
{   
	uint32 pb8_adc_val=0;
	uint32 vbus_detect_voltage=0;
    static uint8 old_state = CHARGER_OUT;
    static uint8 new_state = CHARGER_OUT;
    static uint8 switch1 = 0;

	adc_get_value(p_adc_usb, PB_9, &pb8_adc_val);

	vbus_detect_voltage=((double)pb8_adc_val/2047)*3*2*1000;

	if(charger_state==CHARGER_IN)
	{
		if(switch1==0)
		{
			status_bar_right_update(4,LV_SYMBOL_MY_BAT_CHARGE);
			switch1 = 1;
		}
		else
		{
			status_bar_right_update(4,LV_SYMBOL_MY_BAT_OPT_CHARGE);
			switch1 = 0;	
		}
	}

	if(vbus_detect_voltage>vbus_dectect_threshold)
	{
		new_state=CHARGER_IN;
	}
	else
	{
		new_state=CHARGER_OUT;
	}

	if((old_state==CHARGER_OUT)&&(new_state==CHARGER_IN))
	{
		charger_state=CHARGER_IN;

	}
	else if((old_state==CHARGER_IN)&&(new_state==CHARGER_OUT))
	{
		charger_state=CHARGER_OUT;
	}
	old_state = new_state;
	
}

void bms_monitor_run()
{

	while(1)
	{	
		usb_adc_getval();
        battery_adc_getval();
		
		if(charger_state==CHARGER_IN)
		{
			os_sleep_ms(2000);	
		}
		else
		{
			os_sleep_ms(5000);	
		}
    }
}  


void bms_monitor_task_init(void)
{

	battery_adc_init();
	usb_adc_init();

    os_task_create("bms_run", bms_monitor_run, NULL, OS_TASK_PRIORITY_BELOW_NORMAL, 0, NULL, 2048);
    os_sleep_ms(1);
}

 void sd_plugout_handle(void)
{

	status_bar_right_update(2,LV_SYMBOL_MY_NOSD);
	popup_create("SD Plug out!!", 3000); 
}

 void sd_plugin_handle(void)
 {
 
	 status_bar_right_update(2,LV_SYMBOL_MY_SD);
	 popup_create("SD Plug in!!", 3000);	
 }

void status_bar_left_update(const char * text1,const char * text2)
{//支持图标和字符串

	char str1[50] = "";
	//char str_720p[50] = "1080P";
	
	if(text1!=NULL)
		strcat(str1, text1); 
	if(text2!=NULL)
		strcat(str1, text2); 
	lv_label_set_text(status_bar_left, str1); 

}

void status_bar_mid_update(const char * text)
{//支持图标和字符串

	lv_label_set_text(status_bar_mid, text);		

}

void status_bar_right_update(uint8 index,     	char *prompts)
{
	char str1[50] = "";


	if(index>5)  //右侧状态栏最多支持5个图标
		return;

	memset(status_bar_pos_right[index].pos, 0, sizeof(status_bar_pos_right[index].pos));
	strcpy(status_bar_pos_right[index].pos, prompts);

	for (uint8_t i = 0; i < STATE_PROMPTS_SIZE(status_bar_pos_right); i++)
	{
		if (status_bar_pos_right[i].pos != '\0')
		{
			strcat(str1, status_bar_pos_right[i].pos); // 拼接字符串
		}
	}
	lv_label_set_text(status_bar_right, str1); // 刷新显示图标
	lv_label_set_recolor(status_bar_right,true);

}

static void lv_time_add(struct lv_time *time_now){
	if(time_now->lv_sec == 59){
		if(time_now->lv_min == 59){
			if(time_now->lv_hour >= 99){
				time_now->lv_hour = 0;
				time_now->lv_min  = 0;
				time_now->lv_sec  = 0;				
			}else{
				time_now->lv_hour++;
				time_now->lv_min  = 0;
				time_now->lv_sec  = 0;				
			}
		}else{
			time_now->lv_min++;
			time_now->lv_sec = 0;
		}
	}else{
		time_now->lv_sec++;
	}
}

void record_timer_cb_handle(void * t) 
{
    time_t now = time(NULL);
    struct tm *tt = localtime(&now);

	lv_time_add(&time_msg_ext);

    static char date_buf[11];
    snprintf(date_buf, sizeof(date_buf), "%04d-%02d-%02d", 
             tt->tm_year + 1900, tt->tm_mon + 1, tt->tm_mday);
    
    static char time_buf[9];
  //  snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d",time_msg_ext.lv_hour, time_msg_ext.lv_min, time_msg_ext.lv_sec);
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d",time_msg_ext.lv_min, time_msg_ext.lv_sec);

	status_bar_left_update(LV_SYMBOL_MY_RECORD,time_buf);

}

void status_bar_record_stop_time(void)
{

	if(record_timer)
		lv_timer_pause(record_timer);
}

void status_bar_record_start_time(void)
{

	if(record_timer)
		lv_timer_resume(record_timer);
}

void popup_timer_cb(lv_timer_t * timer) 
{

    lv_obj_del(timer->user_data);
    lv_timer_del(timer);
}


void popup_create(const char * msg, uint32_t close_time) 
{

    lv_obj_t * popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(popup, 200, 100);
    lv_obj_center(popup);
    
    lv_obj_t * label = lv_label_create(popup);
    lv_label_set_text(label, msg);
    lv_obj_center(label);
    
    lv_timer_t * timer = lv_timer_create(popup_timer_cb, close_time, popup);
    lv_timer_set_repeat_count(timer, 1);
}
////////////////////////////////

#if 1    //lv_page_main_menu_screen

void lv_page_main_menu_screen()
{	
	static lv_style_t style5;
	char str_720p[50] = "720P";

	group0_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group0_golop);
	group_cur = group0_golop;
	
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);

	page0_obj = lv_obj_create(lv_scr_act());	

	cur_obj = page0_obj;
	lv_obj_set_size(page0_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page0_obj, &style5, 0);
	lv_obj_set_style_text_font(page0_obj, &hy_zy_font, 0);

	lv_obj_t *btn_up = lv_btn_create(cur_obj);
	lv_obj_t *up_label = lv_label_create(btn_up);
	lv_label_set_text(up_label, "the first screen!!");

	lv_obj_center(up_label); 
	lv_obj_align(btn_up, LV_ALIGN_CENTER, 0, 20); 
	lv_obj_set_size(btn_up, 200, 100); 
    lv_obj_add_event_cb(btn_up, btn_main_menu_handle, LV_EVENT_ALL, NULL); 

	status_bar_right = lv_label_create(cur_obj);
	status_bar_left = lv_label_create(cur_obj);
	status_bar_mid = lv_label_create(cur_obj);
	
	static lv_style_t style;
	lv_style_init(&style);											   
	lv_style_reset(&style); 										   
	//lv_style_set_bg_color(&style, lv_color_make(0x00, 0x00, 0xff));			
	lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_RED)); 
	lv_style_set_text_letter_space(&style, 8);						  
	lv_style_set_text_letter_space(&style, 2);						   
	lv_style_set_text_line_space(&style, 0);						   
	lv_obj_add_style(status_bar_right, &style, 0);			   
	lv_obj_add_style(status_bar_left, &style, 0);				   
	lv_obj_add_style(status_bar_mid, &style, 0);						

	lv_obj_align(status_bar_right, LV_ALIGN_TOP_RIGHT, -8, 0); 
	lv_obj_align(status_bar_left, LV_ALIGN_TOP_LEFT, -8, 0);	 
	lv_obj_align(status_bar_mid, LV_ALIGN_TOP_MID, -8, 0);	   

	memset(status_bar_buf_right, 0, sizeof(status_bar_buf_right));
	for (uint8_t i = 0; i < STATE_PROMPTS_SIZE(status_bar_pos_right); i++)
	{
		if (status_bar_pos_right[i].pos != '\0')
		{
			strcat(status_bar_buf_right, status_bar_pos_right[i].pos); 
		}
	}

	lv_label_set_text(status_bar_right, status_bar_buf_right);
	lv_label_set_text(status_bar_left, LV_SYMBOL_MY_RECORD);
	lv_label_set_text(status_bar_mid, str_720p); 

	lv_group_add_obj(group0_golop, btn_up);

	record_timer = lv_timer_create(record_timer_cb_handle, 1000, NULL);
	lv_timer_pause(record_timer);

}

static void btn_main_menu_handle(lv_event_t *e)
{

	lv_event_code_t code = lv_event_get_code(e);
	uint8_t key_code;

	if(code == LV_EVENT_KEY){
		key_code = lv_indev_get_key(lv_indev_get_act());
		if(key_code == LV_KEY_LEFT){
			lv_page_select_interface(4);
		}
		if(key_code == LV_KEY_RIGHT){
			lv_page_select_interface(6);
		}		
		if(key_code == LV_KEY_UP){
			status_bar_record_stop_time();			
		}
		if(key_code == LV_KEY_DOWN){
		}			
		if(key_code == LV_KEY_ENTER){
		     status_bar_record_start_time();
		}
	}
	
}

#endif

////////////////////////////////////////
#if 1


void lv_page_playback_photo_screen(){
	static lv_style_t style2;
	static lv_style_t style3;
	static lv_style_t style4;
	static lv_style_t style5;
	static lv_style_t style6;
	char str_720p[50] = "720P";

	lv_obj_t * label;

	group5_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group5_golop);
	group_cur = group5_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	//cur_obj = lv_obj_create(lv_scr_act());
	page5_obj = lv_obj_create(lv_scr_act());//
	cur_obj = page5_obj;
	lv_obj_set_size(page5_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page5_obj, &style5, 0);
	lv_obj_set_style_text_font(page5_obj, &hy_zy_font, 0);

	lv_obj_t *btn_up = lv_btn_create(cur_obj);
	lv_obj_t *up_label = lv_label_create(btn_up);
	lv_label_set_text(up_label, "the second screen!!");

	lv_obj_center(up_label); 
	lv_obj_align(btn_up, LV_ALIGN_CENTER, 0, 20); 
	lv_obj_set_size(btn_up, 200, 100); 
    lv_obj_add_event_cb(btn_up, btn_playback_photo_handle, LV_EVENT_ALL, NULL); 

	status_bar_right = lv_label_create(cur_obj);
	status_bar_left = lv_label_create(cur_obj);
	status_bar_mid = lv_label_create(cur_obj);
	
	static lv_style_t style;
	lv_style_init(&style);											   
	lv_style_reset(&style); 										   
	//lv_style_set_bg_color(&style, lv_color_make(0x00, 0x00, 0xff));			
	lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_RED)); 
	lv_style_set_text_letter_space(&style, 8);						  
	lv_style_set_text_letter_space(&style, 2);						   
	lv_style_set_text_line_space(&style, 0);						   
	lv_obj_add_style(status_bar_right, &style, 0);			   
	lv_obj_add_style(status_bar_left, &style, 0);				   
	lv_obj_add_style(status_bar_mid, &style, 0);						

	lv_obj_align(status_bar_right, LV_ALIGN_TOP_RIGHT, -8, 0); 
	lv_obj_align(status_bar_left, LV_ALIGN_TOP_LEFT, -8, 0);	 
	lv_obj_align(status_bar_mid, LV_ALIGN_TOP_MID, -8, 0);	   

	memset(status_bar_buf_right, 0, sizeof(status_bar_buf_right));
	for (uint8_t i = 0; i < STATE_PROMPTS_SIZE(status_bar_pos_right); i++)
	{
		if (status_bar_pos_right[i].pos != '\0')
		{
			strcat(status_bar_buf_right, status_bar_pos_right[i].pos); 
		}
	}

	lv_label_set_text(status_bar_right, status_bar_buf_right);
	lv_label_set_text(status_bar_left, LV_SYMBOL_MY_RECORD);
	lv_label_set_text(status_bar_mid, str_720p); 
	
	lv_group_add_obj(group5_golop, btn_up);


}


static void btn_playback_photo_handle(lv_event_t *e)
{
	uint8_t key_code;
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_KEY){
		key_code = lv_indev_get_key(lv_indev_get_act());
		if(key_code == LV_KEY_LEFT){
			lv_page_select_interface(5);
		}
		if(key_code == LV_KEY_RIGHT){
		}		
		if(key_code == LV_KEY_UP){
		}
		if(key_code == LV_KEY_DOWN){
		}			
		if(key_code == LV_KEY_ENTER){
		}
	}
	
}



#endif

////////////////////////////////////////

#if 1


void lv_page_playback_rec_screen(){
	static lv_style_t style2;
	static lv_style_t style3;
	static lv_style_t style4;
	static lv_style_t style5;
	static lv_style_t style6;
	lv_obj_t * label;
	char str_720p[50] = "720P";
	
	group6_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group6_golop);
	group_cur = group6_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	//page6_obj = lv_obj_create(lv_scr_act());
	page6_obj = lv_obj_create(lv_scr_act());//
	cur_obj = page6_obj;
	lv_obj_set_size(page6_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page6_obj, &style5, 0);
	lv_obj_set_style_text_font(page6_obj, &hy_zy_font, 0);
	
	lv_obj_t *btn_up = lv_btn_create(cur_obj);
	lv_obj_t *up_label = lv_label_create(btn_up);
	lv_label_set_text(up_label, "the third screen!!");

	lv_obj_center(up_label); 
	lv_obj_align(btn_up, LV_ALIGN_CENTER, 0, 20); 
	lv_obj_set_size(btn_up, 200, 100); 
    lv_obj_add_event_cb(btn_up, btn_playback_rec_handle, LV_EVENT_ALL, NULL); 

	status_bar_right = lv_label_create(cur_obj);
	status_bar_left = lv_label_create(cur_obj);
	status_bar_mid = lv_label_create(cur_obj);
	
	static lv_style_t style;
	lv_style_init(&style);											   
	lv_style_reset(&style); 										   
	//lv_style_set_bg_color(&style, lv_color_make(0x00, 0x00, 0xff));			
	lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_RED)); 
	lv_style_set_text_letter_space(&style, 8);						  
	lv_style_set_text_letter_space(&style, 2);						   
	lv_style_set_text_line_space(&style, 0);						   
	lv_obj_add_style(status_bar_right, &style, 0);			   
	lv_obj_add_style(status_bar_left, &style, 0);				   
	lv_obj_add_style(status_bar_mid, &style, 0);						

	lv_obj_align(status_bar_right, LV_ALIGN_TOP_RIGHT, -8, 0); 
	lv_obj_align(status_bar_left, LV_ALIGN_TOP_LEFT, -8, 0);	 
	lv_obj_align(status_bar_mid, LV_ALIGN_TOP_MID, -8, 0);	   

	memset(status_bar_buf_right, 0, sizeof(status_bar_buf_right));
	for (uint8_t i = 0; i < STATE_PROMPTS_SIZE(status_bar_pos_right); i++)
	{
		if (status_bar_pos_right[i].pos != '\0')
		{
			strcat(status_bar_buf_right, status_bar_pos_right[i].pos); 
		}
	}

	lv_label_set_text(status_bar_right, status_bar_buf_right);
	lv_label_set_text(status_bar_left, LV_SYMBOL_MY_RECORD);
	lv_label_set_text(status_bar_mid, str_720p); 
	
	lv_group_add_obj(group6_golop, btn_up);


}


static void btn_playback_rec_handle(lv_event_t *e)
{
	uint8_t key_code;
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_KEY){
		key_code = lv_indev_get_key(lv_indev_get_act());
		if(key_code == LV_KEY_LEFT){
			lv_page_select_interface(0);
		}
		if(key_code == LV_KEY_RIGHT){
		}		
		if(key_code == LV_KEY_UP){
		}
		if(key_code == LV_KEY_DOWN){
		}			
		if(key_code == LV_KEY_ENTER){
		}
	}
	
}



#endif

////////////////////////////////////////

#if 1







#endif

////////////////////////////////////////

#if 1

void lv_page_setting_screen(){
	static lv_style_t style5;
	lv_obj_t *main_list;
	static lv_obj_t *status_bar;  
	uint16 lcd_width,lcd_height;
	
	//char str1[50] = "Hello";  //初始图标
	char str1[50] = "";  //初始图标	
	char str_720p[50] = "720P";
	
	group4_golop = lv_group_create();
	//lv_indev_set_group(indev_keypad, group4_golop);
	group_cur = group4_golop;
	lv_style_reset(&style5);
	lv_style_init(&style5);
	lv_style_set_bg_color(&style5, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&style5, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&style5,0);
	page4_obj = lv_obj_create(lv_scr_act());//NULL
	cur_obj = page4_obj;
	lv_obj_set_size(page4_obj, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(page4_obj, &style5, 0);
	lv_obj_set_style_text_font(page4_obj, &hy_zy_font, 0);

	status_bar = lv_obj_create(cur_obj);
	lv_obj_set_size(status_bar, SCALE_WIDTH, 60);	   // 宽度100%，高度40px
	lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x000000), LV_PART_MAIN); // 黑色背景
	lv_obj_set_style_text_color(status_bar, lv_color_white(), LV_PART_MAIN);
	lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);  // 顶部居中
	lv_obj_set_pos(status_bar, 0, -40);

	// 状态栏文本
	lcd_width=SCALE_WIDTH;
	lcd_height=SCALE_HIGH;

	status_bar_right = lv_label_create(status_bar);
	status_bar_left = lv_label_create(status_bar);
	status_bar_mid = lv_label_create(status_bar);
		
	static lv_style_t style;
	lv_style_init(&style);											   
	lv_style_reset(&style); 										   
	//lv_style_set_bg_color(&style, lv_color_make(0x00, 0x00, 0xff));			
	lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_RED)); 
	//lv_style_set_text_letter_space(&style, 8);						  
	lv_style_set_text_letter_space(&style, 1);						   
	lv_style_set_text_line_space(&style, 0);						   
	// lv_style_set_text_decor(&style, LV_TEXT_DECOR_UNDERLINE);		  
	lv_obj_add_style(status_bar_right, &style, 0);			   
	lv_obj_add_style(status_bar_left, &style, 0);				   
	lv_obj_add_style(status_bar_mid, &style, 0);						

	lv_obj_align(status_bar_right, LV_ALIGN_TOP_RIGHT, 0, 12); 
	lv_obj_align(status_bar_left, LV_ALIGN_TOP_LEFT, 0, 12);	 
	lv_obj_align(status_bar_mid, LV_ALIGN_TOP_MID, 0, 12);	   

	memset(status_bar_buf_right, 0, sizeof(status_bar_buf_right));
	for (uint8_t i = 0; i < STATE_PROMPTS_SIZE(status_bar_pos_right); i++)
	{
		if (status_bar_pos_right[i].pos != '\0')
		{
			strcat(status_bar_buf_right, status_bar_pos_right[i].pos); // 拼接字符串
		}
	}

	lv_label_set_text(status_bar_right, status_bar_buf_right); // 刷新显示图标 
	lv_label_set_text(status_bar_left, LV_SYMBOL_MY_MENU); // 	
	lv_label_set_text(status_bar_mid, str_720p); // 

	main_list = lv_list_create(page4_obj);
	lv_obj_add_flag(main_list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(main_list, LV_OBJ_FLAG_HIDDEN);
//	lv_obj_set_size(main_list, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_set_size(main_list, SCALE_WIDTH, SCALE_HIGH-60);  //设置list大小

	lv_obj_center(main_list);		
	lv_obj_t * btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][SOUND_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[SOUND_STR] = btn1;
	
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][TIME_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[TIME_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][RECORD_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[RECORD_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][TAKEPHOTO_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[TAKEPHOTO_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][FORMAT_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[FORMAT_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][CYCLE_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[CYCLE_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][BATTERY_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[BATTERY_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][LANGUAGE_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[LANGUAGE_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);

	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][CONTINUOUS_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[CONTINUOUS_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);

	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][USBDEV_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[USBDEV_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
	
	btn1 = lv_list_add_btn(main_list, NULL, (const char *)language_switch[language_cur][EXIT_STR]);
	lv_obj_add_event_cb(btn1, btn_setting_handler, LV_EVENT_ALL, NULL);
	setting_option_btn[EXIT_STR] = btn1;
	lv_group_add_obj(group4_golop, btn1);
		
}

static void format_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	struct jpg_device *jpeg_dev;	
	struct lcdc_device *lcd_dev;
	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	if(code == LV_EVENT_CLICKED) {
				
		if(e->target == setting_option_btn[YES_STR]){
			lv_page_select_interface(6);
		}
		
		if(e->target == setting_option_btn[NO_STR]){
			lv_page_select_interface(6);
		}
		lv_group_focus_obj(setting_option_btn[FORMAT_STR]);
	}
}

static void language_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[LANGUAGE_EN_STR]){
			language_cur = 0;
			lv_page_select_interface(6);
		}
		
		if(e->target == setting_option_btn[LANGUAGE_CN_STR]){
			language_cur = 1;
			lv_page_select_interface(6);
		}
		lv_group_focus_obj(setting_option_btn[LANGUAGE_STR]);
	}
}

static void sound_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[OPEN_STR]){
		//	gui_cfg.sound_en = 1;
			lv_page_select_interface(6);
		}
		
		if(e->target == setting_option_btn[CLOSE_STR]){
		//	gui_cfg.sound_en = 0;
			lv_page_select_interface(6);
		}
		lv_group_focus_obj(setting_option_btn[SOUND_STR]);
	}
}

static void usbdev_handler(lv_event_t * e)
{
	void hgusb_dev_recfg(uint8_t dev_tpye);
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[UDISK_STR]){
//			hgusb_dev_recfg(1);
			lv_page_select_interface(6);
		}
		
		if(e->target == setting_option_btn[UVC_STR]){
//			hgusb_dev_recfg(2);
			lv_page_select_interface(6);
		}
		lv_group_focus_obj(setting_option_btn[USBDEV_STR]);
	}
}

static void cycle_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == setting_option_btn[OPEN_STR]){
		//	gui_cfg.cycle_rec_en = 1;
			lv_page_select_interface(6);
		}
		
		if(e->target == setting_option_btn[CLOSE_STR]){
		//	gui_cfg.cycle_rec_en = 0;
			lv_page_select_interface(6);
		}
		lv_group_focus_obj(setting_option_btn[CYCLE_STR]);
	}
}

static void rec_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);	
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == resolution_btn[0]){
		//	gui_cfg.rec_w = 640;
		//	gui_cfg.rec_h = 480;
			lv_page_select_interface(6);
		}
		
		if(e->target == resolution_btn[1]){
		//	gui_cfg.rec_w = 1280;
		//	gui_cfg.rec_h = 720;
			lv_page_select_interface(6);
		}
		
		if(e->target == resolution_btn[2]){
		//	gui_cfg.rec_w = 1920;
		//	gui_cfg.rec_h = 1080;			
			lv_page_select_interface(6);
		}
		lv_group_focus_obj(setting_option_btn[RECORD_STR]);
	}
}

static void takephoto_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == resolution_btn[0]){
		//	gui_cfg.photo_w = 640;
		//	gui_cfg.photo_h = 480;
			lv_page_select_interface(6);
		}
		
		if(e->target == resolution_btn[1]){
		//	gui_cfg.photo_w = 1280;
		//	gui_cfg.photo_h = 720;
			lv_page_select_interface(6);
		}
		
		if(e->target == resolution_btn[2]){
		//	gui_cfg.photo_w = 1920;
		//	gui_cfg.photo_h = 1080;
			lv_page_select_interface(6);
		}

		if(e->target == resolution_btn[3]){
		//	gui_cfg.photo_w = 2560;
		//	gui_cfg.photo_h = 1440;
			lv_page_select_interface(6);
		}

		if(e->target == resolution_btn[4]){
		//	gui_cfg.photo_w = 3840;
		//	gui_cfg.photo_h = 2560;
			lv_page_select_interface(6);
		}

		if(e->target == resolution_btn[5]){
		//	gui_cfg.photo_w = 7680;
		//	gui_cfg.photo_h = 4320;
			lv_page_select_interface(6);
		}
		
		lv_group_focus_obj(setting_option_btn[TAKEPHOTO_STR]);
	}
}

static void continous_shot_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		
		if(e->target == continous_btn[0]){
			printf("take photo 1\r\n");
		//	gui_cfg.take_photo_num = 1;
			lv_page_select_interface(6);
		}
		
		if(e->target == continous_btn[1]){
			printf("take photo 2\r\n");
		//	gui_cfg.take_photo_num = 2;
			lv_page_select_interface(6);
		}
		
		if(e->target == continous_btn[2]){
			printf("take photo 5\r\n");
		//	gui_cfg.take_photo_num = 5;
			lv_page_select_interface(6);
		}

		if(e->target == continous_btn[3]){
			printf("take photo 8\r\n");
		//	gui_cfg.take_photo_num = 8;
			lv_page_select_interface(6);
		}

		if(e->target == continous_btn[4]){
			printf("take photo 10\r\n");
		//	gui_cfg.take_photo_num = 10;
			lv_page_select_interface(6);
		}
		
		lv_group_focus_obj(setting_option_btn[CONTINUOUS_STR]);
	}
}

void format_list(){
	lv_obj_t *list;
	lv_obj_t *msbbg;	
	
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);

	msbbg = lv_obj_create(cur_obj);

    lv_obj_set_size(msbbg, 200, 160);
	lv_obj_align(msbbg, LV_ALIGN_OUT_TOP_MID, 0, 0);
	lv_obj_set_pos(msbbg, 0, 20);

	lv_obj_t *label1 = lv_label_create(msbbg);

	setting_option_btn[YES_STR] = lv_btn_create(msbbg);
	setting_option_btn[NO_STR] = lv_btn_create(msbbg);

	lv_obj_t *btn_label11 = lv_label_create(setting_option_btn[YES_STR]);
	lv_obj_t *btn_label21 = lv_label_create(setting_option_btn[NO_STR]);		

	lv_obj_set_size(label1, 160, 80);
	lv_obj_set_size(setting_option_btn[YES_STR], 60, 30);
	lv_obj_set_size(setting_option_btn[NO_STR], 60, 30);

	lv_obj_set_pos(label1, 10, 50);
	lv_obj_set_pos(setting_option_btn[YES_STR], 10, 110);	 
	lv_obj_set_pos(setting_option_btn[NO_STR], 95, 110);

	lv_label_set_text(btn_label11, "OK");
	lv_label_set_text(btn_label21, "Canel");
	lv_label_set_text(label1, "Want format??");

	//lv_obj_set_style_text_font(label1, &myfont, 0);
	//lv_obj_set_style_text_font(label1, &lv_font_montserrat_18, 0);

	lv_obj_add_event_cb(setting_option_btn[YES_STR], format_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[YES_STR]);
	
	lv_obj_add_event_cb(setting_option_btn[NO_STR], format_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[NO_STR]);


}

void language_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	setting_option_btn[LANGUAGE_EN_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][LANGUAGE_EN_STR]);
	lv_obj_add_event_cb(setting_option_btn[LANGUAGE_EN_STR], language_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[LANGUAGE_EN_STR]);
	
	setting_option_btn[LANGUAGE_CN_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][LANGUAGE_CN_STR]);
	lv_obj_add_event_cb(setting_option_btn[LANGUAGE_CN_STR], language_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[LANGUAGE_CN_STR]);

}


void sound_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	setting_option_btn[OPEN_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][OPEN_STR]);
	lv_obj_add_event_cb(setting_option_btn[OPEN_STR], sound_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[OPEN_STR]);
	
	setting_option_btn[CLOSE_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][CLOSE_STR]);
	lv_obj_add_event_cb(setting_option_btn[CLOSE_STR], sound_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[CLOSE_STR]);

}

void usbdev_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	setting_option_btn[UDISK_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][UDISK_STR]);
	lv_obj_add_event_cb(setting_option_btn[UDISK_STR], usbdev_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[UDISK_STR]);
	
	setting_option_btn[UVC_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][UVC_STR]);
	lv_obj_add_event_cb(setting_option_btn[UVC_STR], usbdev_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[UVC_STR]);

}


void date_time_set_handle(lv_event_t * e)
{

	lv_event_code_t code = lv_event_get_code(e);
	uint8_t key_code;
    struct date_time_s *ui_s = (struct date_time_s *)lv_event_get_user_data(e);

	if(code == LV_EVENT_KEY){
		key_code = lv_indev_get_key(lv_indev_get_act());
		
		if(key_code == LV_KEY_UP){
						
			if(ui_s->flag_modify==0)
			{
                if(ui_s->order==0)
                {			
					ui_s->day=ui_s->day+1;
					if(ui_s->day>31)
						ui_s->day=1;
				}
				else if(ui_s->order==1)
				{			
					ui_s->month=ui_s->month+1;
					if(ui_s->month>12)
						ui_s->month=1;
				}
				else
				{			
					ui_s->year=ui_s->year+1;
				}
			
			}
			else if(ui_s->flag_modify==1)
			{
				if(ui_s->order==0)
				{			
					ui_s->month=ui_s->month+1;
					if(ui_s->month>12)
						ui_s->month=1;
				}
				else if(ui_s->order==1)
				{			
					ui_s->day=ui_s->day+1;
					if(ui_s->day>31)
						ui_s->day=1;
				}
				else
				{			
					ui_s->month=ui_s->month+1;
					if(ui_s->month>12)
						ui_s->month=1;
				}
			}
			else if(ui_s->flag_modify==2)
			{
				if(ui_s->order==0)
				{			
					ui_s->year=ui_s->year+1;
				}
				else if(ui_s->order==1)
				{			
					ui_s->year=ui_s->year+1;
				}
				else
				{			
					ui_s->day=ui_s->day+1;
					if(ui_s->day>31)
						ui_s->day=1;
				}
			}
			else if(ui_s->flag_modify==3)
			{
				ui_s->hour=ui_s->hour+1;
				if(ui_s->hour>23)
					ui_s->hour=0;

			}
			else if(ui_s->flag_modify==4)
			{
				ui_s->minute=ui_s->minute+1;
				if(ui_s->minute>59)
					ui_s->minute=0;
			}
			else if(ui_s->flag_modify==5)
			{
				ui_s->second=ui_s->second+1;
				if(ui_s->second>59)
					ui_s->second=0;
			}
			else 
			{
				ui_s->order=ui_s->order+1;
				if(ui_s->order>2)
					ui_s->order=0;
			}			
			date_time_set(ui_s->year,ui_s->month,ui_s->day,ui_s->hour,ui_s->minute,ui_s->second,ui_s->order,ui_s->flag_modify);
		}
		if(key_code == LV_KEY_DOWN){
						
			if(ui_s->flag_modify==0)
			{

				if(ui_s->order==0)
				{			
					ui_s->day=ui_s->day-1;
					if(ui_s->day<1)
						ui_s->day=31;
				}
				else if(ui_s->order==1)
				{			
					ui_s->month=ui_s->month-1;
					if(ui_s->month<1)
						ui_s->month=12;
				}
				else
				{			
					ui_s->year=ui_s->year-1;
				}
			}
			else if(ui_s->flag_modify==1)
			{

				if(ui_s->order==0)
				{			
					ui_s->month=ui_s->month-1;
					if(ui_s->month<1)
						ui_s->month=12;
				}
				else if(ui_s->order==1)
				{			
					ui_s->day=ui_s->day-1;
					if(ui_s->day<1)
						ui_s->day=31;
				}
				else
				{			
					ui_s->month=ui_s->month-1;
					if(ui_s->month<1)
						ui_s->month=12;
				}

			}
			else if(ui_s->flag_modify==2)
			{
				if(ui_s->order==0)
				{			
					ui_s->year=ui_s->year-1;
				}
				else if(ui_s->order==1)
				{			
					ui_s->year=ui_s->year-1;
				}
				else
				{			
					ui_s->day=ui_s->day-1;
					if(ui_s->day<1)
						ui_s->day=31;
				}

			}
			else if(ui_s->flag_modify==3)
			{
				ui_s->hour=ui_s->hour-1;
				if(ui_s->hour<0)
					ui_s->hour=23;

			}
			else if(ui_s->flag_modify==4)
			{
				ui_s->minute=ui_s->minute-1;
				if(ui_s->minute<0)
					ui_s->minute=59;
			}
			else  if(ui_s->flag_modify==5)
			{
				ui_s->second=ui_s->second-1;
				if(ui_s->second<0)
					ui_s->second=59;
			}
			else
			{
				ui_s->order=ui_s->order-1;
				if(ui_s->order<0)
					ui_s->order=2;

			}			
			date_time_set(ui_s->year,ui_s->month,ui_s->day,ui_s->hour,ui_s->minute,ui_s->second,ui_s->order,ui_s->flag_modify);
		}
		if(key_code == LV_KEY_ENTER){
			
			ui_s->flag_modify++;
		    if(ui_s->flag_modify>6)
				ui_s->flag_modify=0;
						
			date_time_set(ui_s->year,ui_s->month,ui_s->day,ui_s->hour,ui_s->minute,ui_s->second,ui_s->order,ui_s->flag_modify);			
		}
		if(key_code == LV_KEY_LEFT){
			
			lv_page_select_interface(6);
			
		}
	}
}

void date_time_set(int16 year,int16 month,int16 day,int16 hour,int16 minute,int16 second,int8 order,int8 flag_m)
{
	lv_obj_t *msbbg;

	static char time_str[50];
	static char date_str[50];
	static char set_str[20];

    struct date_time_s *ui_s = (struct date_time_s*)STREAM_LIBC_ZALLOC(sizeof(struct date_time_s));

 // if(cur_obj)
//	  lv_obj_del(cur_obj);
  
 // if(group_golop)
//	  lv_group_del(group_golop);

	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);

	msbbg = lv_obj_create(page4_obj);

	lv_obj_set_size(msbbg, 220, 200);
	lv_obj_align(msbbg, LV_ALIGN_OUT_TOP_MID, 0, 0);
    lv_obj_set_pos(msbbg, -5, 10);

	lv_obj_t *btn = lv_btn_create(msbbg);
    lv_obj_t *label1 = lv_label_create(msbbg);

 //   lv_obj_set_style_text_font(label1, &lv_font_montserrat_24, 0);

	lv_obj_set_size(label1, 160, 30);
	lv_obj_set_pos(label1, 20, 20);   

	if(flag_m==0)
	{
        if(order==0)
		{
			sprintf(date_str,"#f8fa2a %02d# / %02d / %04d",day,month,year);	
		}
		else if(order==1)
		{
			sprintf(date_str,"#f8fa2a %02d# / %02d / %04d",month,day,year);
		}
		else if(order==2)
		{
			sprintf(date_str,"#f8fa2a %04d# / %02d / %02d",year,month,day);
		}
	}
	else if(flag_m==1)
	{

        if(order==0)
		{
			sprintf(date_str,"%02d / #f8fa2a %02d# / %04d",day,month,year);
		}
		else if(order==1)
		{
			sprintf(date_str,"%02d / #f8fa2a %02d# / %04d",month,day,year);
		}
		else if(order==2)
		{
			sprintf(date_str,"%04d / #f8fa2a %02d# / %02d",year,month,day);
		}
	}
	else if(flag_m==2)
	{
        if(order==0)
		{
			sprintf(date_str,"%02d / %02d / #f8fa2a %04d#",day,month,year);
		}
		else if(order==1)
		{
			sprintf(date_str,"%02d / %02d / #f8fa2a %04d#",month,day,year);
		}
		else if(order==2)
		{
			sprintf(date_str,"%04d / %02d / #f8fa2a %02d#",year,month,day);
		}
	}
	else 
	{
        if(order==0)
		{
			sprintf(date_str,"%02d / %02d / %04d",day,month,year);
		}
		else if(order==1)
		{
			sprintf(date_str,"%02d / %02d / %04d",month,day,year);
		}
		else if(order==2)
		{
			sprintf(date_str,"%04d / %02d / %02d",year,month,day);
		}
	}

	lv_label_set_text(label1,date_str);
	lv_label_set_recolor(label1,true);

    lv_obj_t *label2 = lv_label_create(msbbg);
 //   lv_obj_set_style_text_font(label2, &lv_font_montserrat_24, 0);

	lv_obj_set_size(label2, 160, 30);
	lv_obj_set_pos(label2, 20, 80);   

	if(flag_m==3)
	{
		sprintf(time_str,"#f8fa2a %02d# : %02d : %02d",hour,minute,second);
	}
	else if(flag_m==4)
	{
		sprintf(time_str,"%02d : #f8fa2a %02d# : %02d",hour,minute,second);
	}
	else if(flag_m==5)
	{
		sprintf(time_str,"%02d :  %02d : #f8fa2a %02d#",hour,minute,second);
	}
	else 
	{
		sprintf(time_str,"%02d : %02d : %02d",hour,minute,second);
	}

	lv_label_set_text(label2,time_str);
	lv_label_set_recolor(label2,true);

    lv_obj_t *label3 = lv_label_create(msbbg);
//    lv_obj_set_style_text_font(label3, &lv_font_montserrat_24, 0);

	lv_obj_set_size(label3, 160, 30);
	lv_obj_set_pos(label3, 20, 140);   

	if(flag_m==6)
	{
        if(order==0)
		{
			lv_label_set_text(label3, "#f8fa2a DD/MM/YY#"); 		
		}
		else if(order==1)
		{
			lv_label_set_text(label3, "#f8fa2a MM/DD/YY#"); 		
		}
		else if(order==2)
		{
			lv_label_set_text(label3, "#f8fa2a YY/MM/DD#"); 		
		}
	}
	else 
	{
        if(order==0)
		{
			lv_label_set_text(label3, "DD/MM/YY"); 
		}
		else if(order==1)
		{
			lv_label_set_text(label3, "MM/DD/YY"); 
		}
		else if(order==2)
		{
			lv_label_set_text(label3, "YY/MM/DD"); 
		}		
	}
	
	lv_label_set_recolor(label3,true);

	ui_s->year=year;
	ui_s->month=month;
	ui_s->day=day;
	ui_s->hour=hour;
	ui_s->minute=minute;
	ui_s->second=second;
	ui_s->order=order;
	ui_s->flag_modify=flag_m;

	lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN); 

	lv_group_add_obj(group_golop, btn);
	lv_obj_add_event_cb(btn, date_time_set_handle, LV_EVENT_ALL, ui_s);
	lv_group_focus_obj(btn);
	
}

void date_time_list(void)
{
	int16 year,month,day,hour,minute,second;
	int8 order,flag_modify;
	
	year=2025;
	month=12;
	day=19;
	
	hour=20;
	minute=56;
	second=2;
	
	order=0;
	flag_modify=0;

	date_time_set(year,month,day,hour,minute,second,order,flag_modify);

}

void rec_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	resolution_btn[0] = lv_list_add_btn(list, NULL, "VGA");
	lv_obj_add_event_cb(resolution_btn[0], rec_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[0]);
	
	resolution_btn[1] = lv_list_add_btn(list, NULL, "720P");
	lv_obj_add_event_cb(resolution_btn[1], rec_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[1]);
	
	resolution_btn[2] = lv_list_add_btn(list, NULL, "1080P");
	lv_obj_add_event_cb(resolution_btn[2], rec_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[2]);

}

void takephoto_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);	
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
	lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	resolution_btn[0] = lv_list_add_btn(list, NULL, "VGA");
	lv_obj_add_event_cb(resolution_btn[0], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[0]);
	
	resolution_btn[1] = lv_list_add_btn(list, NULL, "720P");
	lv_obj_add_event_cb(resolution_btn[1], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[1]);
	
	resolution_btn[2] = lv_list_add_btn(list, NULL, "1080P");
	lv_obj_add_event_cb(resolution_btn[2], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[2]);

	resolution_btn[3] = lv_list_add_btn(list, NULL, "2K");
	lv_obj_add_event_cb(resolution_btn[3], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[3]);

	resolution_btn[4] = lv_list_add_btn(list, NULL, "4K");
	lv_obj_add_event_cb(resolution_btn[4], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[4]);

	resolution_btn[5] = lv_list_add_btn(list, NULL, "8K");
	lv_obj_add_event_cb(resolution_btn[5], takephoto_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, resolution_btn[5]);

}

void cycle_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);  
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
    lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	setting_option_btn[OPEN_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][OPEN_STR]);
	lv_obj_add_event_cb(setting_option_btn[OPEN_STR], cycle_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[OPEN_STR]);
	
	setting_option_btn[CLOSE_STR] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][CLOSE_STR]);
	lv_obj_add_event_cb(setting_option_btn[CLOSE_STR], cycle_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, setting_option_btn[CLOSE_STR]);

}

void continous_shot_list(){
	lv_obj_t *list;
	if(group_golop)
		lv_group_del(group_golop);
	
	group_golop = lv_group_create();
	lv_indev_set_group(indev_keypad, group_golop);
	
	list = lv_list_create(page4_obj);
	list_setting_children = list;
	lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);	
	lv_obj_clear_flag(list, LV_OBJ_FLAG_HIDDEN);		
	lv_obj_set_size(list, 160, 160);
	lv_obj_align(list, LV_ALIGN_OUT_TOP_MID, 80, 0);
	continous_btn[0] = lv_list_add_btn(list, NULL, (const char *)language_switch[language_cur][CLOSE_STR]);
	lv_obj_add_event_cb(continous_btn[0], continous_shot_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, continous_btn[0]);
	
	continous_btn[1] = lv_list_add_btn(list, NULL, "2");
	lv_obj_add_event_cb(continous_btn[1], continous_shot_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, continous_btn[1]);
	
	continous_btn[2] = lv_list_add_btn(list, NULL, "5");
	lv_obj_add_event_cb(continous_btn[2], continous_shot_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, continous_btn[2]);

	continous_btn[3] = lv_list_add_btn(list, NULL, "8");
	lv_obj_add_event_cb(continous_btn[3], continous_shot_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, continous_btn[3]);

	continous_btn[4] = lv_list_add_btn(list, NULL, "10");
	lv_obj_add_event_cb(continous_btn[4], continous_shot_handler, LV_EVENT_ALL, NULL);
	lv_group_add_obj(group_golop, continous_btn[4]);


}


void page_setting_analyze(lv_event_t * e){
	if(e->target == setting_back_btn){
		//lv_page_select_interface(0);
		page_num = 0;
	}
	
	if(e->target == setting_next_btn){
		page_num++;
		if(page_num == 9)
			page_num = 1;
		lv_page_select_interface(page_num);
	}

}

static void event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	struct lcdc_device *lcd_dev;
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		if(page_cur == 0){
			//page0_analyze(e);
		}else if(page_cur == 1){
			//page_rec_analyze(e);
		}else if(page_cur == 2){
			//page_takephoto_analyze(e);
		}else if(page_cur == 3){
			//page_wifi_analyze(e);
		}else if(page_cur == 4){
			//page_playback_photo_analyze(e);
		}else if(page_cur == 5){
			//page_playback_rec_analyze(e);
		}else if(page_cur == 6){
			page_setting_analyze(e);
		}		
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}

}

static void btn_setting_handler(lv_event_t * e)
{
	uint8_t key_code;
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	
		if(e->target == setting_option_btn[FORMAT_STR]){
			format_list();
		}

		if(e->target == setting_option_btn[LANGUAGE_STR]){
			language_list();
		}
		
		if(e->target == setting_option_btn[SOUND_STR]){
			sound_list();
		}	

		if(e->target == setting_option_btn[TIME_STR]){
			date_time_list();
		}

		if(e->target == setting_option_btn[RECORD_STR]){
			rec_list();
		}

		if(e->target == setting_option_btn[TAKEPHOTO_STR]){
			takephoto_list();
		}
		
		if(e->target == setting_option_btn[CYCLE_STR]){
			cycle_list();
		}

		if(e->target == setting_option_btn[CONTINUOUS_STR]){
			continous_shot_list();
		}

		if(e->target == setting_option_btn[USBDEV_STR]){
			//usbdev_list();
		}

		if(e->target == setting_option_btn[EXIT_STR]){
			lv_page_select_interface(6);
		}
		
	}

	if(code == LV_EVENT_KEY){
		key_code = lv_indev_get_key(lv_indev_get_act());
		if(key_code == LV_KEY_LEFT){
		}
		if(key_code == LV_KEY_RIGHT){
			lv_page_select_interface(0);
		}		
		if(key_code == LV_KEY_UP){
		}
		if(key_code == LV_KEY_DOWN){
		}			
		if(key_code == LV_KEY_ENTER){
		}
	}

}







#endif


/////////////////////

void lv_page_select_interface(uint8_t page)
{
	uint8_t name[16];

	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	struct jpg_device *jpg_dev;
	
	page_cur = page;

	if(page == 0){
		page_num = 0;	
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		lv_page_main_menu_screen();		
		lv_indev_set_group(indev_keypad, group0_golop);
	}
	else if(page == 1){
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		//lv_page_rec_screen();			
		lv_indev_set_group(indev_keypad, group1_golop);
	}
	else if(page == 2){
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
//		lv_page_photo_screen();		
		lv_indev_set_group(indev_keypad, group2_golop);
	}
	else if(page == 3){
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
	//	lv_page_wifi_screen();
		lv_indev_set_group(indev_keypad, group3_golop);	
	}
	else if(page == 4){
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}		
		lv_page_playback_photo_screen();
		lv_indev_set_group(indev_keypad, group5_golop);	
	}
	else if(page == 5){
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}
		lv_page_playback_rec_screen();		
		lv_indev_set_group(indev_keypad, group6_golop);		
	}
	else if(page == 6){
		if(list_setting_children){
			lv_obj_del(list_setting_children);
			list_setting_children = NULL;
		}
		if(cur_obj){
			lv_obj_del(cur_obj);
			lv_group_del(group_cur);
			cur_obj = NULL;
		}		
		lv_page_setting_screen();
		lv_indev_set_group(indev_keypad, group4_golop);		
	}	
	
}


void lv_page_test(void)
{

//	lv_page_select_interface(6);

	lv_page_select_interface(0);


}









