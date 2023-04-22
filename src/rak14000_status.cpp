/**
 * @file rak14000_status.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Device status UI
 * @version 0.1
 * @date 2023-03-28
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app.h"

#include <Adafruit_GFX.h>
#include <Adafruit_EPD.h>

#include "RAK14000_epd.h"

/** Bandwidths as char arrays */
extern char *bandwidths[];
/** Regions as char arrays */
extern char *region_names[];

void status_ui_rak14000(void)
{
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

	x_text = 10;
	y_text = 15;
	if (g_lorawan_settings.lorawan_enable)
	{
		x_graph = 125;
	}
	else
	{
		x_graph = 150;
	}
	display.setFont(SMALL_FONT);
	display.setTextSize(1);

	snprintf(disp_text, 29, "Device LoRa/LoRaWAN Status:");
	text_rak14000(x_text, y_text, disp_text, txt_color, 1);
	y_text += 15;

	snprintf(disp_text, 29, "Send Interval:");
	text_rak14000(x_text, y_text, disp_text, txt_color, 1);
	snprintf(disp_text, 29, "%ld s", g_lorawan_settings.send_repeat_time / 1000);
	text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
	y_text += 15;

	// snprintf(disp_text, 29, "Device Power:");
	// text_rak14000(x_text, y_text, disp_text, txt_color, 1);
	// snprintf(disp_text, 29, "%s", g_is_using_battery ? "Battery" : "External supply");
	// text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
	// y_text += 15;

	snprintf(disp_text, 29, "Mode:");
	text_rak14000(x_text, y_text, disp_text, txt_color, 1);
	snprintf(disp_text, 29, "%s", g_lorawan_settings.lorawan_enable ? "LPWAN" : "P2P");
	text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
	y_text += 15;

	if (g_lorawan_settings.lorawan_enable)
	{
		snprintf(disp_text, 29, "Auto Join:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%s", g_lorawan_settings.auto_join ? "Enabled" : "Disabled");
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "Network:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%s", g_lpwan_has_joined ? "Joined" : "Not joined");
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "Join Mode:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%s", g_lorawan_settings.otaa_enabled ? "OTAA" : "ABP");
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		if (g_lorawan_settings.otaa_enabled)
		{
			snprintf(disp_text, 29, "Device EUI:");
			text_rak14000(x_text, y_text, disp_text, txt_color, 1);
			snprintf(disp_text, 29, "%02X%02X%02X%02X%02X%02X%02X%02X", g_lorawan_settings.node_device_eui[0], g_lorawan_settings.node_device_eui[1],
					 g_lorawan_settings.node_device_eui[2], g_lorawan_settings.node_device_eui[3],
					 g_lorawan_settings.node_device_eui[4], g_lorawan_settings.node_device_eui[5],
					 g_lorawan_settings.node_device_eui[6], g_lorawan_settings.node_device_eui[7]);
			text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
			y_text += 15;

			snprintf(disp_text, 29, "Application EUI:");
			text_rak14000(x_text, y_text, disp_text, txt_color, 1);
			snprintf(disp_text, 29, "%02X%02X%02X%02X%02X%02X%02X%02X", g_lorawan_settings.node_app_eui[0], g_lorawan_settings.node_app_eui[1],
					 g_lorawan_settings.node_app_eui[2], g_lorawan_settings.node_app_eui[3],
					 g_lorawan_settings.node_app_eui[4], g_lorawan_settings.node_app_eui[5],
					 g_lorawan_settings.node_app_eui[6], g_lorawan_settings.node_app_eui[7]);
			text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
			y_text += 15;

			snprintf(disp_text, 29, "Application Key:");
			text_rak14000(x_text, y_text, disp_text, txt_color, 1);
			snprintf(disp_text, 29, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
					 g_lorawan_settings.node_app_key[0], g_lorawan_settings.node_app_key[1],
					 g_lorawan_settings.node_app_key[2], g_lorawan_settings.node_app_key[3],
					 g_lorawan_settings.node_app_key[4], g_lorawan_settings.node_app_key[5],
					 g_lorawan_settings.node_app_key[6], g_lorawan_settings.node_app_key[7],
					 g_lorawan_settings.node_app_key[8], g_lorawan_settings.node_app_key[9],
					 g_lorawan_settings.node_app_key[10], g_lorawan_settings.node_app_key[11],
					 g_lorawan_settings.node_app_key[12], g_lorawan_settings.node_app_key[13],
					 g_lorawan_settings.node_app_key[14], g_lorawan_settings.node_app_key[15]);
			text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
			y_text += 15;
		}
		else
		{
			snprintf(disp_text, 29, "Device Address:");
			text_rak14000(x_text, y_text, disp_text, txt_color, 1);
			snprintf(disp_text, 29, "%08lX", g_lorawan_settings.node_dev_addr);
			text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
			y_text += 15;

			snprintf(disp_text, 29, "Network Session Key:");
			text_rak14000(x_text, y_text, disp_text, txt_color, 1);
			snprintf(disp_text, 29, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
					 g_lorawan_settings.node_nws_key[0], g_lorawan_settings.node_nws_key[1],
					 g_lorawan_settings.node_nws_key[2], g_lorawan_settings.node_nws_key[3],
					 g_lorawan_settings.node_nws_key[4], g_lorawan_settings.node_nws_key[5],
					 g_lorawan_settings.node_nws_key[6], g_lorawan_settings.node_nws_key[7],
					 g_lorawan_settings.node_nws_key[8], g_lorawan_settings.node_nws_key[9],
					 g_lorawan_settings.node_nws_key[10], g_lorawan_settings.node_nws_key[11],
					 g_lorawan_settings.node_nws_key[12], g_lorawan_settings.node_nws_key[13],
					 g_lorawan_settings.node_nws_key[14], g_lorawan_settings.node_nws_key[15]);
			text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
			y_text += 15;

			snprintf(disp_text, 29, "Application Session Key:");
			text_rak14000(x_text, y_text, disp_text, txt_color, 1);
			snprintf(disp_text, 29, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
					 g_lorawan_settings.node_apps_key[0], g_lorawan_settings.node_apps_key[1],
					 g_lorawan_settings.node_apps_key[2], g_lorawan_settings.node_apps_key[3],
					 g_lorawan_settings.node_apps_key[4], g_lorawan_settings.node_apps_key[5],
					 g_lorawan_settings.node_apps_key[6], g_lorawan_settings.node_apps_key[7],
					 g_lorawan_settings.node_apps_key[8], g_lorawan_settings.node_apps_key[9],
					 g_lorawan_settings.node_apps_key[10], g_lorawan_settings.node_apps_key[11],
					 g_lorawan_settings.node_apps_key[12], g_lorawan_settings.node_apps_key[13],
					 g_lorawan_settings.node_apps_key[14], g_lorawan_settings.node_apps_key[15]);
			text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
			y_text += 15;
		}

		snprintf(disp_text, 29, "Datarate:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%d", g_lorawan_settings.data_rate);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "TX Power:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%d", g_lorawan_settings.tx_power);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "Device Class:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%s", g_lorawan_settings.lora_class == 0 ? "Class A" : "Class C");
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "ADR:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%s", g_lorawan_settings.otaa_enabled ? "Enabled" : "Disabled");
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "Upload type:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%s", g_lorawan_settings.confirmed_msg_enabled ? "Confirmed" : "Unconfirmed");
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "fPort:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%d", g_lorawan_settings.app_port);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "Dutycycle:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%s", g_lorawan_settings.duty_cycle_enabled ? "Enabled" : "Disabled");
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "Network type:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%s", g_lorawan_settings.public_network ? "Public" : "Private");
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "LoRaWAN Region:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%s", region_names[g_lorawan_settings.lora_region]);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);

		switch (g_lorawan_settings.lora_region)
		{
		case 1:
		case 2:
		case 8:
		case 12:
			snprintf(disp_text, 29, "Subband:");
			text_rak14000(x_text + 200, y_text, disp_text, txt_color, 1);
			snprintf(disp_text, 29, "%d", g_lorawan_settings.subband_channels);
			text_rak14000(x_text + 300, y_text, disp_text, txt_color, 1);
			y_text += 15;
			break;
		}
	}
	else
	{
		snprintf(disp_text, 29, "P2P frequency:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%ld", g_lorawan_settings.p2p_frequency);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "P2P TX Power:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%d", g_lorawan_settings.p2p_tx_power);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "P2P Bandwidth:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%s", bandwidths[g_lorawan_settings.p2p_bandwidth]);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "P2P Spreading Factor:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%d", g_lorawan_settings.p2p_sf);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "P2P Coding Rate:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%d", g_lorawan_settings.p2p_cr);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "P2P Preamble length:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%d", g_lorawan_settings.p2p_preamble_len);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;

		snprintf(disp_text, 29, "P2P Symbol Timeout:");
		text_rak14000(x_text, y_text, disp_text, txt_color, 1);
		snprintf(disp_text, 29, "%d", g_lorawan_settings.p2p_symbol_timeout);
		text_rak14000(x_text + x_graph, y_text, disp_text, txt_color, 1);
		y_text += 15;
	}
}