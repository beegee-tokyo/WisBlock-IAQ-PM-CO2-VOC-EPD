/**
 * @file RAK14000_epd_4_2.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialization and functions for EPD display
 * @version 0.1
 * @date 2022-06-25
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"

#include "RAK14000_epd.h"

uint16_t display_width = 400;
uint16_t display_height = 300;

// 4.2" EPD with SSD1683
Adafruit_SSD1681 display(display_height, display_width, EPD_MOSI,
						 EPD_SCK, EPD_DC, EPD_RESET,
						 EPD_CS, SRAM_CS, EPD_MISO,
						 EPD_BUSY);

/** Set num_values to 1/4 of the display width */

const uint16_t num_values = 400 / 4;
uint16_t voc_values[num_values] = {0};
float temp_values[num_values] = {0.0};
float humid_values[num_values] = {0.0};
float baro_values[num_values] = {0.0};
float co2_values[num_values] = {0.0};
uint16_t pm10_values[num_values] = {0};
uint16_t pm25_values[num_values] = {0};
uint16_t pm100_values[num_values] = {0};
uint8_t voc_idx = 0;
uint8_t temp_idx = 0;
uint8_t humid_idx = 0;
uint8_t baro_idx = 0;
uint8_t co2_idx = 0;
uint8_t pm_idx = 0;

char disp_text[60];

uint16_t bg_color = EPD_WHITE;
uint16_t txt_color = EPD_BLACK;

bool g_epd_off = false;

/** EPD task handle */
TaskHandle_t epd_task_handle;

/** Semaphore for EPD display update */
SemaphoreHandle_t g_epd_sem;

/** Required for Semaphore from ISR */
static BaseType_t xHigherPriorityTaskWoken = pdTRUE;

/** Task declaration */
void epd_task(void *pvParameters);

// For text length calculations
int16_t txt_x1;
int16_t txt_y1;
uint16_t txt_w;
uint16_t txt_w2;
uint16_t txt_h;

// For text and image placements
uint16_t x_text;
uint16_t y_text;
uint16_t s_text;
uint16_t w_text;
uint16_t h_text;
uint16_t x_graph;
uint16_t y_graph;
uint16_t h_bar;
uint16_t w_bar;
float bar_divider;
uint16_t spacer;

/** Timer to switch off display */
SoftwareTimer display_off;

/** UI selector. 0 = scientific, 1 = Icon, 2 = Status */
uint8_t g_ui_selected = 0;

/**
 * @brief Initialization of RAK14000 EPD
 *
 */
void init_rak14000(void)
{
	pinMode(POWER_ENABLE, INPUT_PULLUP);
	digitalWrite(POWER_ENABLE, HIGH);

	g_epd_off = false;

	// Create the EPD event semaphore
	g_epd_sem = xSemaphoreCreateBinary();
	// Initialize semaphore
	xSemaphoreGive(g_epd_sem);
	// Take semaphore
	xSemaphoreTake(g_epd_sem, 10);
	if (!xTaskCreate(epd_task, "EPD", 4096, NULL, TASK_PRIO_LOW, &epd_task_handle))
	{
		MYLOG("EPD", "Failed to start EPD task");
	}
	MYLOG("EPD", "Initialized 4.2\" display");

	// Get saved UI selection
	read_ui_settings();

	// Prepare display off timer
	display_off.begin(10000, shut_down_rak14000, NULL, false);
}

/**
 * @brief Wake task to handle screen updates
 *
 */
void wake_rak14000(void)
{
	xSemaphoreGiveFromISR(g_epd_sem, &xHigherPriorityTaskWoken);
}

/**
   @brief Write a text on the display

   @param x x position to start
   @param y y position to start
   @param text text to write
   @param text_color color of text
   @param text_size size of text
*/
void text_rak14000(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size)
{
	if (text_size == 1)
	{
		display.setFont(SMALL_FONT); // Font_5x7_practical8pt7b
		y = y + 7;
	}
	else
	{
		display.setFont(LARGE_FONT);
		y = y + 12;
	}
	display.setCursor(x, y);
	display.setTextColor(text_color);
	display.setTextSize(1);
	display.setTextWrap(false);
	display.print(text);
}

/**
 * @brief Clear display content
 *
 */
void clear_rak14000(void)
{
	display.clearBuffer();
	display.fillRect(0, 0, display_width, display_height, bg_color);
}

/**
 * @brief Switch the UI to the next version.
 *			Triggered by button
 *
 */
void switch_ui(void)
{
	g_ui_selected += 1;
	if (g_ui_selected == 3)
	{
		g_ui_selected = 0;
	}
	wake_rak14000();
}

/** Flag for first screen update */
bool first_time = true;

char *months_txt[] = {(char *)"Jan", (char *)"Feb", (char *)"Mar", (char *)"Apr", (char *)"May", (char *)"Jun", (char *)"Jul", (char *)"Aug", (char *)"Sep", (char *)"Oct", (char *)"Nov", (char *)"Dec"};

/**
 * @brief Update screen content
 *
 */
