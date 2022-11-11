/**
 * @file app.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Application specific functions. Mandatory to have init_app(),
 *        app_event_handler(), ble_data_handler(), lora_data_handler()
 *        and lora_tx_finished()
 * @version 0.2
 * @date 2022-01-30
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "app.h"

/** Timer for delayed sending to keep duty cycle */
#ifdef NRF52_SERIES
SoftwareTimer delayed_sending;
#endif
#ifdef ESP32
Ticker delayed_sending;
#endif
#ifdef ARDUINO_ARCH_RP2040
mbed::Ticker delayed_sending;
#endif

/** Flag if delayed sending is already activated */
bool delayed_active = false;

/** Flag for battery protection enabled */
bool battery_check_enabled = false;

/** Set the device name, max length is 10 characters */
char g_ble_dev_name[10] = "WisBlock";

/** Send Fail counter **/
uint8_t join_send_fail = 0;

/** Flag for low battery protection */
bool low_batt_protection = false;

/** LoRaWAN packet */
WisCayenne g_solution_data(255);

char disp_txt[64] = {0};

/**
 * @brief Application specific setup functions
 *
 */
void setup_app(void)
{
	// Initialize Serial for debug output
	Serial.begin(115200);

	time_t serial_timeout = millis();
	// On nRF52840 the USB serial is not available immediately
	while (!Serial)
	{
		if ((millis() - serial_timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
		}
		else
		{
			break;
		}
	}

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);

#if HAS_EPD > 0
	MYLOG("APP", "Init RAK14000");
	init_rak14000();
#endif

	delay(500);

	// Scan the I2C interfaces for devices
	find_modules();

	// Initialize the User AT command list
	init_user_at();

#if defined NRF52_SERIES || defined ESP32
#ifdef BLE_OFF
	// Enable BLE
	g_enable_ble = false;
#else
	// Enable BLE
	g_enable_ble = true;
#endif
#endif
}

/**
 * @brief Application specific initializations
 *
 * @return true Initialization success
 * @return false Initialization failure
 */
bool init_app(void)
{
	MYLOG("APP", "init_app");

	api_set_version(SW_VERSION_1, SW_VERSION_2, SW_VERSION_3);

	// Get the battery check setting
	read_batt_settings();

	AT_PRINTF("============================\n");
	AT_PRINTF("Air Quality Sensor\n");
	AT_PRINTF("Built with RAK's WisBlock\n");
	AT_PRINTF("SW Version %d.%d.%d\n", g_sw_ver_1, g_sw_ver_2, g_sw_ver_3);
	AT_PRINTF("LoRa(R) is a registered trademark or service mark of Semtech Corporation or its affiliates.\nLoRaWAN(R) is a licensed mark.\n");
	AT_PRINTF("============================\n");
	api_log_settings();

	// Announce found modules with +EVT: over Serial
	announce_modules();

	AT_PRINTF("============================\n");

	Serial.flush();
	// Reset the packet
	g_solution_data.reset();

	if (found_sensors[OLED_ID].found_sensor)
	{
		if (found_sensors[RTC_ID].found_sensor)
		{
			read_rak12002();
			snprintf(disp_txt, 64, "%d:%02d Init finished", g_date_time.hour, g_date_time.minute);
		}
		else
		{
			snprintf(disp_txt, 64, "Init finished");
		}
		rak1921_add_line(disp_txt);
	}

	return true;
}

/**
 * @brief Application specific event handler
 *        Requires as minimum the handling of STATUS event
 *        Here you handle as well your application specific events
 */
