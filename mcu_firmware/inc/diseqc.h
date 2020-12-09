/*
   diseqc.h
    - LNB Controller: power supply, polarity, bands

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

#ifndef DISEQC_H
#define DISEQC_H

#include <stdint.h>

void init_diseqc(void);

/* PS API */
void diseqc_set_ps_mode(uint8_t enabled);
uint8_t diseqc_get_ps_mode(void);

/* CH1 API */
void diseq_set_ch1_tone_signal_mode(uint8_t enabled);
uint8_t diseq_get_ch1_tone_signal_mode(void);
void diseqc_set_ch1_out_voltage(uint8_t voltage_val);
uint8_t diseqc_get_ch1_out_voltage(void);

/* CH2 API */
void diseq_set_ch2_tone_signal_mode(uint8_t enabled);
uint8_t diseq_get_ch2_tone_signal_mode(void);
void diseqc_set_ch2_out_voltage(uint8_t voltage_val);
uint8_t diseqc_get_ch2_out_voltage(void);

#endif