void refresh_rak14000(void)
{
	// Clear display buffer
	clear_rak14000();

	switch (g_ui_selected)
	{
	case 0:
		scientific_rak14000();
		break;
	case 1:
		icon_rak14000();
		break;
	case 2:
		status_ui_rak14000();
		break;
	}

	delay(100);
	display.display();
	delay(100);
}

/**
 * @brief Display device status
 *
 */
void status_rak14000(void)
{
	// Clear display buffer
	clear_rak14000();

	if (found_sensors[RTC_ID].found_sensor)
	{
		read_rak12002();

		snprintf(disp_text, 59, "Status RAK10702   %s %d %d %02d:%02d",
				 months_txt[g_date_time.month - 1], g_date_time.date, g_date_time.year,
				 g_date_time.hour, g_date_time.minute);
	}

	display.getTextBounds(disp_text, 0, 0, &txt_x1, &txt_y1, &txt_w, &txt_h);
	text_rak14000((display_width / 2) - (txt_w / 2), 1, disp_text, (uint16_t)txt_color, 1);
}

/**
 * @brief Add VOC value to buffer
 *
 * @param voc_value new VOC value
 */
void set_voc_rak14000(uint16_t voc_value)
{
	MYLOG("EPD", "VOC set to %d at index %d", voc_value, voc_idx);
	// Shift values if necessary
	if (voc_idx == num_values)
	{
		for (int idx = 0; idx < (num_values - 1); idx++)
		{
			voc_values[idx] = voc_values[idx + 1];
		}
		voc_idx = (num_values - 1);
	}

	// Fill VOC array
	voc_values[voc_idx] = voc_value;

	// Increase index
	voc_idx++;
}

/**
 * @brief Add temperature value to buffer
 *
 * @param temp_value new temperature value
 */
void set_temp_rak14000(float temp_value)
{
	MYLOG("EPD", "Temp set to %.2f at index %d", temp_value, temp_idx);
	// Shift values if necessary
	if (temp_idx == num_values)
	{
		for (int idx = 0; idx < (num_values - 1); idx++)
		{
			temp_values[idx] = temp_values[idx + 1];
		}
		temp_idx = (num_values - 1);
	}

	// Fill Temperature array
	temp_values[temp_idx] = temp_value;

	// Increase index
	temp_idx++;
}

/**
 * @brief Add humidity value to buffer
 *
 * @param humid_value new humidity value
 */
void set_humid_rak14000(float humid_value)
{
	MYLOG("EPD", "Humid set to %.2f at index %d", humid_value, humid_idx);
	// Shift values if necessary
	if (humid_idx == num_values)
	{
		for (int idx = 0; idx < (num_values - 1); idx++)
		{
			humid_values[idx] = humid_values[idx + 1];
		}
		humid_idx = (num_values - 1);
	}

	// Fill VOC array
	humid_values[humid_idx] = humid_value;

	// Increase index
	humid_idx++;
}

/**
 * @brief Add CO2 value to buffer
 *
 * @param co2_value new CO2 value
 */
void set_co2_rak14000(float co2_value)
{
	MYLOG("EPD", "CO2 set to %.2f at index %d", co2_value, co2_idx);
	// Shift values if necessary
	if (co2_idx == num_values)
	{
		for (int idx = 0; idx < (num_values - 1); idx++)
		{
			co2_values[idx] = co2_values[idx + 1];
		}
		co2_idx = (num_values - 1);
	}

	// Fill VOC array
	co2_values[co2_idx] = co2_value;

	// Increase index
	co2_idx++;
}

/**
 * @brief Add barometric pressure to buffer
 *
 * @param baro_value new barometric pressure
 */
void set_baro_rak14000(float baro_value)
{
	MYLOG("EPD", "Baro set to %.2f at index %d", baro_value, baro_idx);
	// Shift values if necessary
	if (baro_idx == num_values)
	{
		for (int idx = 0; idx < (num_values - 1); idx++)
		{
			baro_values[idx] = baro_values[idx + 1];
		}
		baro_idx = (num_values - 1);
	}

	// Fill Barometer array
	baro_values[baro_idx] = baro_value;

	// Increase index
	baro_idx++;
}

/**
 * @brief Add PM values to array
 *
 * @param pm10_env new PM 1.0 value
 * @param pm25_env new PM 2.5 value
 * @param pm100_env new PM 10 value
 */
void set_pm_rak14000(uint16_t pm10_env, uint16_t pm25_env, uint16_t pm100_env)
{
	MYLOG("EPD", "PM set to %d %d %d  at index %d", pm10_env, pm25_env, pm100_env, pm_idx);
	// Shift values if necessary
	if (pm_idx == num_values)
	{
		for (int idx = 0; idx < (num_values - 1); idx++)
		{
			pm10_values[idx] = pm10_values[idx + 1];
			pm25_values[idx] = pm25_values[idx + 1];
			pm100_values[idx] = pm100_values[idx + 1];
		}
		pm_idx = (num_values - 1);
	}

	// Fill PM array
	pm10_values[pm_idx] = pm10_env;
	pm25_values[pm_idx] = pm25_env;
	pm100_values[pm_idx] = pm100_env;

	// Increase index
	pm_idx++;
}

