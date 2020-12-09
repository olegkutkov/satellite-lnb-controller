#
# Makefile
#
#   Copyright 2020  Oleg Kutkov <contact@olegkutkov.me>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
#

PROGRAM = lnb_controller
PROGRAM_CLI = lnb_controller-cli

prefix ?= /usr
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)/bin
datarootdir ?= $(prefix)/share
datadir ?= $(datarootdir)

INSTALL = install
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_PROGRAM = $(INSTALL)

DEBUG := #-g -ggdb
OPTIMIZE := -O2

SRC_PATH := src

LIBS_COMMON := 

LIBS_GUI := gtk+-3.0 \
		$(LIBS_COMMON)

LIBS_CLI := $(LIBS_COMMON)

CFLAGS += -Wall -pipe -I./include $(DEBUG) $(OPTIMIZE)
CFLAGS_GUI := $(shell pkg-config --cflags $(LIBS_GUI)) $(CFLAGS)
CFLAGS_CLI := $(shell pkg-config --cflags $(LIBS_CLI)) $(CFLAGS)

LDFLAGS_COMMON := -lpthread
LDFLAGS_GUI += $(shell pkg-config --libs $(LIBS_GUI)) $(LDFLAGS_COMMON) 
LDFLAGS_CLI += $(shell pkg-config --libs $(LIBS_CLI)) $(LDFLAGS_COMMON)

SRC_COMMON := ${SRC_PATH}/device_communicator.c \
	${SRC_PATH}/crc8.c \
	${SRC_PATH}/port_utils.c

SRC_UI := ${SRC_PATH}/main.c
SRC_CLI := ${SRC_PATH}/main_cli.c

all: gui cli

gui:
	$(CC) $(CFLAGS_GUI) $(SRC_COMMON) $(SRC_UI) $(LDFLAGS_GUI) -o $(PROGRAM)

cli:
	$(CC) $(CFLAGS_CLI) $(SRC_COMMON) $(SRC_CLI) $(LDFLAGS_CLI) -o $(PROGRAM_CLI)

install: install-gui install-cli

install-gui:
	$(INSTALL_DATA) -D glade/lnb_controller_128x128.png $(DESTDIR)$(datadir)/lnb_controller/aw2fits_128x128.png
	$(INSTALL_DATA) -D glade/lnb_controller_48x48.png $(DESTDIR)$(datadir)/lnb_controller/lnb_controller_48x48.png
	$(INSTALL_DATA) -D glade/lnb_controller_gui.glade $(DESTDIR)$(datadir)/lnb_controller/lnb_controller_gui.glade
	$(INSTALL_DATA) -D glade/lnb_controller_48x48.png $(DESTDIR)$(datadir)/pixmaps/lnb_controller.png
	$(INSTALL_PROGRAM) -D lnb_controller $(DESTDIR)$(bindir)
	xdg-desktop-menu install desktop/lnb_controller.desktop --novendor 

uninstall: uninstall-gui uninstall-cli

uninstall-gui:
	rm -f $(DESTDIR)$(bindir)/lnb_controller
	rm -f $(DESTDIR)$(datadir)/pixmaps/lnb_controller.png
	rm -f $(DESTDIR)$(datadir)/lnb_controller/lnb_controller_48x48.png
	rm -f $(DESTDIR)$(datadir)/lnb_controller/lnb_controller_128x128.png
	rm -f $(DESTDIR)$(datadir)/lnb_controller/lnb_controller_gui.glade
	rm -fr $(DESTDIR)$(datadir)/lnb_controller/
	xdg-desktop-menu uninstall desktop/lnb_controller.desktop

install-cli:
	cp -f lnb_controller-cli $(DESTDIR)$(bindir)

uninstall-cli:
	rm -f $(DESTDIR)$(bindir)/lnb_controller-cli

clean:
	rm -f $(PROGRAM) $(PROGRAM_CLI) $(OBJ_COMMON) $(OBJ_GUI) $(OBJ_CLI)

