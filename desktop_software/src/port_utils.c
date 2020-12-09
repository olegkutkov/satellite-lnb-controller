/*
   port_utils.c
	- serial port utilities: open, configure, close and list

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


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#define SERIAL_DEVS_MAX_NUM 10
#define SERIAL_DEV_MAX_NAME_LEN 13
#define DEV_DIR "/dev/"
#if defined (__linux__)
#define DEV_TTY_USB_PATT "ttyUSB"
#define DEV_TTY_ACM_PATT "ttyACM"
#elif defined (__APPLE__)
#define DEV_APL_CU_PATT "cu.usb"
#endif

#if defined (__linux__)
/* Convert baud num to the Linux baud define */
static speed_t baud_rate_to_speed_t(uint32_t baud)
{
#define B(x) case x: return B##x
	switch (baud) {
		B(50);     B(75);     B(110);    B(134);    B(150);
		B(200);    B(300);    B(600);    B(1200);   B(1800);
		B(2400);   B(4800);   B(9600);   B(19200);  B(38400);
		B(57600);  B(115200); B(230400); B(460800); B(500000); 
		B(576000); B(921600); B(1000000);B(1152000);B(1500000); 
	default:
		return 0;

	}
#undef B
}
#elif defined (__APPLE__)
static speed_t baud_rate_to_speed_t(uint32_t baud)
{
	return baud;
}
#else
	#pragma error("Bad platform!")
#endif

/* Set specified baud rate
   Other params are constant:
    - NO PARITY
    - 1 STOP BIT
    - 8 DATA BITS
  */
void set_baud_rate(int fd, speed_t baud)
{
	struct termios settings;
	tcgetattr(fd, &settings);

	cfsetispeed(&settings, baud); /* input baud rate */
	cfsetospeed(&settings, baud); /* output baud rate */

	settings.c_cflag &= ~PARENB; /* No parity */
	settings.c_cflag &= ~CSTOPB; /* Clear stop field, only one stop bit used in communication (most common) */
	settings.c_cflag |= CS8; /* 8 bits */

	settings.c_lflag &= ~ICANON;

	 /* Turn off s/w flow ctrl */
	settings.c_iflag &= ~(IXON | IXOFF | IXANY);

	/* Disable any special handling of received bytes */
	settings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

	/* apply the settings */
	tcsetattr(fd, TCSANOW, &settings);
	tcflush(fd, TCOFLUSH);
}

/* Open serial device "dev", configure baud rate and apply some default params */
int open_serial_dev(const char* dev, uint32_t baud, int non_block)
{
	int serial_fd = 0;
	int fd_flags = 0;
	speed_t bs = baud_rate_to_speed_t(baud);

	if (!bs) {
		errno = EINVAL;
		return -errno;
	}

	printf("Open %s and set baud rate = %d\n", dev, baud);

	serial_fd = open(dev, O_RDWR | O_NOCTTY);

	if (serial_fd == -1) {
		return -errno;
	}

	set_baud_rate(serial_fd, bs);

	if (non_block) {
		fd_flags = fcntl(serial_fd, F_GETFL, 0);
		fcntl(serial_fd, F_SETFL, fd_flags | O_NONBLOCK);
	}

	printf("Device %s successfully opened and initialized, descriptor = %i\n"
			, dev, serial_fd);

	return serial_fd;
}

/* */
int close_serial_dev(int fd)
{
	printf("Close device, descriptor %i\n", fd);
	return close(fd);
}

/* Build list of the available and valid serial devices */
/* Don't forget to call free_serial_devices_list */
char **list_serial_devices(int *cnt)
{
	int count = 0;
	DIR *dp;
	struct dirent *de;
	char **list;

	list = (char**) malloc(SERIAL_DEVS_MAX_NUM * sizeof(char*));

	if (!list) {
		fprintf(stderr, "Failed to allocate memory for the serial devices list, error: %s\n", strerror(errno));
		*cnt = -errno;
		return NULL;
	}

	dp = opendir(DEV_DIR);

	if (!dp) {
		fprintf(stderr, "Failed to open %s directory, error: %s\n", DEV_DIR, strerror(errno));
		*cnt = -errno;
		return NULL;
	}

	while ((de = readdir(dp))) {
#if defined (__linux__)
		if (strstr(de->d_name, DEV_TTY_USB_PATT) || strstr(de->d_name, DEV_TTY_ACM_PATT)) {
#elif defined (__APPLE__)
		if (strstr(de->d_name, DEV_APL_CU_PATT)) {
#else
		if (0) {
#endif
			list[count] = (char*) malloc(strlen(de->d_name) * sizeof(char*));

			if (!list[count]) {

				fprintf(stderr, "Failed to allocate memory for the serial device, error: %s\n", strerror(errno)); 
				closedir(dp);
				*cnt = -errno;
				return NULL;
			}

			memcpy(list[count], DEV_DIR, strlen(DEV_DIR));
			memcpy(list[count] + strlen(DEV_DIR), de->d_name, strlen(de->d_name));

			if (count++ > SERIAL_DEVS_MAX_NUM) {
				break;
			}
		}
	}

	closedir(dp);

	*cnt = count;

	return list;
}

/* Free the serail devices list */
void free_serial_devices_list(char **list, int count)
{
	int i;

	if (list) {
		for (i = 0; i < count; ++i) {
			if (list[i]) {
				free(list[i]);
			}
		}

		free(list);
		list = NULL;
	}
}

