/* 
   main.c
    - entry point of the application
    - UI routines

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

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "gui_state.h"
#include "port_utils.h"
#include "device_communicator.h"

/* */

static GMainContext *main_context;
static GMutex hw_err_lock;

/* */
struct gui_thread_arg {
	struct hardware_state *hw_state;
	struct lnb_ctrl_gui *gui;
};

/*  */
static void show_error(char *title, char *text)
{
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
									GTK_BUTTONS_OK, "%s", text);

	gtk_window_set_title(GTK_WINDOW(dialog), title);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

/* Switch all UI elements to the "connected" state */
static void set_ui_state_connected(struct lnb_ctrl_gui *gui)
{
	gtk_widget_set_sensitive(gui->button_serial_connect, FALSE);
	gtk_widget_set_sensitive(gui->button_serial_disconnect, TRUE);

	gtk_label_set_markup(GTK_LABEL(gui->connection_status_label), HW_CONNECTED_LABEL);

	gtk_widget_set_sensitive(GTK_WIDGET(gui->power_switch), TRUE);
	gtk_widget_set_sensitive(gui->ch1_polarity_sw_vr, TRUE);
	gtk_widget_set_sensitive(gui->ch1_polarity_sw_hl, TRUE);
	gtk_widget_set_sensitive(gui->ch1_band_sw_low, TRUE);
	gtk_widget_set_sensitive(gui->ch1_band_sw_high, TRUE);
	gtk_widget_set_sensitive(gui->ch2_polarity_sw_vr, TRUE);
	gtk_widget_set_sensitive(gui->ch2_polarity_sw_hl, TRUE);
	gtk_widget_set_sensitive(gui->ch2_band_sw_low, TRUE);
	gtk_widget_set_sensitive(gui->ch2_band_sw_high, TRUE);
}

/* Convert voltage 'float' value to the correct string and update UI label */
static void update_voltage_label(GtkLabel *label, float value)
{
	char str[8];

	snprintf(str, 8, "%2.2f V", value);

	gtk_label_set_text(label, str);
}

/* Set UI elements state according to the current hardware state */
static int ui_set_state_from_hardware(struct lnb_ctrl_gui *gui, struct hardware_state *hw_state)
{
	update_voltage_label(gui->ch1_voltage_label, hw_state->ch1_output_voltage);
	update_voltage_label(gui->ch2_voltage_label, hw_state->ch2_output_voltage);

	gtk_switch_set_active(gui->power_switch, hw_state->ps_enabled);

	if (hw_state->ps_enabled) {
		gtk_label_set_markup(gui->power_status_label, HW_PS_SWITCH_ENABLED_LABEL);
	} else {
		gtk_label_set_markup(gui->power_status_label, HW_PS_SWITCH_DISABLED_LABEL);
	}

	if (hw_state->ch1_polarity_vr) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->ch1_polarity_sw_vr), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->ch1_polarity_sw_hl), TRUE);
	}

	if (hw_state->ch2_polarity_vr) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->ch2_polarity_sw_vr), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->ch2_polarity_sw_hl), TRUE);
	}

	if (hw_state->ch1_band_low) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->ch1_band_sw_low), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->ch1_band_sw_high), TRUE);
	}

	if (hw_state->ch2_band_low) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->ch2_band_sw_low), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->ch2_band_sw_high), TRUE);
	}

	return 0;
}

/* Force initial UI update with a current hardware state */
static int force_update_ui_from_hardware(struct lnb_ctrl_gui *gui)
{
	struct hardware_state hw_state;

	if (hardware_read_full_state(&hw_state) != 0) {
		return -1;
	}

	return ui_set_state_from_hardware(gui, &hw_state);
}

/* Wrapper function for the data update callback */
/* We can't directly work with UI from other (non-GUI) threads */
static gboolean on_hardware_update_cb_from_thread(gpointer arg)
{	
	struct gui_thread_arg *th_arg = (struct gui_thread_arg *) arg;

	if (!th_arg) {
		return G_SOURCE_REMOVE;
	}

	ui_set_state_from_hardware(th_arg->gui, th_arg->hw_state);

	return G_SOURCE_REMOVE;
}

/* Callback function: update UI data from the hardware thread */
static void on_hardware_update_cb(struct hardware_state *hw_state, void *arg)
{
	GSource *source;
	static struct gui_thread_arg th_arg;

	g_mutex_lock(&hw_err_lock);

	source = g_idle_source_new();

	th_arg.gui = (struct lnb_ctrl_gui *) arg;
	th_arg.hw_state = hw_state;

	g_source_set_callback(source, on_hardware_update_cb_from_thread, &th_arg, NULL);

	g_source_attach(source, main_context);
	g_source_unref(source);

	g_mutex_unlock(&hw_err_lock);
}

