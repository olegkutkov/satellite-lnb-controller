/*
   voltage_reader.h
    - Simple output voltages reader with ADC1 and DMA

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

#ifndef VOLTAGE_READER_H
#define VOLTAGE_READER_H

void init_voltage_reader(void);

uint16_t get_ch1_voltage(void);
uint16_t get_ch2_voltage(void);

#endif
