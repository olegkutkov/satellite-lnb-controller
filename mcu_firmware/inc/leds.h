/*
   leds.h
     - LEDs controller
 
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

#ifndef LEDS_H
#define LEDS_H

void init_leds(void);

void led13v_ch1_on(void);
void led13v_ch1_off(void);
void led13v_ch2_on(void);
void led13v_ch2_off(void);

void led18v_ch1_on(void);
void led18v_ch1_off(void);
void led18v_ch2_on(void);
void led18v_ch2_off(void);

void led22khz_ch1_tone_on(void);
void led22khz_ch1_tone_off(void);
void led22khz_ch2_tone_on(void);
void led22khz_ch2_tone_off(void);

void system_led_on(void);
void system_led_off(void);

void boot_blink(void);

#endif