void app_event_handler(void)
{
	// Timer triggered event
	if ((g_task_event_type & STATUS) == STATUS)
	{
		g_task_event_type &= N_STATUS;
		MYLOG("APP", "Timer wakeup");

#if USE_BSEC == 0
		/*********************************************/
		/** Select between Bosch BSEC algorithm for  */
		/** IAQ index or simple T/H/P readings       */
		/*********************************************/
		if (found_sensors[ENV_ID].found_sensor) // Using simple T/H/P readings
		{
			// Startup the BME680
			start_rak1906();
		}
#endif
		if (found_sensors[PRESS_ID].found_sensor)
		{
			// Startup the LPS22HB
			start_rak1902();
		}

#if defined NRF52_SERIES || defined ESP32
		// If BLE is enabled, restart Advertising
		if (g_enable_ble)
		{
			restart_advertising(15);
		}
#endif

		// Reset the packet
		g_solution_data.reset();

		if (!low_batt_protection)
		{
			MYLOG("APP", "Start reading the sensors");
			// Get values from the connected modules
			get_sensor_values();
		}

		// Get battery level
		float batt_level_f = read_batt();
		g_solution_data.addVoltage(LPP_CHANNEL_BATT, batt_level_f / 1000.0);

		if (found_sensors[OLED_ID].found_sensor)
		{
			if (found_sensors[RTC_ID].found_sensor)
			{
				read_rak12002();
				snprintf(disp_txt, 64, "%d:%02d Bat %.3fV", g_date_time.hour, g_date_time.minute, batt_level_f / 1000);
			}
			else
			{
				snprintf(disp_txt, 64, "Battery %.3fV", batt_level_f / 1000);
			}
			rak1921_add_line(disp_txt);
		}

		// Protection against battery drain if battery check is enabled
		if (battery_check_enabled)
		{
			if (batt_level_f < 2900)
			{
				// Battery is very low, change send time to 1 hour to protect battery
				low_batt_protection = true; // Set low_batt_protection active
				api_timer_restart(1 * 60 * 60 * 1000);
				MYLOG("APP", "Battery protection activated");
			}
			else if ((batt_level_f > 4100) && low_batt_protection)
			{
				// Battery is higher than 4V, change send time back to original setting
				low_batt_protection = false;
				api_timer_restart(g_lorawan_settings.send_repeat_time);
				MYLOG("APP", "Battery protection deactivated");
			}
		}

		// Get data from the slower sensors
#if USE_BSEC == 0
		/*********************************************/
		/** Select between Bosch BSEC algorithm for  */
		/** IAQ index or simple T/H/P readings       */
		/*********************************************/
		if (found_sensors[ENV_ID].found_sensor) // Using simple T/H/P readings
		{
			// Read environment data
			read_rak1906();
		}
#endif
		if (found_sensors[PRESS_ID].found_sensor)
		{
			// Read environment data
			read_rak1902();
		}

		MYLOG("APP", "Packetsize %d", g_solution_data.getSize());
		if (g_lorawan_settings.lorawan_enable)
		{
			lmh_error_status result = send_lora_packet(g_solution_data.getBuffer(), g_solution_data.getSize());
			switch (result)
			{
			case LMH_SUCCESS:
				if (found_sensors[OLED_ID].found_sensor)
				{
					if (found_sensors[RTC_ID].found_sensor)
					{
						read_rak12002();
						snprintf(disp_txt, 64, "%d:%02d Pkg %d b", g_date_time.hour, g_date_time.minute, g_solution_data.getSize());
					}
					else
					{
						snprintf(disp_txt, 64, "Packet sent %d b", g_solution_data.getSize());
					}
					rak1921_add_line(disp_txt);
				}
				MYLOG("APP", "Packet enqueued");
				break;
			case LMH_BUSY:
				MYLOG("APP", "LoRa transceiver is busy");
				AT_PRINTF("+EVT:BUSY\n");
				break;
			case LMH_ERROR:
				AT_PRINTF("+EVT:SIZE_ERROR\n");
				MYLOG("APP", "Packet error, too big to send with current DR");
				break;
			}
		}
		else
		{
			// Send packet over LoRa
			if (send_p2p_packet(g_solution_data.getBuffer(), g_solution_data.getSize()))
			{
				if (found_sensors[OLED_ID].found_sensor)
				{
					if (found_sensors[RTC_ID].found_sensor)
					{
						read_rak12002();
						snprintf(disp_txt, 64, "%d:%02d Pkg %d b", g_date_time.hour, g_date_time.minute, g_solution_data.getSize());
					}
					else
					{
						snprintf(disp_txt, 64, "Packet sent %d b", g_solution_data.getSize());
					}
					rak1921_add_line(disp_txt);
				}
				MYLOG("APP", "Packet enqueued");
			}
			else
			{
				AT_PRINTF("+EVT:SIZE_ERROR\n");
				MYLOG("APP", "Packet too big");
			}
		}
		// Reset the packet
		g_solution_data.reset();
	}

	// VOC read request event
	if ((g_task_event_type & VOC_REQ) == VOC_REQ)
	{
		g_task_event_type &= N_VOC_REQ;

		do_read_rak12047();
	}

	/*********************************************/
	/** Select between Bosch BSEC algorithm for  */
	/** IAQ index or simple T/H/P readings       */
	/*********************************************/
	// BSEC read request event
	if ((g_task_event_type & BSEC_REQ) == BSEC_REQ)
	{
		g_task_event_type &= N_BSEC_REQ;

#if USE_BSEC == 1
		do_read_rak1906_bsec();
#endif
	}
}

// ESP32 is handling the received BLE UART data different, this works only for nRF52
#if defined NRF52_SERIES
/**
 * @brief Handle BLE UART data
 *
 */
void ble_data_handler(void)
{
	if (g_enable_ble)
	{
		// BLE UART data handling
		if ((g_task_event_type & BLE_DATA) == BLE_DATA)
		{
			MYLOG("AT", "RECEIVED BLE");
			/** BLE UART data arrived */
			g_task_event_type &= N_BLE_DATA;

			while (g_ble_uart.available() > 0)
			{
				at_serial_input(uint8_t(g_ble_uart.read()));
				delay(5);
			}
			at_serial_input(uint8_t('\n'));
		}
	}
}
#endif

/**
 * @brief Handle received LoRa Data
 *
 */