/* Switch all UI elements to the "disconnected" state */
static void set_ui_state_disconnected(struct lnb_ctrl_gui *gui)
{
	gtk_widget_set_sensitive(gui->button_serial_disconnect, FALSE);
	gtk_widget_set_sensitive(gui->button_serial_connect, TRUE);

	gtk_label_set_markup(gui->connection_status_label, HW_DISCONNECTED_LABEL);

	gtk_widget_set_sensitive(GTK_WIDGET(gui->power_switch), FALSE);
	gtk_widget_set_sensitive(gui->ch1_polarity_sw_vr, FALSE);
	gtk_widget_set_sensitive(gui->ch1_polarity_sw_hl, FALSE);
	gtk_widget_set_sensitive(gui->ch1_band_sw_low, FALSE);
	gtk_widget_set_sensitive(gui->ch1_band_sw_high, FALSE);
	gtk_widget_set_sensitive(gui->ch2_polarity_sw_vr, FALSE);
	gtk_widget_set_sensitive(gui->ch2_polarity_sw_hl, FALSE);
	gtk_widget_set_sensitive(gui->ch2_band_sw_low, FALSE);
	gtk_widget_set_sensitive(gui->ch2_band_sw_high, FALSE);
}

/* Button "connect" click handler */
static void connect_serial_btn_click(GtkButton *button, void *arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;
	const char *sdev_path = (const char *) gtk_combo_box_text_get_active_text(gui->serial_dev_path_selector);

	if (!sdev_path) {
		show_error(UI_STR_ERROR_GENERIC, UI_STR_ERROR_CBOX);
		return;
	}

	if (hardware_connect(sdev_path) < 0) {
		show_error(UI_STR_ERROR_SERIAL_OPEN, hardware_get_last_error_desc());
		return;
	}

	if (force_update_ui_from_hardware(gui) < 0) {
		show_error(UI_STR_HW_COMM_FAIL, hardware_get_last_error_desc());
		hardware_disconnect();
		return;
	}

	if (hardware_run_reader_thread() != 0) {
		show_error(UI_STR_READER_THREAD_FAIL, hardware_get_last_error_desc());
		hardware_disconnect();
		return;
	}

	set_ui_state_connected(gui);
}

/* Properly disconnect from the hardware */
static void disconnect_from_the_hardware()
{
	int ret;

	hardware_stop_reader_thread();

	ret = hardware_disconnect();

	if (ret == -1) {
		show_error(UI_STR_ERROR_SERIAL_CLOSE, hardware_get_last_error_desc());
	}
}

/* Button "disconnect" click handler */
static void disconnect_serial_btn_click(GtkButton *button, void *arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	disconnect_from_the_hardware();

	set_ui_state_disconnected(gui);
}

/* GUI-thread hardware error handler */
static void gui_switch_state_on_fail(struct lnb_ctrl_gui *gui)
{	
	disconnect_from_the_hardware();
	set_ui_state_disconnected(gui);
	show_error(UI_STR_HW_COMM_FAIL, hardware_get_last_error_desc());
}

/* Wrapper function for the Error callback */
/* We can't directly work with UI from other (non-GUI) threads */
static gboolean gui_switch_state_on_fail_from_thread(gpointer arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	gui_switch_state_on_fail(gui);

	return G_SOURCE_REMOVE;
}

/* Callback function. Handle erros from the hardware */
static void on_hardware_error_cb(void *arg)
{
	GSource *source;

	g_mutex_lock(&hw_err_lock);

	source = g_idle_source_new();

	g_source_set_callback(source, gui_switch_state_on_fail_from_thread, arg, NULL);
	g_source_attach(source, main_context);
	g_source_unref(source);

	g_mutex_unlock(&hw_err_lock);
}

/* Power switch action */
static void power_switch_active(GObject *switcher, GParamSpec *pspec, void *arg)
{
	int ret;
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	if (gtk_switch_get_active(GTK_SWITCH(switcher))) {
		ret = hardware_set_ps_state(ENABLE);
		gtk_label_set_markup(gui->power_status_label, HW_PS_SWITCH_ENABLED_LABEL);
	} else {
		ret = hardware_set_ps_state(DISABLE);
		gtk_label_set_markup(gui->power_status_label, HW_PS_SWITCH_DISABLED_LABEL);
	}

	if (ret < 0) {
		gtk_switch_set_active(GTK_SWITCH(switcher), FALSE);
		gtk_label_set_markup(gui->power_status_label, HW_PS_SWITCH_DISABLED_LABEL);

		gui_switch_state_on_fail(gui);
	}
}

