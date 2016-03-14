/*
**  Copyright (C) 2016 Anthony Buckley
** 
**  This file is part of AstroCTC.
** 
**  AstroCTC is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**  
**  AstroCTC is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**  
**  You should have received a copy of the GNU General Public License
**  along with AstroCTC.  If not, see <http://www.gnu.org/licenses/>.
*/



/*
** Description: Snapshot user interface.
**
** Author:	Anthony Buckley
**
** History
**	23-Jun-2015	Initial code
**
*/


/* Includes */

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <main.h>
#include <cam.h>
#include <defs.h>
#include <preferences.h>


/* Defines */


/* Types */
typedef struct _snap_ui
{
    GtkWidget *window;
    GtkWidget *main_window;
    GtkWidget *snap_cntr;
    GtkWidget *frames;
    GtkWidget *delay;
    GtkWidget *delay_opt;
    GtkWidget *grp_delay;
    int close_handler;
} SnapUi;


/* Prototypes */

int snap_ui_main(GtkWidget *);
SnapUi * new_snap_ui();
void snapshot_ui(SnapUi *);
void snap_details(SnapUi *);
void delay_option(int, SnapUi *, PangoFontDescription **);
void snap_spin(int, int, int, GtkWidget **, PangoFontDescription **, GtkWidget *, int);
GtkWidget * snap_label(char *, PangoFontDescription **, GtkWidget *, int);
void OnDelayOpt(GtkToggleButton *, gpointer);
void OnSnapOK(GtkWidget *, gpointer);
void OnSnapCancel(GtkWidget *, gpointer);


extern void register_window(GtkWidget *);
extern void deregister_window(GtkWidget *);
extern int snap_control(CamData *, MainUi *, int, int, int);
extern int val_str2numb(char *, int *, char *, GtkWidget *);
extern int get_user_pref(char *, char **);


/* Globals */

static const char *debug_hdr = "DEBUG-snapshot_ui.c ";



/* Display and control snapshot */

int snap_ui_main(GtkWidget *window)
{
    SnapUi *ui;

    /* Initial */
    ui = new_snap_ui();
    ui->main_window = window;

    /* Create the interface */
    snapshot_ui(ui);
    gtk_widget_show_all(ui->window);
    gtk_widget_grab_focus (ui->frames);

    /* Register the window */
    register_window(ui->window);

    return TRUE;
}


/* Create new screen data variable */

SnapUi * new_snap_ui()
{
    SnapUi *ui = (SnapUi *) malloc(sizeof(SnapUi));
    memset(ui, 0, sizeof(SnapUi));

    return ui;
}


/* Create the user interface and set the CallBacks */

void snapshot_ui(SnapUi *s_ui)
{
    GtkWidget *cancel_btn, *ok_btn;
    GtkWidget *main_vbox, *btn_box;

    /* Set up the UI window */
    s_ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(s_ui->window), SNAP_UI);
    gtk_window_set_position(GTK_WINDOW(s_ui->window), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_default_size(GTK_WINDOW(s_ui->window), 450, 100);
    gtk_container_set_border_width(GTK_CONTAINER(s_ui->window), 5);
    gtk_window_set_transient_for (GTK_WINDOW (s_ui->window), GTK_WINDOW (s_ui->main_window));
    g_object_set_data (G_OBJECT (s_ui->window), "ui", s_ui);

    /* Main view */
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    /* Main grid */
    snap_details(s_ui);

    /* Box container for action buttons */
    btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(GTK_WIDGET (btn_box), GTK_ALIGN_CENTER);

    /* Cancel button */
    cancel_btn = gtk_button_new_with_label("  Cancel  ");
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(OnSnapCancel), s_ui->window);
    gtk_box_pack_end (GTK_BOX (btn_box), cancel_btn, FALSE, FALSE, 0);

    /* OK button */
    ok_btn = gtk_button_new_with_label("  OK  ");
    g_signal_connect(ok_btn, "clicked", G_CALLBACK(OnSnapOK), (gpointer) s_ui);
    gtk_box_pack_end (GTK_BOX (btn_box), ok_btn, FALSE, FALSE, 0);

    /* Combine everything onto the window */
    gtk_box_pack_start (GTK_BOX (main_vbox), s_ui->snap_cntr, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), btn_box, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(s_ui->window), main_vbox);

    /* Set default 'Enter' key action */
    gtk_widget_set_can_default (ok_btn, TRUE);
    gtk_widget_grab_default (ok_btn);

    /* Exit when window closed */
    s_ui->close_handler = g_signal_connect(s_ui->window, "destroy", G_CALLBACK(OnSnapCancel), NULL);

    return;
}


