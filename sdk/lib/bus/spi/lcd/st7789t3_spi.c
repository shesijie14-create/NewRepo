#include "lib/lcd/lcd.h"

#if LCD_ST7789T3_SPI_EN
#define CMD(x) {LCD_CMD, x}
#define DAT(x) {LCD_DAT, x}
#define DLY(x) {DELAY_MS, x}
#define END {LCD_TAB_END, LCD_TAB_END}

#define LCD_W 240
#define LCD_H 240

#define XOFFSET ((240-LCD_W)/2)
#define YOFFSET ((240-LCD_H)/2)
//#define YOFFSET ((320-LCD_H)/2)

uint8_t st7789t3_register_init_tab[][2] = {

DLY(10),                

CMD( 0x11),     

DLY(120),                

CMD( 0x36),     
DAT( 0x00),   

CMD( 0x3A),     
DAT( 0x05),   //0x06 rgb565

CMD( 0xB2),     
DAT( 0x1F),   
DAT( 0x1F),   
DAT( 0x00),   
DAT( 0x33),   
DAT( 0x33),   

CMD( 0xB7),     
DAT( 0x44),   

CMD( 0xBB),     
DAT( 0x1D),   

CMD( 0xC0),     
DAT( 0x2C),   

CMD( 0xC2),     
DAT( 0x01),   

CMD( 0xC3),     
DAT( 0x0F),   

CMD( 0xC6),     
DAT( 0x13),   //0x13:60Hz   

CMD( 0xD0),     
DAT( 0xA7),   

CMD( 0xD0),     
DAT( 0xA4),   
DAT( 0xA1),   

CMD( 0xD6),     
DAT( 0xA1),   //sleep in?,gate???GND

CMD( 0xE0),
DAT( 0xF0),
DAT( 0x01),
DAT( 0x06),
DAT( 0x06),
DAT( 0x09),
DAT( 0x14),
DAT( 0x34),
DAT( 0x44),
DAT( 0x4A),
DAT( 0x3A),
DAT( 0x14),
DAT( 0x14),
DAT( 0x29),
DAT( 0x2E),

CMD( 0xE1),
DAT( 0xF0),
DAT( 0x0D),
DAT( 0x10),
DAT( 0x0D),
DAT( 0x0B),
DAT( 0x15),
DAT( 0x33),
DAT( 0x43),
DAT( 0x4A),
DAT( 0x2B),
DAT( 0x17),
DAT( 0x16),
DAT( 0x2A),
DAT( 0x2F),

CMD( 0xE4),     
DAT( 0x1D),   //??240?gate  (N+1)*8
DAT( 0x00),   //??gate????
DAT( 0x00),   //?gate?????,bit4(TMG)??0

CMD( 0x21),     

CMD( 0x29),     
DLY(10),     
#if 1
CMD( 0x2A),     //Column Address Set
DAT( 0x00),   
DAT( 0x00),   //0
DAT( 0x00),   
DAT( 0xEF),   //239

CMD( 0x2B),     //Row Address Set
DAT( 0x00),   
DAT( 0x00),   //0
DAT( 0x00),   
DAT( 0xEF),   //239
#endif

CMD( 0x2C),     

END


};

lcddev_t lcdstruct = {
	.name = "st7789t3_spi",
	.lcd_bus_type = LCD_BUS_SPI4,
	.bus_width = LCD_BUS_WIDTH_1,
	.color_mode = LCD_MODE_565,
 	.osd_scan_mode = LCD_ROTATE_0,//LCD_ROTATE_270
    .scan_mode = LCD_ROTATE_0,//rotate 90
	.te_mode = 0xff,		   // te mode, 0xff:disable
	.colrarray = 0,			   // 0:_RGB_ 1:_RBG_,2:_GBR_,3:_GRB_,4:_BRG_,5:_BGR_
	// f(wr) = source_clk/div/2
	// f(wr) >= screen_w * screen_h * clk_per_pixel * 60
    .pclk = 60000000,
	.even_order = 0,
	.odd_order = 0,
	.lcd_data_mode = (0 << 31) | // data inversion mode
					 (2 << 24) | // data compress mode
					 (1 << 20) | // fifo mode
					 (0 << 17) | // output cycle 2 shift direction
					 (0 << 12) | // output cycle 2 shift bit
					 (0 << 11) | // output cycle 1 shift direction
					 (0 << 6) |	 // output cycle 1 shift bit
					 (0 << 5) |	 // output cycle 0 shift direction
					 (8 << 0),	 // output cycle 0 shift bit
    .screen_w = 240,
    .screen_h = 240,
    .video_x  = XOFFSET,
    .video_y  = YOFFSET,
    .video_w  = 240,
    .video_h  = 240,
	.osd_x = XOFFSET,
	.osd_y = YOFFSET,
	.osd_w = 240, // 0 : value will set to video_w  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.osd_h = 240, // 0 : value will set to video_h  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.init_table  = st7789t3_register_init_tab,
	.frame_table = NULL,
    .clk_per_pixel = 2,

	.pclk_inv = 1,

	.vlw = 0, // 0,
	.vbp = 0, // 12,
	.vfp = 0, // 12,
	.hlw = 0, // 1,
	.hbp = 0, // 14,
	.hfp = 0, // 13,

	.de_en = 1,
	.vs_en = 1,
	.hs_en = 1,
	.de_inv = 0xff,
	.hs_inv = 0,
	.vs_inv = 0,

	.brightness = 1,
	.saturation = 7,
	.contrast = 7,
	.contra_index = 8,

	.gamma_red = 3,
	.gamma_green = 3,
	.gamma_blue = 3,

};
#endif