/* Channel 1 polarity Vertical/Right selected */
static void ch1_vr_select(GtkButton *button, void *arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	if (hardware_set_channel_polarity(LNB_CHANNEL_1, POLARITY_VERTICAL_RIGHT) < 0) {
		gui_switch_state_on_fail(gui);
	}
}

/* Channel 1 polarity Horizontal/Left selected */
static void ch1_hl_select(GtkButton *button, void *arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	if (hardware_set_channel_polarity(LNB_CHANNEL_1, POLARITY_HORIZONTAL_LEFT) < 0) {
		gui_switch_state_on_fail(gui);
	}
}

/* Channel 1 Band Low selected */
static void ch1_bl_select(GtkButton *button, void *arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	if (hardware_set_channel_band(LNB_CHANNEL_1, BAND_LOW) < 0) {
		gui_switch_state_on_fail(gui);
	}
}

/* Channel 1 Band High selected */
static void ch1_bh_select(GtkButton *button, void *arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	if (hardware_set_channel_band(LNB_CHANNEL_1, BAND_HIGH) < 0) {
		gui_switch_state_on_fail(gui);
	}
}

/* Channel 2 polarity Vertical/Right selected */
static void ch2_vr_select(GtkButton *button, void *arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	if (hardware_set_channel_polarity(LNB_CHANNEL_2, POLARITY_VERTICAL_RIGHT) < 0) {
		gui_switch_state_on_fail(gui);
	}
}

/* Channel 2 polarity Horizontal/Left selected */
static void ch2_hl_select(GtkButton *button, void *arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	if (hardware_set_channel_polarity(LNB_CHANNEL_2, POLARITY_HORIZONTAL_LEFT) < 0) {
		gui_switch_state_on_fail(gui);
	}
}

/* Channel 2 Band Low selected */
static void ch2_bl_select(GtkButton *button, void *arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	if (hardware_set_channel_band(LNB_CHANNEL_2, BAND_LOW) < 0) {
		gui_switch_state_on_fail(gui);
	}
}

/* Channel 2 Band High selected */
static void ch2_bh_select(GtkButton *button, void *arg)
{
	struct lnb_ctrl_gui *gui = (struct lnb_ctrl_gui *) arg;

	if (hardware_set_channel_band(LNB_CHANNEL_2, BAND_HIGH) < 0) {
		gui_switch_state_on_fail(gui);
	}
}

/* Release all resources on window close */
static void on_window_main_destroy()
{
	hardware_stop_reader_thread();
	hardware_disconnect();
	gtk_main_quit();
}

/* Fill the Combobox with an appropriate device list from the port_utils. */
static int init_serial_devices_list(GtkComboBoxText *combo_box)
{
	int i, count = 0;
	char **list = NULL;

	list = list_serial_devices(&count);

	for (i = 0; i < count; ++i) {
		gtk_combo_box_text_append_text(combo_box, list[i]);
	}

	if (count) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
	}

	free_serial_devices_list(list, count);

	return 0;
}

