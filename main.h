/*
**  Copyright (C) 2016 Anthony Buckley
** 
**  This file is part of AstroTWC.
** 
**  AstroTWC is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**  
**  AstroTWC is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**  
**  You should have received a copy of the GNU General Public License
**  along with AstroTWC.  If not, see <http://www.gnu.org/licenses/>.
*/



/*
** Description:	Main program include file
**
** Author:	Anthony Buckley
**
** History
**	08-Sep-2014	Initial
**
*/




/* Defines */

#ifndef MAIN_HDR
#define MAIN_HDR
#endif

/* Structure to contain main interface items for easy access */

typedef struct _main_ui
{
    /* Main view widgets */
    GtkWidget *window;
    GtkWidget *video_window;  
    GtkWidget *status_info;  

    /* Menu items */
    GtkWidget *cam_hdr;
    GtkWidget *opt_ret;
    GtkWidget *cam_menu;

    /* Toolbar(2) widgets and items */
    GtkWidget *cbox_dur;
    GtkWidget *cbox_entry_dur;
    GtkWidget *cbox_seq;
    GtkWidget *obj_title;
    GtkWidget *cap_ui;
    GtkToolItem *cap_start_tb;
    GtkWidget *cap_stop;
    GtkToolItem *cap_stop_tb;
    GtkWidget *cap_pause;
    GtkToolItem *cap_pause_tb;
    GtkWidget *snap_ui;
    GtkToolItem *snap_tb;
    GtkWidget *cbox_profile;
    GtkWidget *save_profile_btn;

    /* Control Panel widgets */
    GtkWidget *cntl_hdg;
    GtkWidget *cntl_ev_box;
    GtkWidget *cntl_grid;
    GtkWidget *cbox_clrfmt;
    GtkWidget *cbox_res;
    GtkWidget *cbox_fps;
    GtkWidget *oth_ctrls_btn, *reset_btn, *def_val_btn;

    /* Callback Handlers */
    int close_hndlr_id;
    int preset_hndlr_id;
    int nvexp_hndlr_id;

    int duration;
    int no_of_frames;
    int thread_init;
    int night_view;
} MainUi;