void lora_data_handler(void)
{

	// LoRa Join finished handling
	if ((g_task_event_type & LORA_JOIN_FIN) == LORA_JOIN_FIN)
	{
		g_task_event_type &= N_LORA_JOIN_FIN;
		if (g_join_result)
		{
			if (found_sensors[OLED_ID].found_sensor)
			{
				if (found_sensors[RTC_ID].found_sensor)
				{
					read_rak12002();
					snprintf(disp_txt, 64, "%d:%02d Join success", g_date_time.hour, g_date_time.minute);
				}
				else
				{
					snprintf(disp_txt, 64, "Join success");
				}
				rak1921_add_line(disp_txt);
			}
			MYLOG("APP", "Successfully joined network");
			AT_PRINTF("+EVT:JOINED\n");

			// Reset join failed counter
			join_send_fail = 0;

			// // Force a sensor reading in 2 seconds
#ifdef NRF52_SERIES
			delayed_sending.setPeriod(2000);
			delayed_sending.start();
#endif
#ifdef ESP32
			delayed_sending.attach_ms(2000, send_delayed);

#endif
		}
		else
		{
			MYLOG("APP", "Join network failed");
			AT_PRINTF("+EVT:JOIN FAILED\n");
			/// \todo here join could be restarted.
			lmh_join();

#if defined NRF52_SERIES || defined ESP32
			// If BLE is enabled, restart Advertising
			if (g_enable_ble)
			{
				restart_advertising(15);
			}
#endif

			join_send_fail++;
			if (join_send_fail == 10)
			{
				// Too many failed join requests, reset node and try to rejoin
				delay(100);
				api_reset();
			}
		}
	}

	// LoRa TX finished handling
	if ((g_task_event_type & LORA_TX_FIN) == LORA_TX_FIN)
	{
		g_task_event_type &= N_LORA_TX_FIN;

#if HAS_EPD > 0
		// Refresh display
		MYLOG("APP", "Refresh RAK14000");
		wake_rak14000();
		// refresh_rak14000();
#endif
		MYLOG("APP", "LoRa TX cycle %s", g_rx_fin_result ? "finished ACK" : "failed NAK");

		if ((g_lorawan_settings.confirmed_msg_enabled) && (g_lorawan_settings.lorawan_enable))
		{
			AT_PRINTF("+EVT:SEND CONFIRMED %s\n", g_rx_fin_result ? "SUCCESS" : "FAIL");
		}
		else
		{
			AT_PRINTF("+EVT:SEND OK\n");
		}

		if (!g_rx_fin_result)
		{
			// Increase fail send counter
			join_send_fail++;

			if (join_send_fail == 10)
			{
				// Too many failed sendings, reset node and try to rejoin
				delay(100);
				api_reset();
			}
		}
	}

	// LoRa data handling
	if ((g_task_event_type & LORA_DATA) == LORA_DATA)
	{
		g_task_event_type &= N_LORA_DATA;
		MYLOG("APP", "Received package over LoRa");
		// Check if uplink was a send frequency change command
		if ((g_last_fport == 3) && (g_rx_data_len == 6))
		{
			if (g_rx_lora_data[0] == 0xAA)
			{
				if (g_rx_lora_data[1] == 0x55)
				{
					uint32_t new_send_frequency = 0;
					new_send_frequency |= (uint32_t)(g_rx_lora_data[2]) << 24;
					new_send_frequency |= (uint32_t)(g_rx_lora_data[3]) << 16;
					new_send_frequency |= (uint32_t)(g_rx_lora_data[4]) << 8;
					new_send_frequency |= (uint32_t)(g_rx_lora_data[5]);

					MYLOG("APP", "Received new send frequency %ld s\n", new_send_frequency);
					// Save the new send frequency
					g_lorawan_settings.send_repeat_time = new_send_frequency * 1000;

					// Set the timer to the new send frequency
					api_timer_restart(g_lorawan_settings.send_repeat_time);
					// Save the new send frequency
					save_settings();
				}
			}
		}

		if (g_lorawan_settings.lorawan_enable)
		{
			AT_PRINTF("+EVT:RX_1, RSSI %d, SNR %d\n", g_last_rssi, g_last_snr);
			AT_PRINTF("+EVT:%d:", g_last_fport);
			for (int idx = 0; idx < g_rx_data_len; idx++)
			{
				AT_PRINTF("%02X", g_rx_lora_data[idx]);
			}
			AT_PRINTF("\n");
		}
		else
		{
			AT_PRINTF("+EVT:RXP2P, RSSI %d, SNR %d\n", g_last_rssi, g_last_snr);
			AT_PRINTF("+EVT:");
			for (int idx = 0; idx < g_rx_data_len; idx++)
			{
				AT_PRINTF("%02X", g_rx_lora_data[idx]);
			}
			AT_PRINTF("\n");
		}
	}
}

/**
 * @brief Timer function used to avoid sending packages too often.
 * 			Delays the next package by 10 seconds
 *
 * @param unused
 * 			Timer handle, not used
 */
#ifdef NRF52_SERIES
void send_delayed(TimerHandle_t unused)
{
	api_wake_loop(STATUS);
	delayed_sending.stop();
}
#elif defined ESP32 || defined ARDUINO_ARCH_RP2040
void send_delayed(void)
{
	api_wake_loop(STATUS);
	delayed_sending.detach();
}
#endif
