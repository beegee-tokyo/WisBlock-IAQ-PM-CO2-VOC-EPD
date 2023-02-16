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

/** Filename to save GPS precision setting */
static const char batt_name[] = "BATT";

/** File to save battery check status */
File batt_check(InternalFS);
#endif
#ifdef ESP32
#include <Preferences.h>
/** ESP32 preferences */
Preferences esp32_prefs;
#endif

/*****************************************
 * Query modules AT commands
 *****************************************/

/**
 * @brief Query found modules
 *
 * @return int 0
 */
int at_query_modules(void)
{
	announce_modules();
	return 0;
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
 * @return int 0 if successful, otherwise error value
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
 * @return int 0
 */
static int at_query_rtc(void)
{
	// Get date/time from the RTC
	read_rak12002();
	AT_PRINTF("%d.%02d.%02d %d:%02d:%02d", g_date_time.year, g_date_time.month, g_date_time.date, g_date_time.hour, g_date_time.minute, g_date_time.second);
	return 0;
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
 * @return int
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
 * @return int 0
 */
static int at_query_batt_check(void)
{
	// Wet calibration value query
	AT_PRINTF("Battery check is %s", battery_check_enabled ? "enabled" : "disabled");
	return 0;
}

/**
 * @brief Read saved setting for precision and packet format
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
 * @brief Save the GPS settings
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

#define TEST_ALL_CMDS 0

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

	if (found_sensors[RTC_ID].found_sensor)
	{
		MYLOG("USR_AT", "Adding RTC user AT commands");
		g_user_at_cmd_num += sizeof(g_user_at_cmd_list_rtc) / sizeof(atcmd_t);
		memcpy((void *)&g_user_at_cmd_list[index_next_cmds], (void *)g_user_at_cmd_list_rtc, sizeof(g_user_at_cmd_list_rtc));
		index_next_cmds += sizeof(g_user_at_cmd_list_rtc) / sizeof(atcmd_t);
		MYLOG("USR_AT", "Index after adding RTC %d", index_next_cmds);
	}
}
