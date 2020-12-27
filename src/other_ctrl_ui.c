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
** Description: 'Other' camera controls user interface.
**
** Author:	Anthony Buckley
**
** History
**	12-Aug-2015	Initial code
**	20-Nov-2020	Changes to move to css
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
typedef struct _other_ctrl_ui
{
    GtkWidget *window;
    GtkWidget *main_window;
    GtkWidget *oth_cntr;
    GtkWidget *cntl_ev_box;
    GtkWidget *cntl_frame;
    int close_handler;
    CamData *cam_data;
} OthCtrlUi;


/* Prototypes */

int other_ctrl_main(GtkWidget *, CamData *);
OthCtrlUi * new_other_ctrl_ui();
void other_ctrl_ui(OthCtrlUi *, CamData *);
void other_ctrl_grid(OthCtrlUi *, CamData *);
void create_oth_ctrl(struct v4l2_list *, int, OthCtrlUi *, CamData *);
void OnOthCtrlOK(GtkWidget *, gpointer);
void OnOthCtrlDefVal(GtkWidget *, gpointer);
void OnOthCtrlReset(GtkWidget *, gpointer);

extern struct v4l2_list * get_next_oth_ctrl(struct v4l2_list *, CamData *);
extern void app_msg(char*, char *, GtkWidget*);
extern void register_window(GtkWidget *);
extern void deregister_window(GtkWidget *);
extern int cam_ctrl_reset(CamData *, GtkWidget *, char, GtkWidget *);
extern void scale_cam_ctrl(struct v4l2_queryctrl *, int , GtkWidget *, GtkWidget *, CamData *);
extern void menu_cam_ctrl(struct v4l2_list *, int, GtkWidget *, GtkWidget *, CamData *);
extern void radio_cam_ctrl(struct v4l2_list *, int, GtkWidget *, GtkWidget *, CamData *);


/* Globals */

static const char *debug_hdr = "DEBUG-other_ctrl_ui.c ";


/* Display and set other controls */

int other_ctrl_main(GtkWidget *window, CamData *cam_data)
{
    OthCtrlUi *ui;

    /* Initial */
    ui = new_other_ctrl_ui();
    ui->main_window = window;
    ui->cam_data = cam_data;

    /* Create the interface */
    other_ctrl_ui(ui, cam_data);
    gtk_widget_show_all(ui->window);

    /* Register the window */
    register_window(ui->window);

    return TRUE;
}


/* Create new screen data variable */

OthCtrlUi * new_other_ctrl_ui()
{
    OthCtrlUi *ui = (OthCtrlUi *) malloc(sizeof(OthCtrlUi));
    memset(ui, 0, sizeof(OthCtrlUi));

    return ui;
}


/* Create the user interface and set the CallBacks */

void other_ctrl_ui(OthCtrlUi *o_ui, CamData *cam_data)
{
    GtkWidget *ok_btn, *reset_btn, *def_val_btn;
    GtkWidget *main_vbox, *btn_box;

    /* Set up the UI window */
    o_ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(o_ui->window), OTHER_CTRL_UI);
    gtk_container_set_border_width(GTK_CONTAINER(o_ui->window), 5);
    g_object_set_data (G_OBJECT (o_ui->window), "ui", o_ui);

    /* Main view */
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    /* Main grid */
    other_ctrl_grid(o_ui, cam_data);

    /* Box container for action buttons */
    btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(GTK_WIDGET (btn_box), GTK_ALIGN_CENTER);

    /* Reset button */
    reset_btn = gtk_button_new_with_label(" Reset ");
    g_signal_connect(reset_btn, "clicked", G_CALLBACK(OnOthCtrlReset), (gpointer) o_ui);
    gtk_box_pack_end (GTK_BOX (btn_box), reset_btn, FALSE, FALSE, 0);

    /* Default button */
    def_val_btn = gtk_button_new_with_label(" Default ");
    g_signal_connect(def_val_btn, "clicked", G_CALLBACK(OnOthCtrlDefVal), (gpointer) o_ui);
    gtk_box_pack_end (GTK_BOX (btn_box), def_val_btn, FALSE, FALSE, 0);

    /* OK button */
    ok_btn = gtk_button_new_with_label("  OK  ");
    g_signal_connect(ok_btn, "clicked", G_CALLBACK(OnOthCtrlOK), (gpointer) o_ui->window);
    gtk_box_pack_end (GTK_BOX (btn_box), ok_btn, FALSE, FALSE, 0);

    /* Combine everything onto the window */
    gtk_box_pack_start (GTK_BOX (main_vbox), o_ui->cntl_frame, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), btn_box, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(o_ui->window), main_vbox);

    /* Set default 'Enter' key action */
    gtk_widget_set_can_default (ok_btn, TRUE);
    gtk_widget_grab_default (ok_btn);

    /* Exit when window closed */
    o_ui->close_handler = g_signal_connect(o_ui->window, "destroy", G_CALLBACK(OnOthCtrlOK), o_ui->window);

    return;
}


