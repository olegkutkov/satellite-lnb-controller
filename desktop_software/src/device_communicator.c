/*
   device_communicator.c
	- Protocol implementation and all communication routines

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

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include "device_communicator.h"
#include "usb_protocol_private.h"
#include "crc8.h"
#include "port_utils.h"

/* This is a default buad rate of the STM ACM implementation */
/* At this moment there is no reason to change it */
#define STM_ACM_DEFAULT_BAUD_RATE 115200

/* Division coefficient of the ADC voltage dividers */
#define HARDWARE_ADC_VOLTAGE_DIVIDER_COEFF 6.58

#define HARDWARE_ADC_VOLTAGE_AVG_COUNT 5

/* Poll timing */
#define READ_POLL_RETRY_COUNT 6
#define READ_POLL_TIEMOUT_MS 300

/* Module data */
static on_device_data on_data_cb_fun = NULL;
static void *on_data_cb_user_data = NULL;

static comm_error_handler on_error_cb_fun = NULL;
static void *on_error_cb_user_data = NULL;

static int serial_fd = 0;

/* Notifier thread */
static volatile int run_reader_thread = 0;
static pthread_mutex_t hw_lock;
static pthread_t reader_thread;

/* Open hardware serial device */
int hardware_connect(const char *sdev_path)
{
	/* Open serial device in non-blocking mode */
	serial_fd = open_serial_dev(sdev_path, STM_ACM_DEFAULT_BAUD_RATE, 1);

	if (serial_fd < 0) {
		return serial_fd;
	}

	pthread_mutex_init(&hw_lock, NULL);

	return 0;
}

/* Close hardware serial device */
int hardware_disconnect()
{
	int ret = 0;

	if (serial_fd) {
		ret = close_serial_dev(serial_fd);
		serial_fd = 0;
	}

	pthread_mutex_destroy(&hw_lock);

	return ret;
}

/* Build generic RX/TX packet and fill with  requested data */
static void buld_generic_packet(uint8_t *pkt, uint8_t op, uint8_t cmd, uint8_t a1, uint8_t a2)
{
	pkt[0] = DS_HEADER_MAGIC1;
	pkt[1] = DS_HEADER_MAGIC2;
	pkt[2] = op;
	pkt[3] = cmd;
	pkt[4] = a1;
	pkt[5] = a2;
	pkt[6] = crc8(pkt, USB_PACKET_LEN - 1);
}

/* Read data from the serial port in non-blocking manner */
int read_answer_nb(int fd, void *buf, size_t count)
{
	struct pollfd fds[1];
	int ret; 

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	/* Poll the serail device */
	/* We really don't want to cover all the cases here */
	for (int i = 0; i < READ_POLL_RETRY_COUNT; i++) {
		ret = poll(fds, 1, READ_POLL_TIEMOUT_MS);

		if (ret < 0) {
			return errno;
		}

		if (ret == 0) {
			return ETIMEDOUT;
		}

		if (fds[0].revents == POLLIN) {
			ret = read(fd, buf, count);

			if (ret != count) {
				if (errno == EAGAIN) {
					continue;
				}

				return EIO;
			}

			return 0;

		} else {
			break;
		}
	}

	return EIO;
}

/* Generic writer function */
/* Build and write packet for the requested cmd and data */
static int write_to_the_device(uint8_t cmd, uint8_t a1, uint8_t a2)
{
	int ret;
	uint8_t packet[USB_PACKET_LEN];

	if (serial_fd <= 0) {
		return -EIO;
	}

	buld_generic_packet(packet, DS_CMD_WRITE, cmd, a1, a2);

	pthread_mutex_lock(&hw_lock);

	if (write(serial_fd, packet, USB_PACKET_LEN) != USB_PACKET_LEN) {
		pthread_mutex_unlock(&hw_lock);
		return -errno;
	}

	ret = read_answer_nb(serial_fd, packet, USB_PACKET_LEN);

	pthread_mutex_unlock(&hw_lock);
 
	if (ret != 0) {
		errno = ret;
		return -errno;
	}

	if (packet[4] != 0xFF && packet[5] != 0xFF) {
		errno = EPROTO;
		return -errno;
	}

	return 0;
}

