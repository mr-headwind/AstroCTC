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
** Description: Video capture user interface.
**
** Author:	Anthony Buckley
**
** History
**	8-Aug-2014	Initial code
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


/* Defines */


/* Types */
typedef struct _capt_ui
{
    GtkWidget *window;
    GtkWidget *main_window;
    GtkWidget *capt_cntr;
    GtkWidget *capt_duration;
    GtkWidget *capt_frames;
    GtkWidget *radio_grp;
    int active_radio;
    int close_handler;
    int duration;
    int no_frames;
} CaptUi;


/* Prototypes */

int capture_main(GtkWidget *);
CaptUi * new_capt_ui();
void capture_ui(CaptUi *);
void capt_grid(CaptUi *);
void capt_entry(char *, GtkWidget **, GtkWidget *, int, CaptUi *);
void capt_radio(char *, char *, GtkWidget *, int, CaptUi *);
int validate_capt(CaptUi *);
int validate_numb(GtkWidget *, char *, int, int *, CaptUi *);
void OnCaptRadio(GtkWidget *, gpointer);
void OnCaptOK(GtkWidget *, gpointer);
void OnCaptCancel(GtkWidget *, gpointer);


extern void app_msg(char*, char *, GtkWidget*);
extern void register_window(GtkWidget *);
extern void deregister_window(GtkWidget *);
extern int gst_capture(CamData *, MainUi *, int, int);
extern int val_str2numb(char *, int *, char *, GtkWidget *);


/* Globals */

static const char *debug_hdr = "DEBUG-capture_ui.c ";

enum
    {
       DURATION,
       NO_OF_FRAMES,
       UNLIMITED
    };


/* Display and control capture */

int capture_main(GtkWidget *window)
{
    CaptUi *ui;

    /* Initial */
    ui = new_capt_ui();
    ui->main_window = window;

    /* Create the interface */
    capture_ui(ui);
    gtk_widget_show_all(ui->window);
    gtk_widget_grab_focus (ui->capt_duration);

    /* Register the window */
    register_window(ui->window);

    return TRUE;
}


/* Create new screen data variable */

CaptUi * new_capt_ui()
{
    CaptUi *ui = (CaptUi *) malloc(sizeof(CaptUi));
    memset(ui, 0, sizeof(CaptUi));

    return ui;
}


/* Create the user interface and set the CallBacks */

void capture_ui(CaptUi *c_ui)
{
    GtkWidget *cancel_btn, *ok_btn;
    GtkWidget *main_vbox, *btn_box;

    /* Set up the UI window */
    c_ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(c_ui->window), CAPTURE_UI);
    gtk_window_set_position(GTK_WINDOW(c_ui->window), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_default_size(GTK_WINDOW(c_ui->window), 250, 150);
    gtk_container_set_border_width(GTK_CONTAINER(c_ui->window), 5);
    gtk_window_set_transient_for (GTK_WINDOW (c_ui->window), GTK_WINDOW (c_ui->main_window));
    g_object_set_data (G_OBJECT (c_ui->window), "ui", c_ui);

    /* Main view */
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    /* Main grid */
    capt_grid(c_ui);

    /* Box container for action buttons */
    btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(GTK_WIDGET (btn_box), GTK_ALIGN_CENTER);

    /* Cancel button */
    cancel_btn = gtk_button_new_with_label("  Cancel  ");
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(OnCaptCancel), c_ui->window);
    gtk_box_pack_end (GTK_BOX (btn_box), cancel_btn, FALSE, FALSE, 0);

    /* OK button */
    ok_btn = gtk_button_new_with_label("  OK  ");
    g_signal_connect(ok_btn, "clicked", G_CALLBACK(OnCaptOK), (gpointer) c_ui);
    gtk_box_pack_end (GTK_BOX (btn_box), ok_btn, FALSE, FALSE, 0);

    /* Combine everything onto the window */
    gtk_box_pack_start (GTK_BOX (main_vbox), c_ui->capt_cntr, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), btn_box, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(c_ui->window), main_vbox);

    /* Set default 'Enter' key action */
    gtk_widget_set_can_default (ok_btn, TRUE);
    gtk_widget_grab_default (ok_btn);

    /* Exit when window closed */
    c_ui->close_handler = g_signal_connect(c_ui->window, "destroy", G_CALLBACK(OnCaptCancel), NULL);

    return;
}


/* Container for main screen items */

void capt_grid(CaptUi *c_ui)
{  
    int row;

    /* Layout setup */
    c_ui->capt_cntr = gtk_grid_new();
    gtk_widget_set_name(c_ui->capt_cntr, "capt_cntr");
    gtk_container_set_border_width (GTK_CONTAINER (c_ui->capt_cntr), 10);
    row = 0;

    /* Allow the type of capture required */
    capt_radio("Duration (secs)", "1_dur", c_ui->capt_cntr, row, c_ui);
    capt_entry("capt_dur", &(c_ui->capt_duration), c_ui->capt_cntr, row, c_ui);
    row++;

    capt_radio("Number of Frames", "2_frames", c_ui->capt_cntr, row, c_ui);
    capt_entry("capt_frames", &(c_ui->capt_frames), c_ui->capt_cntr, row, c_ui);
    row++;

    capt_radio("Unlimited", "3_unltd", c_ui->capt_cntr, row, c_ui);

    return;
}


/* Create entry fields */

