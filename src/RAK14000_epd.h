/**
 * @file RAK14000_epd.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Images for the EPD display
 * @version 0.1
 * @date 2022-06-25
 *
 * @copyright Copyright (c) 2022
 * Images cortesy of <a href="https://www.flaticon.com/free-icons" title="Freepik - Flaticon">Icons created by Freepik - Flaticon</a>
 */

#ifndef _RAK14000_EPD_H_
#define _RAK14000_EPD_H_

#include <Adafruit_GFX.h>
#include <Adafruit_EPD.h>

// For text length calculations
extern int16_t txt_x1;
extern int16_t txt_y1;
extern uint16_t txt_w;
extern uint16_t txt_w2;
extern uint16_t txt_h;

// For text and image placements
extern uint16_t x_text;
extern uint16_t y_text;
extern uint16_t s_text;
extern uint16_t w_text;
extern uint16_t h_text;
extern uint16_t x_graph;
extern uint16_t y_graph;
extern uint16_t h_bar;
extern uint16_t w_bar;
extern float bar_divider;
extern uint16_t spacer;

extern Adafruit_SSD1681 display;

extern uint16_t bg_color;
extern uint16_t txt_color;

extern const uint16_t num_values;
extern uint16_t voc_values[];
extern float temp_values[];
extern float humid_values[];
extern float baro_values[];
extern float co2_values[];
extern uint16_t pm10_values[];
extern uint16_t pm25_values[];
extern uint16_t pm100_values[];
extern uint8_t voc_idx;
extern uint8_t temp_idx;
extern uint8_t humid_idx;
extern uint8_t baro_idx;
extern uint8_t co2_idx;
extern uint8_t pm_idx;

extern char disp_text[60];

extern uint16_t display_width;
extern uint16_t display_height;

extern char *months_txt[];

// Forward declaration
void text_rak14000(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size);
void scientific_rak14000(void);
void voc_rak14000(void);
void co2_rak14000(bool has_pm);
void pm_rak14000(void);
void temp_rak14000(bool has_pm, bool has_baro);
void humid_rak14000(bool has_pm, bool has_baro);
void baro_rak14000(bool has_pm);
void icon_rak14000(void);
void draw_bar_rak14000(uint8_t level, uint16_t x, uint16_t y);

extern unsigned char good_air[];
extern uint16_t good_air_width;
extern uint16_t good_air_height;
extern uint16_t good_air_len;

extern unsigned char bad_air[];
extern uint16_t bad_air_width;
extern uint16_t bad_air_height;
extern uint16_t bad_air_len;

extern unsigned char worried_air[];
extern uint16_t worried_air_width;
extern uint16_t worried_air_height;
extern uint16_t worried_air_len;

extern unsigned char rak_img[];
extern uint16_t rak_img_width;
extern uint16_t rak_img_height;
extern uint16_t rak_img_length;

extern unsigned char celsius_img[];
extern uint16_t celsius_img_width;
extern uint16_t celsius_img_height;
extern uint16_t celsius_img_length;

extern unsigned char humidity_img[];
extern uint16_t humidity_img_width;
extern uint16_t humidity_img_height;
extern uint16_t humidity_img_length;

extern unsigned char co2_img[];
extern uint16_t co2_img_width;
extern uint16_t co2_img_height;
extern uint16_t co2_img_length;

extern unsigned char barometer_img[];
extern uint16_t barometer_img_width;
extern uint16_t barometer_img_height;
extern uint16_t barometer_img_length;

extern unsigned char voc_img[];
extern uint16_t voc_img_width;
extern uint16_t voc_img_height;
extern uint16_t voc_img_length;

extern unsigned char pm_img[];
extern uint16_t pm_img_width;
extern uint16_t pm_img_height;
extern uint16_t pm_img_length;

extern unsigned char built_img[];
extern uint16_t built_img_width;
extern uint16_t built_img_height;
extern uint16_t built_img_length;

extern unsigned char wisblock_img[];
extern uint16_t wisblock_width;
extern uint16_t wisblock_height;
extern uint16_t wisblock_length;

extern uint8_t RAK_EPD_10pt_Bitmaps[];

extern GFXglyph RAK_EPD_10pt_Glyphs[];

extern GFXfont RAK_EPD_10pt;

extern uint8_t RAK_EPD_20pt_Bitmaps[];

extern GFXglyph RAK_EPD_20pt_Glyphs[];

extern GFXfont RAK_EPD_20pt;

#define SMALL_FONT &RAK_EPD_10pt
#define LARGE_FONT &RAK_EPD_20pt

#define POWER_ENABLE WB_IO2
#define EPD_MOSI MOSI
#define EPD_MISO -1 // not use
#define EPD_SCK SCK
#define EPD_CS SS
#define EPD_DC WB_IO1
#define SRAM_CS -1	 // not use
#define EPD_RESET -1 // not use
#define EPD_BUSY WB_IO4

#define RGB_BLUE 0,0,255
#define RGB_YELLOW 246,190,0
#define RGB_RED 255,0,0

#endif // _RAK14000_EPD_H_
