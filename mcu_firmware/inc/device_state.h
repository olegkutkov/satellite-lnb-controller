/*
   device_state.h
    - Device internal state structure

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

#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include <stdint.h>

struct controller_state {
	uint8_t ps_enabled:1;
	uint8_t ch1_tone_enabled:1;
	uint8_t ch1_out_voltage_high:1;
	uint8_t ch2_tone_enabled:1;
	uint8_t ch2_out_voltage_high:1;
};

#endif