void capt_entry(char *nm, GtkWidget **ent, GtkWidget *grid, int row, CaptUi *c_ui)
{  
    *ent = gtk_entry_new();
    gtk_widget_set_name(*ent, nm);
    gtk_entry_set_max_length(GTK_ENTRY (*ent), 7);
    gtk_entry_set_width_chars(GTK_ENTRY (*ent), 7);
    gtk_widget_set_margin_start (*ent, 10);
    gtk_widget_set_margin_end (*ent, 10);
    gtk_widget_set_margin_bottom(*ent, 3);
    gtk_widget_set_valign (*ent, GTK_ALIGN_CENTER);

    if (row == 0)
    	gtk_widget_set_sensitive(*ent, TRUE);
    else
    	gtk_widget_set_sensitive(*ent, FALSE);

    gtk_grid_attach(GTK_GRID (grid), *ent, 1, row, 1, 1);
    g_signal_connect(*ent, "activate", G_CALLBACK(OnCaptOK), (gpointer) c_ui);

    return;
}


/* Create radio buttons */

void capt_radio(char *desc, char *nm, GtkWidget *grid, int row, CaptUi *c_ui)
{  
    GtkWidget *radio;

    if (row == 0)
    {
	radio = gtk_radio_button_new_with_label (NULL, desc);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
	c_ui->radio_grp = radio;
	c_ui->active_radio = DURATION;
    }
    else
    {
	radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (c_ui->radio_grp), desc);
    }

    gtk_widget_set_name(radio, nm);
    gtk_widget_set_margin_bottom(radio, 3);
    gtk_widget_set_valign (radio, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (radio, GTK_ALIGN_START);

    g_signal_connect(radio, "toggled", G_CALLBACK(OnCaptRadio), (gpointer) c_ui);
    gtk_grid_attach(GTK_GRID (grid), radio, 0, row, 1, 1);

    return;
}


/* Validate screen contents, save if valid */

int validate_capt(CaptUi *c_ui)
{
    /* Duration */
    if (validate_numb(c_ui->capt_duration,
    		      "Duration", 
    		      DURATION, 
    		      &(c_ui->duration), 
    		      c_ui) == FALSE)
    	return FALSE;

    /* Number of frames */
    if (validate_numb(c_ui->capt_frames,
		      "Number of frames", 
		      NO_OF_FRAMES, 
		      &(c_ui->no_frames), 
		      c_ui) == FALSE)
    	return FALSE;

    return TRUE;
}


/* Validate number */

int validate_numb(GtkWidget *ent, char *fld_desc, int active_fld, int *i_capt, CaptUi *c_ui)
{
    const gchar *s;
    int i;

    s = gtk_entry_get_text (GTK_ENTRY (ent));

    if (val_str2numb((char *) s, &i, fld_desc, c_ui->window) == FALSE)
	return FALSE;

    if (c_ui->active_radio == active_fld && i == 0)
    {
	app_msg("APP0003", fld_desc, c_ui->window);
	return FALSE;
    }

    *i_capt = i;

    return TRUE;
}


/* Callback - Set capture type */

void OnCaptRadio(GtkWidget *radio, gpointer user_data)
{  
    const gchar *nm;
    CaptUi *c_ui;
    int dur_sens, frm_sens;

    char s[30];
    GtkWidget *grid;

    /* Initial */
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio)) == FALSE)
    	return;

    c_ui = (CaptUi *) user_data;
    nm = gtk_widget_get_name (radio);

    /* Determine which toggle is active */
    switch (*nm)
    {
    	case '1':
	    dur_sens = TRUE;
	    frm_sens = FALSE;
	    c_ui->active_radio = DURATION;
	    break;

    	case '2':
	    dur_sens = FALSE;
	    frm_sens = TRUE;
	    c_ui->active_radio = NO_OF_FRAMES;
	    break;

    	default:
	    dur_sens = FALSE;
	    frm_sens = FALSE;
	    c_ui->active_radio = UNLIMITED;
	    break;
    }

    gtk_widget_set_sensitive(c_ui->capt_duration, dur_sens);
    gtk_widget_set_sensitive(c_ui->capt_frames, frm_sens);

    return;
}


/* Callback OK */

void OnCaptOK(GtkWidget *btn, gpointer user_data)
{
    CaptUi *ui;
    CamData *cam_data;
    MainUi *m_ui;

    /* Get data */
    ui = (CaptUi *) user_data;

    /* Check the values of the entry fields as numeric */
    if (validate_capt(ui) == FALSE)
    	return;

    /* Close the window, free the screen data and block any secondary close signal */
    g_signal_handler_block (ui->window, ui->close_handler);

    deregister_window(ui->window);
    gtk_window_close(GTK_WINDOW(ui->window));

    /* Initiate capture */
    cam_data = g_object_get_data (G_OBJECT(ui->main_window), "cam_data");
    m_ui = g_object_get_data (G_OBJECT(ui->main_window), "ui");

    if (ui->active_radio == DURATION)
    {
    	ui->no_frames = 0;
    }
    else if (ui->active_radio == NO_OF_FRAMES)
    {
    	ui->duration = 0;
    }
    else
    {
    	ui->duration = 0;
    	ui->no_frames = 0;
    }

    gst_capture(cam_data, m_ui, ui->duration, ui->no_frames);

    /* Clean up */
    free(ui);

    return;
}


// Callback for window close
// Destroy the window and de-register the window 
// Check for changes

void OnCaptCancel(GtkWidget *window, gpointer user_data)
{ 
    CaptUi *ui;

    /* Get data */
    ui = (CaptUi *) g_object_get_data (G_OBJECT (window), "ui");

    /* Close the window, free the screen data and block any secondary close signal */
    g_signal_handler_block (window, ui->close_handler);

    deregister_window(window);
    gtk_window_close(GTK_WINDOW(window));

    free(ui);

    return;
}
