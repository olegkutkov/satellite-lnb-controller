/*
   port_utils.h

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

#ifndef PORT_UTILS_H
#define PORT_UTILS_H

#include <stdint.h>

int open_serial_dev(const char* dev, uint32_t baud, int non_block);
int close_serial_dev(int fd);

char **list_serial_devices(int *count);
void free_serial_devices_list(char **list, int count);

#endif
