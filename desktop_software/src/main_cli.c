/* 
   main_cl.c
    - entry point of the command line application

   Copyright 2020  Oleg Kutkov <contact@olegkutkov.me>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include "device_communicator.h"

/* Just a simple layer between cli arguments and required actions */
typedef enum user_cmd {
	USER_CMD_NO_CMD = 0,
	USER_CMD_POWER_SUPPLY_ENABLE,
	USER_CMD_POWER_SUPPLY_DISABLE,
	USER_CMD_TONE_ON,
	USER_CMD_TONE_OFF,
	USER_CMD_VERTICAL_POL,
	USER_CMD_RIGHT_POL,
	USER_CMD_HORIZONTAL_POL,
	USER_CMD_LEFT_POL,
	USER_CMD_GET_DATA,
} user_cmd_t;

/* List of cli options */
static struct option cmd_long_options[] =
{
	{ "port", required_argument, 0, 'p' },
	{ "baud", required_argument, 0, 'b' },
	{ "channel", required_argument, 0, 'c' },
	{ "power", required_argument, 0, 'w' },
	{ "tone_on", no_argument, 0, 'o' },
	{ "tone_off", no_argument, 0, 'f' },
	{ "vertical_pol", no_argument, 0, 'v' },
	{ "right_pol", no_argument, 0, 'r' },
	{ "horizontal_pol", no_argument, 0, 'z' },
	{ "left_pol", no_argument, 0, 'l' },
	{ "get", no_argument, 0, 'g' },
	{ "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 }
};

static int show_help()
{
	printf("Usage:\n");
	printf("lnb_controller <params>\n");
	printf("\t--port=<device> - Serial device path (for example: /dev/ttyACM0)\n");
	printf("\t--baud=<baud> - Serial device baud rate, optional. Default value is 115200\n");
	printf("\t--channel=<1|2> - Set the channel number (not requred for a 'get' and PS operations)\n");
	printf("\t--power=<0|1> - Disable or enable power supply\n");
	printf("\t--tone_on - Enable 22KHz tone for the selected channel\n");
	printf("\t--tone_off - Disable 22KHz tone for the selected channel\n");
	printf("\t--vertical_pol - Select Vertical polarization for the selected channel\n");
	printf("\t--right_pol - Select Right polarization for the selected channel\n");
	printf("\t--horizontal_pol - Select Horizontal polarization\n");
	printf("\t--left_pol - Select Left polarization\n");
	printf("\t--get - Read the current state of the hardware\n");
	printf("\t--help - Show this help and exit\n");
	printf("\nLive long and prosper\n");

	return 0;
}

static void display_hw_state()
{
	struct hardware_state hw_state;
	char volt_str[8];

	if (hardware_read_full_state(&hw_state) < 0) {
		printf("Couldn't read the full hardware state, error: %s\n", hardware_get_last_error_desc());
		return;
	}

	printf("\n-------------------------------------------\n");
	printf("Current state of the LNB Controller:\n");
	printf(" Power supply - %s\n", hw_state.ps_enabled ? "ENABLED" : "DISABLED");

	snprintf(volt_str, 8, "%2.2f V", hw_state.ch1_output_voltage);
	printf(" Channel 1 output voltage: %s\n", volt_str);

	snprintf(volt_str, 8, "%2.2f V", hw_state.ch2_output_voltage);
	printf(" Channel 2 output voltage: %s\n", volt_str);

	printf("Channel 1 polarity: %s\n", hw_state.ch1_polarity_vr
										? "Vertical/Right" : "Horizontal/Left");

	printf("Channel 2 polarity: %s\n", hw_state.ch2_polarity_vr
										? "Vertical/Right" : "Horizontal/Left");

	printf("Channel 1 band: %s\n", hw_state.ch1_band_low
										? "LOW (No 22KHz tone)" : "HIGH (22KHz tone)");

	printf("Channel 2 band: %s\n", hw_state.ch2_band_low
										? "LOW (No 22KHz tone)" : "HIGH (22KHz tone)");

	printf("-------------------------------------------\n\n");
}

static inline int verify_ch_num(const uint8_t chnum)
{
	return (chnum == 1 || chnum == 2);
}

int do_cmd(char *port, uint32_t baud, const uint8_t channel, user_cmd_t cmd)
{
	if (hardware_connect(port) < 0) {
		printf("Failed to open serial device %s, error: %s\n", port, hardware_get_last_error_desc());
		return EFAULT;
	}

	switch (cmd) {
		case USER_CMD_POWER_SUPPLY_ENABLE:
			printf("Switching the power supply ON\n");
			if (hardware_set_ps_state(ENABLE) < 0) {
				printf("Failed, error: %s\n", hardware_get_last_error_desc());
			}
			break;

		case USER_CMD_POWER_SUPPLY_DISABLE:
			if (printf("Switching the power supply OFF\n") < 0) {
				printf("Failed, error: %s\n", hardware_get_last_error_desc());
			}
			hardware_set_ps_state(DISABLE);
			break;

		case USER_CMD_TONE_ON:
			if (verify_ch_num(channel)) {
				printf("Setting channel %d HIGH band (22KHz tone)\n", channel);
				if (hardware_set_channel_band(channel, BAND_HIGH) < 0) {
					printf("Failed, error: %s\n", hardware_get_last_error_desc());
				}
			} else {
				printf("Unknown channel %d\n", channel);
			}
			break;

		case USER_CMD_TONE_OFF:
			if (verify_ch_num(channel)) {
				printf("Setting channel %d LOW band (No 22KHz tone)\n", channel);
				if (hardware_set_channel_band(channel, BAND_LOW)) {
					printf("Failed, error: %s\n", hardware_get_last_error_desc());
				}
			} else {
				printf("Unknown channel %d\n", channel);
			}
			break;

		case USER_CMD_VERTICAL_POL:
		case USER_CMD_RIGHT_POL:
			if (verify_ch_num(channel)) {
				printf("Setting channel %d Vertical/Right polarization\n", channel);
				if (hardware_set_channel_polarity(channel, POLARITY_VERTICAL_RIGHT) < 0) {
					printf("Failed, error: %s\n", hardware_get_last_error_desc());
				}
			} else {
				printf("Unknown channel %d\n", channel);
			}
			break;

		case USER_CMD_HORIZONTAL_POL:
		case USER_CMD_LEFT_POL:
			if (verify_ch_num(channel)) {
				printf("Setting channel %d Horizontal/Left polarization\n", channel);
				if (hardware_set_channel_polarity(channel, POLARITY_HORIZONTAL_LEFT) < 0) {
					printf("Failed, error: %s\n", hardware_get_last_error_desc());
				}
			} else {
				printf("Unknown channel %d\n", channel);
			}
			break;

		case USER_CMD_GET_DATA:
			display_hw_state();
			break;

		default:
			break;
	}

	return hardware_disconnect();
}

int main(int argc, char *argv[])
{
	int c;
	int option_index;

	char *port = NULL;
	uint32_t baud = 115200;
	uint8_t channel = 0;

	user_cmd_t ucmd = USER_CMD_NO_CMD;

	while (1) {
		option_index = 0;

		c = getopt_long(argc, argv, "p:b:c:w:ofvzgh", cmd_long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
			case 'p':
				port = optarg;
				break;

			case 'b':
				baud = atoi(optarg);
				break;

			case 'c':
				channel = (uint8_t) atoi(optarg);
				break;

			case 'w':
				if (atoi(optarg) == 1) {
					ucmd = USER_CMD_POWER_SUPPLY_ENABLE;
				} else {
					ucmd = USER_CMD_POWER_SUPPLY_DISABLE;
				}

				break;

			case 'o':
				ucmd = USER_CMD_TONE_ON;
				break;

			case 'f':
				ucmd = USER_CMD_TONE_OFF;
				break;

			case 'v':
				ucmd = USER_CMD_VERTICAL_POL;
				break;

			case 'r':
				ucmd = USER_CMD_RIGHT_POL;
				break;

			case 'z':
				ucmd = USER_CMD_HORIZONTAL_POL;
				break;

			case 'l':
				ucmd = USER_CMD_LEFT_POL;
				break;

			case 'g':
				ucmd = USER_CMD_GET_DATA;
				break;

			case 'h':
				return show_help();

			default:
				break;
		}
	}

	if (!port) {
		fprintf(stderr, "Please set serial port device name\n");
		return -1;
	}

	return do_cmd(port, baud, channel, ucmd);
}