/* Generic reader function */
static int read_from_the_device(uint8_t cmd, uint8_t *res1, uint8_t *res2)
{
	int ret;
	uint8_t packet[USB_PACKET_LEN];

	if (serial_fd <= 0) {
		errno = EIO;
		return -errno;
	}

	buld_generic_packet(packet, DS_CMD_READ, cmd, 0, 0);

	pthread_mutex_lock(&hw_lock);

	if (write(serial_fd, packet, USB_PACKET_LEN) != USB_PACKET_LEN) {
		pthread_mutex_unlock(&hw_lock);
		return -errno;
	}

	ret = read_answer_nb(serial_fd, packet, USB_PACKET_LEN);

	pthread_mutex_unlock(&hw_lock);
 
	if (ret != 0) {
		errno = ret;
		return -errno;
	}

	/* Let's verify what we got from the device */
	if (packet[2] != DS_RESPONSE) {
		errno = EPROTO;
		return -errno;
	}

	if (packet[6] != crc8(packet, USB_PACKET_LEN - 1)) {
		errno = EPROTO;
		return -errno;
	}

	if (res1) {
		*res1 = packet[4];
	}

	if (res2) {
		*res2 = packet[5];
	}

	return 0;
}

/* Read Power Supply state */
static int read_ps_state(struct hardware_state *hw_state)
{
	int ret;
	uint8_t ps_state;

	ret = read_from_the_device(POWER_SUPPLY_CONTROL, &ps_state, NULL);

	if (ret != 0) {
		return ret;
	}

	hw_state->ps_enabled = (ps_state == POWER_SUPPLY_ENABLED);

	return 0;
}

/* Generic channel voltage reader */
static int read_channel_out_real_voltage(uint8_t channel, float *result)
{
	int ret = 0;
	uint8_t a1, a2;
	uint16_t voltage_raw;

	ret = read_from_the_device(channel, &a1, &a2);

	if (ret != 0) {
		*result = 0.0;
		return ret;
	}

	voltage_raw = a1 << 8;
	voltage_raw |= a2;

	*result = ((float) voltage_raw) / 1000;
	*result *= HARDWARE_ADC_VOLTAGE_DIVIDER_COEFF;

	return ret;
}

/* Channel voltage reader with averaging */
static int read_channel_out_real_voltage_avg(uint8_t channel, float *result)
{
	int ret = 0;
	uint8_t iter;
	float sampled = 0;
	float tmp = 0;

	for (iter = 0; iter < HARDWARE_ADC_VOLTAGE_AVG_COUNT; ++iter) {
		ret = read_channel_out_real_voltage(channel, &tmp);

		if (ret != 0) {
			break;
		}

		sampled += tmp;
	}

	*result = sampled / HARDWARE_ADC_VOLTAGE_AVG_COUNT;

	return ret;
}

/* Read output voltage of the both channels */
static int read_output_real_voltages(struct hardware_state *hw_state)
{
	int ret = read_channel_out_real_voltage_avg(DS_CMD_READ_REAL_VOLTAGE_CH1, &hw_state->ch1_output_voltage);

	if (ret != 0) {
		return ret;
	}

	return read_channel_out_real_voltage_avg(DS_CMD_READ_REAL_VOLTAGE_CH2, &hw_state->ch2_output_voltage);
}

/* Read requested channel selected polarity (voltage mode) */
static int read_channel_polarity(uint8_t channel, uint8_t *vert_right)
{
	int ret;
	uint8_t out_voltage_mode;

	ret = read_from_the_device(channel, &out_voltage_mode, NULL);

	if (ret != 0) {
		return ret;
	}

	*vert_right = (out_voltage_mode == DS_OUT_VOLTAGE_MODE_13V);

	return 0;
}

/* Read channels polarity (voltage mode) */
static int read_channels_polarity(struct hardware_state *hw_state)
{
	uint8_t vert_right;
	int ret = read_channel_polarity(DS_CMD_TYPE_OUT_VOLTAGE_CH1, &vert_right);

	if (ret != 0) {
		return ret;
	}

	hw_state->ch1_polarity_vr = (int) vert_right;

	ret = read_channel_polarity(DS_CMD_TYPE_OUT_VOLTAGE_CH2, &vert_right);

	hw_state->ch2_polarity_vr = (int) vert_right;

	return ret;
}

/* Read requested channel selected band */
static int read_channel_band_mode(uint8_t channel, uint8_t *lo_band)
{
	int ret;
	uint8_t band_mode;

	ret = read_from_the_device(channel, &band_mode, NULL);

	if (ret != 0) {
		return ret;
	}

	*lo_band = (band_mode == DS_OUT_TONE_SIGNAL_DISABLED);

	return 0;
}