/* Container for main screen items */

void snap_details(SnapUi *s_ui)
{  
    PangoFontDescription *pf, *pf2;
    GtkWidget *label;
    int row;
    char *p;

    /* Font and layout setup */
    pf = pango_font_description_from_string ("Sans 9");
    pf2 = pango_font_description_from_string ("Sans 7");

    s_ui->snap_cntr = gtk_grid_new();
    gtk_widget_set_name(s_ui->snap_cntr, "snap_cntr");
    gtk_container_set_border_width (GTK_CONTAINER (s_ui->snap_cntr), 10);
    row = 0;

    /* Number of frames to grab, delay (if any)(one off or between each frame) */
    snap_label("No. of Frames", &pf, s_ui->snap_cntr, row);
    snap_spin(1, 100000, 1, &(s_ui->frames), &pf, s_ui->snap_cntr, row);
    row++;

    get_user_pref(SNAPSHOT_DELAY, &p);
    snap_label("Delay (secs)", &pf, s_ui->snap_cntr, row);
    snap_spin(0, 1000, atoi(p), &(s_ui->delay), &pf, s_ui->snap_cntr, row);

    /* At 1 second delay is recommended */
    pango_font_description_set_style(pf2, PANGO_STYLE_ITALIC);

    label = gtk_label_new ("(At least 1 second is recommended)");
    gtk_widget_override_font (label, pf2);
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_END);

    gtk_grid_attach(GTK_GRID (s_ui->snap_cntr), label, 3, row, 1, 1);
    row++;

    /* Delay options */
    delay_option(row, s_ui, &pf);
    
    /* Free font */
    pango_font_description_free (pf);
    pango_font_description_free (pf2);

    return;
}


/* Create label */

GtkWidget * snap_label(char *title, PangoFontDescription **pf, GtkWidget *grid, int row)
{  
    GtkWidget *label;

    pango_font_description_set_weight(*pf, PANGO_WEIGHT_BOLD);

    label = gtk_label_new (title);
    gtk_widget_override_font (label, *pf);
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_END);
    gtk_widget_set_margin_end (label, 2);

    gtk_grid_attach(GTK_GRID (grid), label, 0, row, 1, 1);
    
    pango_font_description_set_weight(*pf, PANGO_WEIGHT_NORMAL);

    return label;
}


/* Create entry fields */

void snap_spin(int min, int max, int val, GtkWidget **spin, PangoFontDescription **pf, GtkWidget *grid, int row)
{  
    *spin = gtk_spin_button_new_with_range (min, max, 1);

    gtk_widget_override_font (*spin, *pf);
    gtk_widget_set_halign(GTK_WIDGET (*spin), GTK_ALIGN_START);
    gtk_widget_set_margin_start (*spin, 5);
    gtk_widget_set_margin_end (*spin, 5);
    gtk_widget_set_hexpand (*spin, FALSE);

    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (*spin), TRUE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (*spin), (gdouble) val);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (*spin), GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (*spin), TRUE);
    gtk_entry_set_width_chars (GTK_ENTRY (*spin), 5);

    gtk_grid_attach(GTK_GRID (grid), *spin, 1, row, 1, 1);

    return;
}


/* Delay once only or between each frame group */