/* Application entry point */
int main(int argc, char *argv[])
{
	struct lnb_ctrl_gui ctrl_gui;
	GError *err = NULL;
	GtkBuilder *builder;

	/* Init GUI */
	gtk_init(&argc, &argv);

	builder = gtk_builder_new();

	/* Try to load GUI layout from the different locations */
	if (!gtk_builder_add_from_file (builder, "/usr/share/lnb_controller/lnb_controller_gui.glade", &err)) {

		err = NULL;

		/* Application is not installed, try to load from the local dev directory */
		if (!gtk_builder_add_from_file (builder, "glade/lnb_controller_gui.glade", &err)) {
			show_error(UI_STR_ERROR_UI_LOAD, err->message);
			g_object_unref(builder);
			return -1;
		}
	}

	/* Get widgets */
	ctrl_gui.main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
	ctrl_gui.serial_dev_path_selector = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "serial_dev_path_selector"));
	ctrl_gui.button_serial_connect = GTK_WIDGET(gtk_builder_get_object(builder, "serial_connect_button"));
	ctrl_gui.button_serial_disconnect = GTK_WIDGET(gtk_builder_get_object(builder, "serial_disconnect_button"));
	ctrl_gui.connection_status_label = GTK_LABEL(gtk_builder_get_object(builder, "connection_status_label"));
	ctrl_gui.power_switch = GTK_SWITCH(gtk_builder_get_object(builder, "ps_switch"));
	ctrl_gui.power_status_label = GTK_LABEL(gtk_builder_get_object(builder, "ps_status_text"));
	ctrl_gui.ch1_voltage_label = GTK_LABEL(gtk_builder_get_object(builder, "chan1_voltage_show"));
	ctrl_gui.ch2_voltage_label = GTK_LABEL(gtk_builder_get_object(builder, "chan2_voltage_show"));
	ctrl_gui.ch1_polarity_sw_vr = GTK_WIDGET(gtk_builder_get_object(builder, "pol_ch1_vr"));	
	ctrl_gui.ch1_polarity_sw_hl = GTK_WIDGET(gtk_builder_get_object(builder, "pol_ch1_hl"));	
	ctrl_gui.ch1_band_sw_low = GTK_WIDGET(gtk_builder_get_object(builder, "tone_ch1_low"));
	ctrl_gui.ch1_band_sw_high = GTK_WIDGET(gtk_builder_get_object(builder, "tone_ch1_high"));
	ctrl_gui.ch2_polarity_sw_vr = GTK_WIDGET(gtk_builder_get_object(builder, "pol_ch2_vr"));
	ctrl_gui.ch2_polarity_sw_hl = GTK_WIDGET(gtk_builder_get_object(builder, "pol_ch2_hl"));
	ctrl_gui.ch2_band_sw_low = GTK_WIDGET(gtk_builder_get_object(builder, "tone_ch2_low"));
	ctrl_gui.ch2_band_sw_high = GTK_WIDGET(gtk_builder_get_object(builder, "tone_ch2_high"));

	/* Aux init */
	init_serial_devices_list(ctrl_gui.serial_dev_path_selector);

	gtk_label_set_markup(ctrl_gui.connection_status_label, HW_DISCONNECTED_LABEL);
	gtk_label_set_markup(ctrl_gui.power_status_label, HW_PS_SWITCH_DISABLED_LABEL);

	/* Connect signals */
	g_signal_connect(ctrl_gui.main_window, "destroy", G_CALLBACK(on_window_main_destroy), NULL);
	g_signal_connect(ctrl_gui.button_serial_connect, "clicked", G_CALLBACK(connect_serial_btn_click), &ctrl_gui);
	g_signal_connect(ctrl_gui.button_serial_disconnect, "clicked", G_CALLBACK(disconnect_serial_btn_click), &ctrl_gui);	
	g_signal_connect(ctrl_gui.power_switch, "notify::active", G_CALLBACK(power_switch_active), &ctrl_gui);

	g_signal_connect(GTK_TOGGLE_BUTTON(ctrl_gui.ch1_polarity_sw_vr), "toggled", G_CALLBACK(ch1_vr_select), &ctrl_gui);
	g_signal_connect(GTK_TOGGLE_BUTTON(ctrl_gui.ch1_polarity_sw_hl), "toggled", G_CALLBACK(ch1_hl_select), &ctrl_gui);
	g_signal_connect(GTK_TOGGLE_BUTTON(ctrl_gui.ch1_band_sw_low), "toggled", G_CALLBACK(ch1_bl_select), &ctrl_gui);
	g_signal_connect(GTK_TOGGLE_BUTTON(ctrl_gui.ch1_band_sw_high), "toggled", G_CALLBACK(ch1_bh_select), &ctrl_gui);

	g_signal_connect(GTK_TOGGLE_BUTTON(ctrl_gui.ch2_polarity_sw_vr), "toggled", G_CALLBACK(ch2_vr_select), &ctrl_gui);
	g_signal_connect(GTK_TOGGLE_BUTTON(ctrl_gui.ch2_polarity_sw_hl), "toggled", G_CALLBACK(ch2_hl_select), &ctrl_gui);
	g_signal_connect(GTK_TOGGLE_BUTTON(ctrl_gui.ch2_band_sw_low), "toggled", G_CALLBACK(ch2_bl_select), &ctrl_gui);
	g_signal_connect(GTK_TOGGLE_BUTTON(ctrl_gui.ch2_band_sw_high), "toggled", G_CALLBACK(ch2_bh_select), &ctrl_gui);

	/* Set HW callbacks and data */
	hardware_set_reader_cb(on_hardware_update_cb, &ctrl_gui);
	hardware_set_error_cb(on_hardware_error_cb, &ctrl_gui);

	g_mutex_init(&hw_err_lock);
	main_context = g_main_context_default();

	/* Run the GUI loop */
	gtk_widget_show(ctrl_gui.main_window);
	gtk_main();

	g_mutex_clear(&hw_err_lock);

	return 0;
}