/* Read channel's selected bands */
static int read_channels_band(struct hardware_state *hw_state)
{
	uint8_t lo_band;
	int ret = read_channel_band_mode(DS_CMD_TYPE_OUT_TONE_SIGNAL_CH1, &lo_band);

	if (ret != 0) {
		return ret;
	}

	hw_state->ch1_band_low = (int) lo_band;

	ret = read_channel_band_mode(DS_CMD_TYPE_OUT_TONE_SIGNAL_CH2, &lo_band);

	hw_state->ch2_band_low = (int) lo_band;

	return ret;
}

/* Read the full state of the hardware and fill-up structure */
int hardware_read_full_state(struct hardware_state *hw_state)
{
	int ret = 0;

	ret = read_ps_state(hw_state);

	if (ret != 0) {
		return ret;
	}

	ret = read_output_real_voltages(hw_state);

	if (ret != 0) {
		return ret;
	}

	ret = read_channels_polarity(hw_state);

	if (ret != 0) {
		return ret;
	}

	return read_channels_band(hw_state);
}

/* Send commands to the hardware */
int hardware_set_ps_state(uint8_t enabled)
{
	uint8_t en = (enabled == ENABLE ? POWER_SUPPLY_ENABLED : POWER_SUPPLY_DISABLED);

	return write_to_the_device(POWER_SUPPLY_CONTROL, en, 0);
}

/* Set selected channel polarity (voltage mode) */
int hardware_set_channel_polarity(uint8_t channel, uint8_t polarity)
{
	uint8_t sel_chan = (channel == LNB_CHANNEL_1 ? DS_CMD_TYPE_OUT_VOLTAGE_CH1 : DS_CMD_TYPE_OUT_VOLTAGE_CH2);
	uint8_t voltage = (polarity == POLARITY_VERTICAL_RIGHT
						? DS_OUT_VOLTAGE_MODE_13V
						: DS_OUT_VOLTAGE_MODE_18V);

	return write_to_the_device(sel_chan, voltage, 0);
}

/* Set selected channel band mode */
int hardware_set_channel_band(uint8_t channel, uint8_t band)
{
	uint8_t sel_chan = (channel == LNB_CHANNEL_1 ? DS_CMD_TYPE_OUT_TONE_SIGNAL_CH1 : DS_CMD_TYPE_OUT_TONE_SIGNAL_CH2);
	uint8_t sel_band = (band == BAND_LOW ? DS_OUT_TONE_SIGNAL_DISABLED : DS_OUT_TONE_SIGNAL_ENABLED);

	return write_to_the_device(sel_chan, sel_band, 0);
}

/* Callback routines */
void hardware_set_reader_cb(on_device_data func, void *user_data)
{
	on_data_cb_fun = func;
	on_data_cb_user_data = user_data;
}

void hardware_set_error_cb(comm_error_handler func, void *user_data)
{
	on_error_cb_fun = func;
	on_error_cb_user_data = user_data;
}

/* Hardware reader and cb caller thread */
void *reader_thread_fn(void *arg)
{
	int bad_cnt = 0;
	struct hardware_state hw_state;

	while (run_reader_thread) {
		/* If we have some cb function */
		if (on_data_cb_fun) {
			if (hardware_read_full_state(&hw_state) != 0) {
				/* Give it a chance */
				if (bad_cnt++ > 3 && on_error_cb_fun) {
					on_error_cb_fun(on_error_cb_user_data);
					return NULL;
				}
			} else {
				bad_cnt = 0;
				/* Send the current hw state to the cb */
				on_data_cb_fun(&hw_state, on_data_cb_user_data);
			}
		}

		/* Looks good */
		usleep(700000);
	}

	return NULL;
}

/* Just run a thread */
int hardware_run_reader_thread()
{
	run_reader_thread = 1;

	return pthread_create(&reader_thread, NULL, reader_thread_fn, NULL);
}

/* Stop the reader thread and wait for exit */
int hardware_stop_reader_thread()
{
	run_reader_thread = 0;

	pthread_join(reader_thread, NULL);

	return 0;
}

/* Get readable error string from the current errno value */
char *hardware_get_last_error_desc()
{
	return strerror(errno);
}

