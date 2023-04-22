/**
 * @file user_at_cmd.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Handle user defined AT commands
 * @version 0.4
 * @date 2022-01-29
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "app.h"
#ifdef NRF52_SERIES
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
using namespace Adafruit_LittleFS_Namespace;

/** Filename to save battery check setting */
static const char batt_name[] = "BATT";

/** File to save battery check status */
File batt_check(InternalFS);

/** Filename to save UI setting */
static const char ui_name[] = "UI";

/** File to save UI status */
File ui_check(InternalFS);
#endif
#ifdef ESP32
#include <Preferences.h>
/** ESP32 preferences */
Preferences esp32_prefs;
#endif

/*****************************************
 * Set UI commands
 *****************************************/

/**
 * @brief Set UI display selection
 *
 * @param str selected UI as String, 0 = scientific, 1 = iconized
 * @return int AT_SUCCESS if ok, AT_ERRNO_PARA_FAIL if invalid value
 */
static int at_set_ui(char *str)
{
	long new_ui = strtol(str, NULL, 0);

	if (new_ui > 1)
	{
		return AT_ERRNO_PARA_NUM;
	}
	g_ui_selected = new_ui;
	save_ui_settings(new_ui);
	return AT_SUCCESS;
}

/**
 * @brief Select UI mode
 *
 * @return int AT_SUCCESS
 */
int at_query_ui(void)
{
	AT_PRINTF("%d", g_ui_selected);
	return AT_SUCCESS;
}

/**
 * @brief List of all available commands with short help and pointer to functions
 *
 */
atcmd_t g_user_at_cmd_list_ui[] = {
	/*|    CMD    |     AT+CMD?      |    AT+CMD=?    |  AT+CMD=value |  AT+CMD  | Permissions |*/
	// Module commands
	{"+UI", "Switch display UI", at_query_ui, at_set_ui, NULL, "RW"},
};

/**
 * @brief Read saved setting UI selection
 *
 */
void read_ui_settings(void)
{
#ifdef NRF52_SERIES
	if (InternalFS.exists(ui_name))
	{
		g_ui_selected = 0;
		MYLOG("USR_AT", "File found, set UI 0 (scientific)");
	}
	else
	{
		g_ui_selected = 1;
		MYLOG("USR_AT", "File not found, set UI 1 (iconized)");
	}
#endif
#ifdef ESP32
	esp32_prefs.begin("ui", false);
	g_ui_selected = esp32_prefs.getInt8("ui", 0);
	esp32_prefs.end();
#endif

	save_ui_settings(g_ui_selected);
}

/**
 * @brief Save the UI settings
 *
 */
void save_ui_settings(uint8_t ui_selected)
{
#ifdef NRF52_SERIES
	if (ui_selected == 0)
	{
		ui_check.open(ui_name, FILE_O_WRITE);
		ui_check.write("1");
		ui_check.close();
		MYLOG("USR_AT", "Created File for UI selection 0");
	}
	else
	{
		InternalFS.remove(ui_name);
		MYLOG("USR_AT", "Remove File for UI selection 1");
	}
#endif
#ifdef ESP32
	esp32_prefs.begin("ui", false);
	esp32_prefs.putBool("ui", ui_selected);
	esp32_prefs.end();
#endif
}

/*****************************************
 * Query modules AT commands
 *****************************************/

/**
 * @brief Query found modules
 *
 * @return int AT_SUCCESS
 */
int at_query_modules(void)
{
	announce_modules();
	return AT_SUCCESS;
}

/**
 * @brief List of all available commands with short help and pointer to functions
 *
 */
atcmd_t g_user_at_cmd_list_modules[] = {
	/*|    CMD    |     AT+CMD?      |    AT+CMD=?    |  AT+CMD=value |  AT+CMD  | Permissions |*/
	// Module commands
	{"+MOD", "List all connected I2C devices", at_query_modules, NULL, at_query_modules, "RW"},
};

/*****************************************
 * RTC AT commands
 *****************************************/

/**
 * @brief Set RTC time
 *
 * @param str time as string, format <year>:<month>:<date>:<hour>:<minute>
 * @return int AT_SUCCESS if successful, otherwise AT_ERRNO_PARA_NUM
 */
