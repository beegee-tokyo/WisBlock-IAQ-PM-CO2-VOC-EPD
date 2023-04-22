/**
 * @file button.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Button initializer and handler
 * @version 0.1
 * @date 2023-03-23
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app.h"
#include "OneButton.h"

/** Timer for VOC measurement */
SoftwareTimer button_check;

// Flag if button timer is already running
bool timer_running = false;

/**
 * @brief Button instance
 * 		First parameter is interrupt input for button
 * 		Second parameter defines button pushed states:
 * 			true if active low (LOW if pressed)
 * 			false if active high (HIGH if pressed)
 *
 * @return OneButton
 */
OneButton button(BUTTON_INT, true);

// save the millis when a press has started.
/** Time when button was pressed, used to determine length of long press */
unsigned long pressStartTime;

/**
 * @brief Button interrupt callback
 * 		calls button.tick() to process status
 * 		start a timer to frequently repeat button status process until event is handled
 *
 */
void checkTicks(void)
{
	// include all buttons here to be checked
	button.tick(); // just call tick() to check the state.
	if (!timer_running)
	{
		timer_running = true;
		button_check.start();
	}
}

// this function will be called when the button was pressed 1 time only.

/**
 * @brief Callback if single button push was detected
 * 		Used to switch between different display UI's
 * 		At the moment just enables RGB LED
 */
void singleClick(void)
{
	button_check.stop();
	timer_running = false;
	MYLOG("BTN", "singleClick() detected.");

	// Request an UI change
	switch_ui();
}

/**
 * @brief Callback for double button push was detected
 * 		Used to switch display from white to black mode and back
 */
void doubleClick(void)
{
	button_check.stop();
	timer_running = false;
	MYLOG("BTN", "doubleClick() detected.");
	rak14000_switch_bg();
}

// this function will be called when the button was pressed multiple times in a short timeframe.

/**
 * @brief Callback for multi push button events (> 3 push)
 * 		Used for different functionalities
 *      - 9 times ==> reset device
 *
 */
void multiClick()
{
	button_check.stop();
	timer_running = false;
	uint8_t tick_num = button.getNumberClicks();
	switch (tick_num)
	{
	case 3:
		// If BLE is enabled, restart Advertising
		if (g_enable_ble)
		{
			restart_advertising(15);
		}
		break;
	case 9:
		if (g_epd_off)
		{
			MYLOG("BTN", "EPD was off");
			startup_rak14000();
		}
		rak14000_start_screen(false);
		delay(3000);
		api_reset();
		break;
	default:
		MYLOG("BTN", "multiClick(%d) detected.", button.getNumberClicks());
		break;
	}
}

// this function will be called when the button was held down for 1 second or more.

/**
 * @brief Callback when a button is pushed, records the start time of a long press
 *
 */
void pressStart(void)
{
	MYLOG("BTN", "pressStart()");
	pressStartTime = millis() - 1000; // as set in setPressTicks()
}

// this function will be called when the button was released after a long hold.

/**
 * @brief Callback when a long-press event has finished
 * 		Unused at the moment
 *
 */
void pressStop(void)
{
	button_check.stop();
	timer_running = false;
	MYLOG("BTN", "pressStop(%ld) detected.", (millis() - pressStartTime));
}

/**
 * @brief Timer callback after a button push event was detected.
 * 		Needed to continue to check the button status
 *
 * @param unused
 */
void check_button(TimerHandle_t unused)
{
	button.tick();
}

/**
 * @brief Initialize Button functions
 *
 */
void init_button(void)
{
	// Setup interrupt routine
	attachInterrupt(digitalPinToInterrupt(BUTTON_INT), checkTicks, CHANGE);

	// Setup the different callbacks for button events
	button.attachClick(singleClick);
	button.attachDoubleClick(doubleClick);
	button.attachMultiClick(multiClick);

	// Setup long press button events
	button.setPressTicks(3000); // that is the time when LongPressStart is called
	button.attachLongPressStart(pressStart);
	button.attachLongPressStop(pressStop);

	// Create timer for button handling
	button_check.begin(10, check_button, NULL, true);
}
