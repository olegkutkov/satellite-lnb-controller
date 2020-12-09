/* 
   gui_state.h
    - GUI data structures

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

#ifndef GUI_STATE_H
#define GUI_STATE_H

#include <gtk/gtk.h>
#include <string.h>

/* String for the non-static UI labels */
#define HW_CONNECTED_LABEL "<span foreground='green'>Hardware connected</span>"
#define HW_DISCONNECTED_LABEL "<span foreground='red'>Hardware disconnected</span>"

#define HW_PS_SWITCH_ENABLED_LABEL "<span foreground='green'>Enabled</span>"
#define HW_PS_SWITCH_DISABLED_LABEL "<span foreground='red'>Disabled</span>"

/* Error message strings */
#define UI_STR_ERROR_UI_LOAD "Couldn't load UI XML description"
#define UI_STR_ERROR_GENERIC "Error"
#define UI_STR_ERROR_SERIAL_OPEN "Couldn't open serial device"
#define UI_STR_ERROR_SERIAL_CLOSE "An error occurred during serial device close" 
#define UI_STR_ERROR_CBOX "Couldn't get serial device path from the UI control"
#define UI_STR_HW_COMM_FAIL "Couldn't communicate with hardware"
#define UI_STR_READER_THREAD_FAIL "Unable to start reader thread"

/* lnb_controller UI elements */
struct lnb_ctrl_gui {
	GtkWidget *main_window;
	GtkComboBoxText *serial_dev_path_selector;
	GtkWidget *button_serial_connect;
	GtkWidget *button_serial_disconnect;
	GtkLabel *connection_status_label;
	GtkSwitch *power_switch;
	GtkLabel *power_status_label;
	GtkLabel *ch1_voltage_label;
	GtkLabel *ch2_voltage_label;
	GtkWidget *ch1_polarity_sw_vr;
	GtkWidget *ch1_polarity_sw_hl;
	GtkWidget *ch1_band_sw_low;
	GtkWidget *ch1_band_sw_high;
	GtkWidget *ch2_polarity_sw_vr;
	GtkWidget *ch2_polarity_sw_hl;
	GtkWidget *ch2_band_sw_low;
	GtkWidget *ch2_band_sw_high;
};

#endif