static int at_set_rtc(char *str)
{
	uint16_t year;
	uint8_t month;
	uint8_t date;
	uint8_t hour;
	uint8_t minute;

	char *param;

	param = strtok(str, ":");

	// year:month:date:hour:minute

	if (param != NULL)
	{
		/* Check year */
		year = strtoul(param, NULL, 0);

		if (year > 3000)
		{
			return AT_ERRNO_PARA_VAL;
		}

		/* Check month */
		param = strtok(NULL, ":");
		if (param != NULL)
		{
			month = strtoul(param, NULL, 0);

			if ((month < 1) || (month > 12))
			{
				return AT_ERRNO_PARA_VAL;
			}

			// Check day
			param = strtok(NULL, ":");
			if (param != NULL)
			{
				date = strtoul(param, NULL, 0);

				if ((date < 1) || (date > 31))
				{
					return AT_ERRNO_PARA_VAL;
				}

				// Check hour
				param = strtok(NULL, ":");
				if (param != NULL)
				{
					hour = strtoul(param, NULL, 0);

					if (hour > 24)
					{
						return AT_ERRNO_PARA_VAL;
					}

					// Check minute
					param = strtok(NULL, ":");
					if (param != NULL)
					{
						minute = strtoul(param, NULL, 0);

						if (minute > 59)
						{
							return AT_ERRNO_PARA_VAL;
						}

						set_rak12002(year, month, date, hour, minute);

						return 0;
					}
				}
			}
		}
	}
	return AT_ERRNO_PARA_NUM;
}

/**
 * @brief Get RTC time
 *
 * @return int AT_SUCCESS
 */
static int at_query_rtc(void)
{
	// Get date/time from the RTC
	read_rak12002();
	AT_PRINTF("%d.%02d.%02d %d:%02d:%02d", g_date_time.year, g_date_time.month, g_date_time.date, g_date_time.hour, g_date_time.minute, g_date_time.second);
	return AT_SUCCESS;
}

atcmd_t g_user_at_cmd_list_rtc[] = {
	/*|    CMD    |     AT+CMD?      |    AT+CMD=?    |  AT+CMD=value |  AT+CMD  | Permissions |*/
	// RTC commands
	{"+RTC", "Get/Set RTC time and date", at_query_rtc, at_set_rtc, at_query_rtc, "RW"},
};

/*****************************************
 * Battery check AT commands
 *****************************************/

/**
 * @brief Enable/Disable battery check
 *
 * @param str
 * @return int AT_SUCCESS if valid, otherwise AT_ERRNO_PARA_VAL
 */
static int at_set_batt_check(char *str)
{
	long check_bat_request = strtol(str, NULL, 0);
	if (check_bat_request == 1)
	{
		battery_check_enabled = true;
		save_batt_settings(battery_check_enabled);
	}
	else if (check_bat_request == 0)
	{
		battery_check_enabled = false;
		save_batt_settings(battery_check_enabled);
	}
	else
	{
		return AT_ERRNO_PARA_VAL;
	}
	return 0;
}

/**
 * @brief Enable/Disable battery check
 *
 * @return int AT_SUCCESS
 */
static int at_query_batt_check(void)
{
	// Wet calibration value query
	AT_PRINTF("Battery check is %s", battery_check_enabled ? "enabled" : "disabled");
	return AT_SUCCESS;
}

/**
 * @brief Read saved setting for battery check
 *
 */
void read_batt_settings(void)
{
#ifdef NRF52_SERIES
	if (InternalFS.exists(batt_name))
	{
		battery_check_enabled = true;
		MYLOG("USR_AT", "File found, enable battery check");
	}
	else
	{
		battery_check_enabled = false;
		MYLOG("USR_AT", "File not found, disable battery check");
	}
#endif
#ifdef ESP32
	esp32_prefs.begin("bat", false);
	battery_check_enabled = esp32_prefs.getBool("bat", false);
	esp32_prefs.end();
#endif

	save_batt_settings(battery_check_enabled);
}

/**
 * @brief Save the battery check settings
 *
 */