void rak14000_start_screen(bool startup)
{
	// Clear display
	display.clearBuffer();

	// Draw Welcome Logo
	display.fillRect(0, 0, display_width, display_height, bg_color);
	display.drawBitmap(display_width / 2 - 75, 50, rak_img, 184, 56, txt_color); // 184x56

	display.setFont(SMALL_FONT);
	display.setTextSize(1);

	// If RTC is available, write the date
	if (found_sensors[RTC_ID].found_sensor)
	{
		read_rak12002();
		snprintf(disp_text, 59, "%d/%d/%d %d:%d", g_date_time.date, g_date_time.month, g_date_time.year,
				 g_date_time.hour, g_date_time.minute);
		text_rak14000(0, 0, disp_text, (uint16_t)txt_color, 1);
	}

	display.setFont(LARGE_FONT);
	display.setTextSize(1);
	display.getTextBounds((char *)"IoT Made Easy", 0, 0, &txt_x1, &txt_y1, &txt_w, &txt_h);
	text_rak14000(display_width / 2 - (txt_w / 2), 110, (char *)"IoT Made Easy", (uint16_t)txt_color, 2);

	display.setFont(LARGE_FONT);
	display.setTextSize(1);
	display.getTextBounds((char *)"RAK10702 Air Quality", 0, 0, &txt_x1, &txt_y1, &txt_w, &txt_h);
	text_rak14000(display_width / 2 - (txt_w / 2), 150, (char *)"RAK10702 Air Quality", (uint16_t)txt_color, 2);

	display.drawBitmap(display_width / 2 - 63, 190, built_img, 126, 66, txt_color);

	display.setFont(SMALL_FONT);
	display.setTextSize(1);
	if (startup)
	{
		display.getTextBounds((char *)"Wait for connect", 0, 0, &txt_x1, &txt_y1, &txt_w, &txt_h);
		text_rak14000(display_width / 2 - (txt_w / 2), 260, (char *)"Wait for connect", (uint16_t)txt_color, 1);
	}
	else
	{
		display.getTextBounds((char *)"Restart", 0, 0, &txt_x1, &txt_y1, &txt_w, &txt_h);
		text_rak14000(display_width / 2 - (txt_w / 2), 260, (char *)"Restart", (uint16_t)txt_color, 1);
	}
	display.display(false);
}

void rak14000_switch_bg(void)
{
	uint16_t old_txt = txt_color;
	txt_color = bg_color;
	bg_color = old_txt;
	xSemaphoreGive(g_epd_sem);
	delay(4000);
}

/**
 * @brief Task to update the display
 *
 * @param pvParameters unused
 */
void epd_task(void *pvParameters)
{
	MYLOG("EPD", "EPD Task started");

	display.begin();

	display.setRotation(EPD_ROTATION); // 1 for Gavin 3 for mine
	MYLOG("EPD", "Rotation %d", display.getRotation());

	rak14000_start_screen();

	// For partial update only
	// uint16_t counter = 0;
	while (1)
	{
		// For partial update only
		// display.clearBuffer();
		// display.setTextSize(4);
		// display.setTextColor(EPD_BLACK);
		// display.setCursor(32, 32);
		// display.print((counter / 1000) % 10);
		// display.print((counter / 100) % 10);
		// display.print((counter / 10) % 10);
		// display.print(counter % 10);

		// if ((counter % 10) == 0)
		// {
		// 	MYLOG("EPD", "Full update");
		// 	display.display(false);
		// }
		// else
		// {
		// 	MYLOG("EPD", "Partial update");
		// 	// redraw only 4th digit
		// 	display.displayPartial(32 + (24 * 3), 32, 32 + (24 * 4), 32 + (4 * 8));
		// }

		// MYLOG("EPD", "Counter = %d", counter);
		// counter++;
		// delay(5000);

		if (xSemaphoreTake(g_epd_sem, portMAX_DELAY) == pdTRUE)
		{
			// if (!g_is_unoccupied)
			// {
				// if (g_epd_off)
				// {
				// 	MYLOG("EPD", "EPD was off");
				// 	startup_rak14000();
				// }

				display.powerUp();
				refresh_rak14000();
				display.powerDown();
				// Start timer to shut down EPD after 5 seconds (give time to refresh full screen)
				// display_off.start();
			// }
			// else
			// {
			// 	MYLOG("EPD", "Room is unoccupied");
			// }
		}
	}
}

/**
 * @brief Wake up RAK14000 from sleep
 *
 */
void startup_rak14000(void)
{
	digitalWrite(POWER_ENABLE, HIGH);
	g_epd_off = false;
	delay(250);
	display.begin();
	display.setRotation(EPD_ROTATION); // 1 for Gavin 3 for mine
}

/**
 * @brief Put the RAK14000 into sleep mode
 *
 */
void shut_down_rak14000(TimerHandle_t unused)
{
	// Disable power
	digitalWrite(POWER_ENABLE, LOW);
	g_epd_off = true;
}
