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

#ifndef DEVICE_COMMUNICATOR_H
#define DEVICE_COMMUNICATOR_H

#include <stdint.h>

/* Glue level between hardware and user (gui or cli) */
#define LNB_CHANNEL_1 0x1
#define LNB_CHANNEL_2 0x2

#define ENABLE	0x1
#define DISABLE 0x0

#define POLARITY_VERTICAL_RIGHT  0x2
#define POLARITY_HORIZONTAL_LEFT 0x3

#define BAND_LOW  0x4
#define BAND_HIGH 0x5

/* Hardware state local storage */
struct hardware_state {
	int hw_connected:1;
    int ps_enabled:1;
    float ch1_output_voltage;
    float ch2_output_voltage;
    int ch1_polarity_vr:1;
    int ch1_band_low:1;
    int ch2_polarity_vr:1;
    int ch2_band_low:1;
};

/* Callback functions for the reader thread */
typedef void (*on_device_data) (struct hardware_state *hw_state, void *user_data);
typedef void (*comm_error_handler) (void *user_data);

/* Connect to the hardware */
int hardware_connect(const char *sdev_path);
/* Disconnect from the hardware and clean resources */
int hardware_disconnect();

/* Get the full state of the hardware */
int hardware_read_full_state(struct hardware_state *hw_state);

/* Hardware routines */
int hardware_set_ps_state(uint8_t enabled);
int hardware_set_channel_polarity(uint8_t channel, uint8_t polarity);
int hardware_set_channel_band(uint8_t channel, uint8_t band);

/* Configure data and error cb functions */
void hardware_set_reader_cb(on_device_data func, void *user_data);
void hardware_set_error_cb(comm_error_handler func, void *user_data);

/* Reader thread management */
int hardware_run_reader_thread();
int hardware_stop_reader_thread();

/* Get readable error string */
char *hardware_get_last_error_desc();

#endif