void save_batt_settings(bool check_batt_enables)
{
#ifdef NRF52_SERIES
	if (check_batt_enables)
	{
		batt_check.open(batt_name, FILE_O_WRITE);
		batt_check.write("1");
		batt_check.close();
		MYLOG("USR_AT", "Created File for battery protection enabled");
	}
	else
	{
		InternalFS.remove(batt_name);
		MYLOG("USR_AT", "Remove File for battery protection enabled");
	}
#endif
#ifdef ESP32
	esp32_prefs.begin("bat", false);
	esp32_prefs.putBool("bat", battery_check_enabled);
	esp32_prefs.end();
#endif
}

/** Structure for AT commands */
atcmd_t g_user_at_cmd_list_batt[] = {
	/*|    CMD    |     AT+CMD?      |    AT+CMD=?    |  AT+CMD=value |  AT+CMD  | Permissions |*/
	// Battery check commands
	{"+BATCHK", "Enable/Disable the battery charge check", at_query_batt_check, at_set_batt_check, at_query_batt_check, "RW"},
};

/** Number of user defined AT commands */
uint8_t g_user_at_cmd_num = 0;

/** Pointer to the combined user AT command structure */
atcmd_t *g_user_at_cmd_list;

/**
 * @brief Initialize the user defined AT command list
 *
 */
void init_user_at(void)
{
	uint16_t index_next_cmds = 0;
	uint16_t required_structure_size = sizeof(g_user_at_cmd_list_batt);
	MYLOG("USR_AT", "Structure size %d Battery", required_structure_size);
	required_structure_size += sizeof(g_user_at_cmd_list_modules);
	MYLOG("USR_AT", "Structure size %d Modules", required_structure_size);
	required_structure_size += sizeof(g_user_at_cmd_list_ui);
	MYLOG("USR_AT", "Structure size %d UI", required_structure_size);

	// Get required size of structure
	if (found_sensors[RTC_ID].found_sensor)
	{
		required_structure_size += sizeof(g_user_at_cmd_list_rtc);

		MYLOG("USR_AT", "Structure size %d RTC", required_structure_size);
	}

	// Reserve memory for the structure
	g_user_at_cmd_list = (atcmd_t *)malloc(required_structure_size);

	// Add AT commands to structure
	MYLOG("USR_AT", "Adding battery check AT commands");
	g_user_at_cmd_num += sizeof(g_user_at_cmd_list_batt) / sizeof(atcmd_t);
	memcpy((void *)&g_user_at_cmd_list[index_next_cmds], (void *)g_user_at_cmd_list_batt, sizeof(g_user_at_cmd_list_batt));
	index_next_cmds += sizeof(g_user_at_cmd_list_batt) / sizeof(atcmd_t);
	MYLOG("USR_AT", "Index after adding battery check %d", index_next_cmds);

	MYLOG("USR_AT", "Adding module AT commands");
	g_user_at_cmd_num += sizeof(g_user_at_cmd_list_modules) / sizeof(atcmd_t);
	memcpy((void *)&g_user_at_cmd_list[index_next_cmds], (void *)g_user_at_cmd_list_modules, sizeof(g_user_at_cmd_list_modules));
	index_next_cmds += sizeof(g_user_at_cmd_list_modules) / sizeof(atcmd_t);
	MYLOG("USR_AT", "Index after adding modules check %d", index_next_cmds);

	MYLOG("USR_AT", "Adding UI AT commands");
	g_user_at_cmd_num += sizeof(g_user_at_cmd_list_ui) / sizeof(atcmd_t);
	memcpy((void *)&g_user_at_cmd_list[index_next_cmds], (void *)g_user_at_cmd_list_ui, sizeof(g_user_at_cmd_list_ui));
	index_next_cmds += sizeof(g_user_at_cmd_list_ui) / sizeof(atcmd_t);
	MYLOG("USR_AT", "Index after adding UI commands %d", index_next_cmds);

	if (found_sensors[RTC_ID].found_sensor)
	{
		MYLOG("USR_AT", "Adding RTC user AT commands");
		g_user_at_cmd_num += sizeof(g_user_at_cmd_list_rtc) / sizeof(atcmd_t);
		memcpy((void *)&g_user_at_cmd_list[index_next_cmds], (void *)g_user_at_cmd_list_rtc, sizeof(g_user_at_cmd_list_rtc));
		index_next_cmds += sizeof(g_user_at_cmd_list_rtc) / sizeof(atcmd_t);
		MYLOG("USR_AT", "Index after adding RTC %d", index_next_cmds);
	}
}