void delay_option(int row, SnapUi *s_ui, PangoFontDescription **pf)
{  
    GtkWidget *label;
    GtkWidget *grp_box;

    s_ui->delay_opt = gtk_check_button_new_with_label("Frame Group Delay");
    gtk_widget_override_font (s_ui->delay_opt, *pf);
    gtk_widget_set_margin_start (s_ui->delay_opt, 5);
    gtk_widget_set_margin_end (s_ui->delay_opt, 5);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s_ui->delay_opt), TRUE);
    gtk_widget_set_tooltip_text (s_ui->delay_opt, "Delay may be a one-off at start or for each snapshot (group).");
    g_signal_connect(s_ui->delay_opt, "toggled", G_CALLBACK(OnDelayOpt), (gpointer) s_ui);

    gtk_grid_attach(GTK_GRID (s_ui->snap_cntr), s_ui->delay_opt, 1, row, 1, 1);

    grp_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_halign(GTK_WIDGET (grp_box), GTK_ALIGN_START);
    gtk_widget_set_margin_end (grp_box, 40);

    label = gtk_label_new ("every");
    gtk_widget_override_font (label, *pf);
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_END);
    gtk_box_pack_start (GTK_BOX (grp_box), label, FALSE, FALSE, 0);

    label = gtk_label_new ("frames");
    gtk_widget_override_font (label, *pf);
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_box_pack_end (GTK_BOX (grp_box), label, FALSE, FALSE, 0);

    s_ui->grp_delay = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY (s_ui->grp_delay), 5);
    gtk_entry_set_width_chars(GTK_ENTRY (s_ui->grp_delay), 5);
    gtk_widget_override_font (s_ui->grp_delay, *pf);
    gtk_entry_set_text (GTK_ENTRY (s_ui->grp_delay), "1");
    gtk_box_pack_end (GTK_BOX (grp_box), s_ui->grp_delay, FALSE, FALSE, 0);

    gtk_grid_attach(GTK_GRID (s_ui->snap_cntr), grp_box, 3, row, 1, 1);

    return;
}


/* Callback for group delay */

void OnDelayOpt(GtkToggleButton *opt, gpointer user_data)
{
    SnapUi *ui;

    /* Get data */
    ui = (SnapUi *) user_data;

    /* Set sensitivity of frame group */
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (opt)) == TRUE)
	gtk_widget_set_sensitive (ui->grp_delay, TRUE);
    else
	gtk_widget_set_sensitive (ui->grp_delay, FALSE);

    return;
}


/* Callback OK */

void OnSnapOK(GtkWidget *btn, gpointer user_data)
{
    SnapUi *ui;
    MainUi *m_ui;
    CamData *cam_data;
    int frames, delay, delay_grp;
    const gchar *s;

    /* Get data */
    ui = (SnapUi *) user_data;

    frames = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ui->frames));
    delay = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ui->delay));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ui->delay_opt)) == FALSE)
    {
    	delay_grp = 0;
    }
    else
    {
    	s = gtk_entry_get_text (GTK_ENTRY (ui->grp_delay));

    	if (val_str2numb((char *) s, &delay_grp, "Group Delay", ui->window) == FALSE)
	    return;

	if (delay_grp >= frames)
	    delay_grp = 0;
    }

    /* Close the window, free the screen data and block any secondary close signal */
    g_signal_handler_block (ui->window, ui->close_handler);

    deregister_window(ui->window);
    gtk_window_close(GTK_WINDOW(ui->window));

    /* Initiate snapshot(s) */
    cam_data = g_object_get_data (G_OBJECT(ui->main_window), "cam_data");
    m_ui = g_object_get_data (G_OBJECT(ui->main_window), "ui");

    snap_control(cam_data, m_ui, frames, delay, delay_grp);

    /* Clean up */
    free(ui);

    return;
}


// Callback for window close
// Destroy the window and de-register the window 
// Check for changes

void OnSnapCancel(GtkWidget *window, gpointer user_data)
{ 
    SnapUi *ui;

    /* Get data */
    ui = (SnapUi *) g_object_get_data (G_OBJECT (window), "ui");

    /* Close the window, free the screen data and block any secondary close signal */
    g_signal_handler_block (window, ui->close_handler);

    deregister_window(window);
    gtk_window_close(GTK_WINDOW(window));

    free(ui);

    return;
}
