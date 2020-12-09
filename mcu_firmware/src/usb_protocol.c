/*
   usb_protocol.c
    - USB protocol handler

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

#include "usbd_cdc_if.h"
#include "usb_protocol.h"
#include "usb_protocol_private.h"
#include "leds.h"
#include "crc8.h"
#include "diseqc.h"
#include "voltage_reader.h"

/* Write CMD handler */
static void handle_write_cmd(uint8_t *cmd, uint8_t *arg1, uint8_t *arg2)
{
	/* Just a simple switch */
	switch (*cmd) {
		case POWER_SUPPLY_CONTROL:
			diseqc_set_ps_mode(*arg1 == POWER_SUPPLY_ENABLED);
			break;

		case DS_CMD_TYPE_OUT_VOLTAGE_CH1:
			diseqc_set_ch1_out_voltage(*arg1);
			break;

		case DS_CMD_TYPE_OUT_TONE_SIGNAL_CH1:
			diseq_set_ch1_tone_signal_mode(*arg1 == DS_OUT_TONE_SIGNAL_ENABLED);
			break;

		case DS_CMD_TYPE_OUT_VOLTAGE_CH2:
			diseqc_set_ch2_out_voltage(*arg1);
			break;

		case DS_CMD_TYPE_OUT_TONE_SIGNAL_CH2:
			diseq_set_ch2_tone_signal_mode(*arg1 == DS_OUT_TONE_SIGNAL_ENABLED);
			break;

		default:
			break;
	}
}

/* Send response to the host */
/* We use original CMD and prepared data arguments */
static void send_response(uint8_t *cmd_orig, uint8_t *arg1, uint8_t *arg2)
{
	static uint8_t buf[USB_PACKET_LEN];

	buf[0] = DS_HEADER_MAGIC1;
	buf[1] = DS_HEADER_MAGIC2;
	buf[2] = DS_RESPONSE;
	buf[3] = *cmd_orig;
	buf[4] = *arg1;
	buf[5] = *arg2;
	buf[6] = crc8(buf, USB_PACKET_LEN - 1);

	/* Call framework's transmit function */
	CDC_Transmit_FS(buf, USB_PACKET_LEN /*sizeof(buf)*/);
}

/* Read CMD handler */
static void handle_read_cmd(uint8_t *cmd)
{
	/* TMP voltage storage */
	uint16_t voltage; 
	/* We can respond with two BYTE arguments */
	uint8_t res0 = 0, res1 = 0;

	switch (*cmd) {
		/* Return current state of the power supply */
		case POWER_SUPPLY_CONTROL:
			res0 = diseqc_get_ps_mode() ? POWER_SUPPLY_ENABLED : POWER_SUPPLY_DISABLED;
			break;

		/* Return current configured output voltage for channel 1 */
		case DS_CMD_TYPE_OUT_VOLTAGE_CH1:
			res0 = diseqc_get_ch1_out_voltage() ? DS_OUT_VOLTAGE_MODE_18V : DS_OUT_VOLTAGE_MODE_13V;
			break;

		/* Return current state of the tone signal for channel 1 */
		case DS_CMD_TYPE_OUT_TONE_SIGNAL_CH1:
			res0 = diseq_get_ch1_tone_signal_mode() ? DS_OUT_TONE_SIGNAL_ENABLED : DS_OUT_TONE_SIGNAL_DISABLED;
			break;

		/* Return current configured output voltage for channel 1 */
		case DS_CMD_TYPE_OUT_VOLTAGE_CH2:
			res0 = diseqc_get_ch2_out_voltage() ? DS_OUT_VOLTAGE_MODE_18V : DS_OUT_VOLTAGE_MODE_13V;
			break;

		/* Return current state of the tone signal for channel 1 */
		case DS_CMD_TYPE_OUT_TONE_SIGNAL_CH2:
			res0 = diseq_get_ch2_tone_signal_mode() ? DS_OUT_TONE_SIGNAL_ENABLED : DS_OUT_TONE_SIGNAL_DISABLED;
			break;

		/* Read real output voltage for the channel 1 */
		case DS_CMD_READ_REAL_VOLTAGE_CH1:
			/* Get data from the channel 1 ADC */
			voltage = get_ch1_voltage();
			/* Pack 16 bit value */
			res0 = voltage >> 8;
			res1 = voltage;
			break;

		/* Read real output voltage for the channel 2 */
		case DS_CMD_READ_REAL_VOLTAGE_CH2:
			/* Get data from the channel 2 ADC */
			voltage = get_ch2_voltage();
			/* Pack 16 bit value */
			res0 = voltage >> 8;
			res1 = voltage;
			break;

		default:
			return;
	}

	/* Respond with prepared results */
	send_response(cmd, &res0, &res1);
}

/* Simple respond on the Write commands */
static void send_write_response(uint8_t* buf, uint32_t *len)
{
	buf[4] = 0xFF;
	buf[5] = 0xFF;
	buf[6] = crc8(buf, *len - 1);

	CDC_Transmit_FS(buf, *len);
}

/* We can blink the System LED to indicate RX errors */
void err_blink(uint8_t num)
{
	while (num--) {
		system_led_on();
		LL_mDelay(50);
		system_led_off();
		LL_mDelay(50);
	}
}

/* Callback function for the usbd_cdc_if.c:CDC_Receive_FS */
void handle_rx_data(uint8_t* buf, uint32_t *len)
{
	/* Vasic check of the data */
	if (*len != USB_PACKET_LEN) {
		return;
	}

	/* Verify header magic numbers */
	if (buf[0] != DS_HEADER_MAGIC1 || buf[1] != DS_HEADER_MAGIC2) {
		err_blink(3);
		return;
	}

	/* Verify packet CRC8 checksum */
	if (buf[6] != crc8(buf, *len - 1)) {
		err_blink(4);
		return;
	}

	/* Packet is correct! Let's turn on the System LED */
	/* This LED will be turned off in main() */
	system_led_on();

	/* Handle Read and Write commands */
	if (buf[2] == DS_CMD_WRITE) {
		handle_write_cmd(&(buf[3]), &(buf[4]), &(buf[5]));
		send_write_response(buf, len);
	} else if (buf[2] == DS_CMD_READ) {
		handle_read_cmd(&(buf[3]));
	}
}

