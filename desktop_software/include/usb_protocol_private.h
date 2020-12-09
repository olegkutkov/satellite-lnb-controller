/*
   usb_protocol_private.h
    - USB protocol commands and registers

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

#ifndef USB_PROTOCOL_PRIVATE_H
#define USB_PROTOCOL_PRIVATE_H

/*
 Packet format:
	----------------------------
	| 0 | 0xAE | Magic byte 1  |
	----------------------------
	| 1 | 0xAB | MAgic byte 2  |
	----------------------------
	| 2 | 0x01 | CMD WRITE     |
	|   | 0x02 | CMD READ      |
	|   | 0xE1 | DS_RESPONSE   |
	----------------------------
	| 3 |      | DEV CMD       |
    ----------------------------
	| 4 |      | DEV CMD ARG1  |
	----------------------------
	| 5 |      | DEV CMD ARG2  |
	----------------------------
	| 6 |      | CRC8 Checksum |
    ----------------------------
 */

#define USB_PACKET_LEN		0x7

/* Common protocol defines */
#define DS_HEADER_MAGIC1	0xAE
#define DS_HEADER_MAGIC2	0xAB

#define DS_CMD_WRITE		0x01
#define DS_CMD_READ			0x02
#define DS_RESPONSE			0xE1

/* Common power supply control */
#define POWER_SUPPLY_CONTROL	0xDD
#define POWER_SUPPLY_DISABLED	0xD0
#define POWER_SUPPLY_ENABLED	0xD1

/* Channel 1 voltage and tone signal */
#define DS_CMD_TYPE_OUT_VOLTAGE_CH1		0xB0
#define DS_CMD_READ_REAL_VOLTAGE_CH1	0xC0
#define DS_CMD_TYPE_OUT_TONE_SIGNAL_CH1	0x07

/* Channel 2 voltage and tone signal */
#define DS_CMD_TYPE_OUT_VOLTAGE_CH2		0xB1
#define DS_CMD_READ_REAL_VOLTAGE_CH2	0xC1
#define DS_CMD_TYPE_OUT_TONE_SIGNAL_CH2	0x10

/* Common voltage and tone defines */
#define DS_OUT_VOLTAGE_MODE_13V		0x0D
#define DS_OUT_VOLTAGE_MODE_18V		0x12
#define DS_OUT_TONE_SIGNAL_ENABLED	0xEE
#define DS_OUT_TONE_SIGNAL_DISABLED	0xED

/* TODO: DISEqC 1.x commands */

/* */

#endif
