/**
 * @file rak14000_scientific_ui.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Display functions for scientific UI
 * @version 0.1
 * @date 2023-03-26
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app.h"

#include <Adafruit_GFX.h>
#include <Adafruit_EPD.h>

#include "RAK14000_epd.h"

void icon_rak14000(void)
{
	uint8_t g_air_status = 0;

	x_text = 250;
	y_text = 35;
	display.setFont(SMALL_FONT);
	display.setTextSize(1);

	if (found_sensors[RTC_ID].found_sensor)
	{
		read_rak12002();

		if ((found_sensors[PM_ID].found_sensor) || (found_sensors[CO2_ID].found_sensor))
		{
			snprintf(disp_text, 59, "RAK10702 Indoor Comfort %s %d %d %02d:%02d Batt: %.2f V",
					 months_txt[g_date_time.month - 1], g_date_time.date, g_date_time.year,
					 g_date_time.hour, g_date_time.minute,
					 read_batt() / 1000.0);
		}
		else
		{
			snprintf(disp_text, 59, "RAK10702 Indoor Comfort %s %d %d %02d:%02d",
					 months_txt[g_date_time.month - 1], g_date_time.date, g_date_time.year,
					 g_date_time.hour, g_date_time.minute);
		}
	}
	else
	{
		if ((found_sensors[PM_ID].found_sensor) || (found_sensors[CO2_ID].found_sensor))
		{
			snprintf(disp_text, 59, "RAK10702 Indoor Comfort Batt: %.2f V", read_batt() / 1000.0);
		}
		else
		{
			snprintf(disp_text, 59, "RAK10702 Indoor Comfort");
		}
	}

	display.getTextBounds(disp_text, 0, 0, &txt_x1, &txt_y1, &txt_w, &txt_h);
	text_rak14000((display_width / 2) - (txt_w / 2), 1, disp_text, (uint16_t)txt_color, 1);

	snprintf(disp_text, 29, "Temperature: %.2f~C", temp_values[temp_idx - 1]);
	text_rak14000(x_text, y_text, disp_text, txt_color, 1);
	y_text += 20;

	snprintf(disp_text, 29, "Humidity: %.2f%%RH", humid_values[humid_idx - 1]);
	text_rak14000(x_text, y_text, disp_text, txt_color, 1);
	y_text += 20;

	if ((found_sensors[ENV_ID].found_sensor) || (found_sensors[PRESS_ID].found_sensor))
	{
		snprintf(disp_text, 29, "Baro: %.2fmBar", baro_values[baro_idx - 1]);
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		y_text += 20;
	}

	if ((found_sensors[LIGHT_ID].found_sensor) || (found_sensors[LIGHT2_ID].found_sensor))
	{
		snprintf(disp_text, 29, "Light: %.2f Lux", last_light_lux);
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		y_text += 33;
	}

	uint8_t level = 0;

	if (found_sensors[VOC_ID].found_sensor)
	{
		// Get VOC status
		if (voc_valid)
		{
			if (voc_values[voc_idx - 1] > 400)
			{
				if (g_air_status < 255)
				{
					g_air_status = 255;
				}
			}
			else if (voc_values[voc_idx - 1] > 250)
			{
				if (g_air_status < 128)
				{
					g_air_status = 128;
				}
			}
		}
		level = (uint8_t)(voc_values[voc_idx - 1] / 100);
		snprintf(disp_text, 29, "VOC %d", voc_values[voc_idx - 1]);
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		draw_bar_rak14000(level, display_width - 72, y_text);
		y_text += 33;
	}

	if (found_sensors[CO2_ID].found_sensor)
	{
		if (co2_values[co2_idx - 1] > 1500)
		{
			if (g_air_status < 255)
			{
				g_air_status = 255;
			}
		}
		else if (co2_values[co2_idx - 1] > 1000)
		{
			if (g_air_status < 128)
			{
				g_air_status = 128;
			}
		}
		level = (uint8_t)(co2_values[co2_idx - 1] / 500);
		snprintf(disp_text, 29, "CO2 %.0f", co2_values[co2_idx - 1]);
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		draw_bar_rak14000(level, display_width - 72, y_text);
		y_text += 33;
	}
	if (found_sensors[PM_ID].found_sensor)
	{
		// PM 1.0 levels
		if (pm10_values[pm_idx - 1] > 75)
		{
			if (g_air_status < 255)
			{
				g_air_status = 255;
			}
		}
		else if (pm10_values[pm_idx - 1] > 35)
		{
			if (g_air_status < 128)
			{
				g_air_status = 128;
			}
		}
		// PM 2.5 levels
		if (pm25_values[pm_idx - 1] > 75)
		{
			if (g_air_status < 255)
			{
				g_air_status = 255;
			}
		}
		else if (pm25_values[pm_idx - 1] > 35)
		{
			if (g_air_status < 128)
			{
				g_air_status = 128;
			}
		}
		// PM 10 levels
		if (pm100_values[pm_idx - 1] > 199)
		{
			if (g_air_status < 255)
			{
				g_air_status = 255;
			}
		}
		else if (pm100_values[pm_idx - 1] > 150)
		{
			if (g_air_status < 128)
			{
				g_air_status = 128;
			}
		}
		level = (uint8_t)(pm10_values[pm_idx - 1] / 15);
		snprintf(disp_text, 29, "PM 1.0: %d", pm10_values[pm_idx - 1]);
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		draw_bar_rak14000(level, display_width - 72, y_text);
		y_text += 33;
		level = (uint8_t)(pm25_values[pm_idx - 1] / 15);
		snprintf(disp_text, 29, "PM 2.5: %d", pm25_values[pm_idx - 1]);
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		draw_bar_rak14000(level, display_width - 72, y_text);
		y_text += 33;
		level = (uint8_t)(pm100_values[pm_idx - 1] / 40);
		snprintf(disp_text, 29, "PM 10: %d", pm100_values[pm_idx - 1]);
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		draw_bar_rak14000(level, display_width - 72, y_text);
		y_text += 33;
	}

	if (g_air_status == 0)
	{
		display.drawBitmap((x_text - good_air_width) / 2, (display_height - 20 - good_air_height) / 2, good_air, good_air_width, good_air_height, txt_color);
	}
	else if (g_air_status < 255)
	{
		display.drawBitmap((x_text - bad_air_width) / 2, (display_height - 20 - bad_air_height) / 2, worried_air, worried_air_width, worried_air_height, txt_color);
	}
	else
	{
		display.drawBitmap((x_text - worried_air_width) / 2, (display_height - 20 - worried_air_height) / 2, bad_air, bad_air_width, bad_air_height, txt_color);
	}
}

void draw_bar_rak14000(uint8_t level, uint16_t x, uint16_t y)
{
	switch (level)
	{
	case 0:
		display.drawRect(x, y, 10, 10, txt_color);
		display.drawRect(x + 15, y-5, 10, 15, txt_color);
		display.drawRect(x + 30, y-10, 10, 20, txt_color);
		display.drawRect(x + 45, y-15, 10, 25, txt_color);
		display.drawRect(x + 60, y-20, 10, 30, txt_color);
		break;
	case 1:
		display.fillRect(x, y, 10, 10, txt_color);
		display.drawRect(x + 15, y-5, 10, 15, txt_color);
		display.drawRect(x + 30, y-10, 10, 20, txt_color);
		display.drawRect(x + 45, y-15, 10, 25, txt_color);
		display.drawRect(x + 60, y-20, 10, 30, txt_color);
		break;
	case 2:
		display.fillRect(x, y, 10, 10, txt_color);
		display.fillRect(x + 15, y-5, 10, 15, txt_color);
		display.drawRect(x + 30, y-10, 10, 20, txt_color);
		display.drawRect(x + 45, y-15, 10, 25, txt_color);
		display.drawRect(x + 60, y-20, 10, 30, txt_color);
		break;
	case 3:
		display.fillRect(x, y, 10, 10, txt_color);
		display.fillRect(x + 15, y-5, 10, 15, txt_color);
		display.fillRect(x + 30, y-10, 10, 20, txt_color);
		display.drawRect(x + 45, y-15, 10, 25, txt_color);
		display.drawRect(x + 60, y-20, 10, 30, txt_color);
		break;
	case 4:
		display.fillRect(x, y, 10, 10, txt_color);
		display.fillRect(x + 15, y-5, 10, 15, txt_color);
		display.fillRect(x + 30, y-10, 10, 20, txt_color);
		display.fillRect(x + 45, y-15, 10, 25, txt_color);
		display.drawRect(x + 60, y-20, 10, 30, txt_color);
		break;
	default:
		display.fillRect(x, y, 10, 10, txt_color);
		display.fillRect(x + 15, y-5, 10, 15, txt_color);
		display.fillRect(x + 30, y-10, 10, 20, txt_color);
		display.fillRect(x + 45, y-15, 10, 25, txt_color);
		display.fillRect(x + 60, y-20, 10, 30, txt_color);
		break;
	}
}