/* Container for main screen items */

void other_ctrl_grid(OthCtrlUi *o_ui, CamData *cam_data)
{  
    int row;
    struct v4l2_list *tmp, *last;
    GtkWidget *label, *hbox;

    /* Container */
    o_ui->oth_cntr = gtk_grid_new();
    gtk_widget_set_name(o_ui->oth_cntr, "ctrl_cntr_oth");
    gtk_grid_set_row_spacing(GTK_GRID (o_ui->oth_cntr), 7);
    gtk_grid_set_column_spacing(GTK_GRID (o_ui->oth_cntr), 5);
    gtk_container_set_border_width (GTK_CONTAINER (o_ui->oth_cntr), 5);

    /* Overall label */
    label = gtk_label_new("Other Settings");
    gtk_widget_set_name(label, "lbl_5");
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, FALSE, 10);
    gtk_grid_attach(GTK_GRID (o_ui->oth_cntr), hbox, 0, 0, 2, 1);
    gtk_widget_set_halign(hbox, GTK_ALIGN_START);
    //gtk_grid_attach(GTK_GRID (o_ui->oth_cntr), label, 0, 0, 2, 1);
    //gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_START);

    /* Iterate thru the camera controls fo find the non-standard (ie not on main window) ones */
    last = NULL;
    row = 2;

    while((tmp = get_next_oth_ctrl(last, cam_data)) != NULL)
    {
	create_oth_ctrl(tmp, row, o_ui, cam_data);
	row++;
	last = tmp;
    }

    /* Iterate thru the camera controls fo find the private ones */
    tmp = cam_data->cam->pctl_head;

    for(tmp = cam_data->cam->pctl_head; tmp != NULL; tmp = last)
    {
	create_oth_ctrl(tmp, row, o_ui, cam_data);
	row++;
	last = tmp->next;
    }

    /* Add some decoration to the control grid */ 
    o_ui->cntl_ev_box = gtk_event_box_new ();
    gtk_widget_set_name(o_ui->cntl_ev_box, "ev_1");
    gtk_container_add(GTK_CONTAINER (o_ui->cntl_ev_box), o_ui->oth_cntr);  

    o_ui->cntl_frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER (o_ui->cntl_frame), o_ui->cntl_ev_box);

    return;
}


/* Create a screen control */

void create_oth_ctrl(struct v4l2_list *tmp, int row, OthCtrlUi *o_ui, CamData *cam_data)
{  
    struct v4l2_queryctrl *qctrl;

    qctrl = (struct v4l2_queryctrl *) tmp->v4l2_data;

    /* Create widgets according to the control type */
    if (qctrl->type == V4L2_CTRL_TYPE_MENU)
    {
	/* Create a new menu control (list or radio) */
	menu_cam_ctrl(tmp, row, o_ui->oth_cntr, o_ui->window, cam_data);
    }
    else if (qctrl->type == V4L2_CTRL_TYPE_BOOLEAN)
    {
	/* Create a new radio control */
	radio_cam_ctrl(tmp, row, o_ui->oth_cntr, o_ui->window, cam_data);
    }
    else
    {
	/* Create a new slider control */
	scale_cam_ctrl(qctrl, row, o_ui->oth_cntr, o_ui->window, cam_data);
    }

    return;
}


/* Callback for Camera Defaults button */

void OnOthCtrlDefVal(GtkWidget *def_val_btn, gpointer user_data)
{ 
    OthCtrlUi *o_ui;

    /* Get data */
    o_ui = (OthCtrlUi *) user_data;

    cam_ctrl_reset(o_ui->cam_data, o_ui->oth_cntr, 'd', o_ui->window);

    return;
}


/* Callback for Reset button */

void OnOthCtrlReset(GtkWidget *reset_btn, gpointer user_data)
{ 
    OthCtrlUi *o_ui;

    /* Get data */
    o_ui = (OthCtrlUi *) user_data;

    cam_ctrl_reset(o_ui->cam_data, o_ui->oth_cntr, 'l', o_ui->window);

    return;
}


// Callback for window close, OK button
// Destroy the window and de-register the window 

void OnOthCtrlOK(GtkWidget *w, gpointer user_data)
{ 
    OthCtrlUi *o_ui;
    GtkWidget *window;

    /* Get data */
    window = (GtkWidget *) user_data;
    o_ui = (OthCtrlUi *) g_object_get_data (G_OBJECT (window), "ui");

    /* Close the window, free the screen data and block any secondary close signal */
    g_signal_handler_block (window, o_ui->close_handler);

    deregister_window(window);
    gtk_window_close(GTK_WINDOW(window));

    free(o_ui);

    return;
}
