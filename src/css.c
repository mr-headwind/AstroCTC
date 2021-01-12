/*
**  Copyright (C) 2017 Anthony Buckley
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
** Description:
**  Screen appearance setup
**
** Author:	Anthony Buckley
**
** History
**	07-Nov-2020	Initial code
**
*/


/* Defines */

#define SD_W 1600
#define SD_H 900
#define SD_SZ 3


/* Includes */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>  
#include <gdk/gdk.h> 


/* Prototypes */

void set_css();


/* Globals */

static const char *debug_hdr = "DEBUG-css.c ";

static char *css_data_fhd = 
	"@define-color DARK_BLUE rgba(0%,0%,50%,1.0); "
	"@define-color MID_BLUE rgba(50%,50%,100%,1.0); "
	"@define-color METAL_GREY rgba(55,83,103,1.0); "
	"@define-color BLUE_GRAY rgba(90%,90%,100%,1.0); "
	"@define-color RED1 rgba(80%,10%,10%,1.0); "

	"button, entry, label { font-family: Sans; font-size: 13px; }"
	"menuitem, menubar, menu { font-family: Sans; font-size: 12px; }"
	"spinbutton { font-family: Sans; font-size: 12px; }"
	"checkbutton { font-family: Sans; font-size: 9px; }"
	"scale { font-family: Sans; font-size: 12px; }"
	"button.scale { font-family: Sans; font-size: 9px; }"
	"button.link { font-family: Sans; font-size: 12px; color: @DARK_BLUE; }"
	"frame { background-color: #e6e6fa; border-radius: 8px}"

	"combobox * { color: @METAL_GREY; font-family: Sans; font-size: 9px; }"
	"textview * { font-family: Sans; font-size: 12px; }"
	"frame > label { color: #800000; font-weight: 500; }"						// Crimson
	"radiobutton > label { color: @DARK_BLUE; font-family: Sans; font-size: 12px; }"

	"button#btn_1 { font-family: Sans; font-size: 9px; }"
	"label#lbl_1 { font-family: Sans; font-size: 12px; }"
	"label#lbl_2 { font-family: Sans; font-size: 12px; color: @DARK_BLUE; }"
	"label#lbl_3 { font-family: Sans; font-size: 12px; font-style: italic; color: @RED1; }"
	"label#lbl_4 { font-family: Sans; font-size: 12px; font-style: italic; color: @MID_BLUE; }"
	"label#lbl_5 { font-family: Sans; font-size: 14px; color: @DARK_BLUE; }"
	"label#lbl_6 { font-family: Sans; font-size: 12px; font-weight: bold; }"
	"label#lbl_7 { font-family: Sans; font-size: 13px; }"
	"label#lbl_8 { font-family: Sans; font-size: 10px; font-style: italic; }"
	"label#lbl_9 { font-family: Sans; font-size: 12px; font-style: italic; color: @DARK_BLUE; }"
	"label#lbl_10 { font-family: Sans; font-size: 20px; font-weight: bold; }"
	"label#lbl_11 { font-family: Sans; font-size: 12px; font-weight: bold; }"
	"label#status { font-family: Sans; font-size: 12px; color: #b8860b; font-style: italic; }"	// Dark yellow
	"label#cam_hdg { font-family: Sans; font-size: 14px; font-weight: bold; color: @METAL_GREY; }"
	"entry#ent_1 { font-family: Sans; font-size: 12px; }"
	"entry#ent_2 { font-family: Sans; font-size: 14px; }"
	"entry#jpeg_qual, entry#fits_bits, entry#snap_delay { font-family: Sans; font-size: 12px; }"
	"eventbox#ev_1 { background-color: @BLUE_GRAY; }"
	"textview#txtview_1 { font-family: Sans; font-size: 12px; }"

	"#capt_cntr * { font-family: Sans; font-size: 12px; }"
	"#ctrl_cntr_main > label { font-family: Sans; font-size: 12px; font-weight: bold; }"
	"#ctrl_cntr_oth > label { font-family: Sans; font-size: 12px; font-weight: bold; }"
	"#more_ctl_btn, #def_val_btn, #reset_val_btn { font-family: Sans; font-size: 9px; }"
	"#fmt_head, #exp_head { color: @DARK_BLUE; }"
	"#combobox_def * { color: @RED1; font-family: Sans; font-size: 9px; }"
	"#combobox_1 * { color: @METAL_GREY; font-family: Sans; font-size: 11px; }"
	"#combobox_2 * { color: @METAL_GREY; font-family: Sans; font-size: 12px; }";


// These don't work
//"GtkLabel#title_deco1 { font: Comic Sans 15; font-weight: 500; color: #fa8072 }"
//"GtkLabel#title_deco2 { font-family: Comic Sans; font-size: 15px; font-style: italic; color: #fa8072 }"


/* Set up provider data and apply */

void set_css()
{
    int sd_flg;
    GError *err = NULL;

    GtkCssProvider *provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *screen = gdk_display_get_default_screen(display);

    gtk_style_context_add_provider_for_screen(screen,
    					      GTK_STYLE_PROVIDER(provider),
    					      GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider),
				    (const gchar *) css_data_fhd,
				    -1,
				    &err);

    if (err != NULL)
    {
    	printf("%s set_css  ****css error  %s\n", debug_hdr, err->message); fflush(stdout);
    	g_clear_error (&err);
    }

    g_object_unref(provider);

    return;
}
