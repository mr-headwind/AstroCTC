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
char * check_screen_res(int *);
void get_screen_res(GdkRectangle *);
void css_adjust_font_sz(char **);


/* Globals */

static const char *debug_hdr = "DEBUG-css.c ";

/*  Yuk! Pain!  !@#$%^  At 18.04 the selectors became proper selector names, not the Gtk name */

/*  16.04
static char *css_data_fhd = 
	"@define-color DARK_BLUE rgba(0%,0%,50%,1.0); "
	"@define-color METAL_GREY rgba(55,83,103,1.0); "
	"GtkButton, GtkEntry, GtkLabel { font-family: Sans; font-size: 12px; }"
	"GtkLabel#data_1 { color: @DARK_BLUE; }"
	"GtkLabel#data_2 { color: #800000; font-family: Sans; font-size: 11px; }"
	"GtkLabel#data_3 { color: #400080; font-family: Sans; font-size: 10px; }"
	"GtkLabel#title_1 { font-family: Sans; font-size: 18px; font-weight: bold; }"
	"GtkLabel#title_2 { font-family: Serif; font-size: 18px; font-style: italic; color: #fa8072; }"
	"GtkLabel#title_3 { font-family: Sans; font-size: 12px; color: @DARK_BLUE;}"
	"GtkLabel#title_4 { font-family: Sans; font-size: 12px; font-weight: bold; }"
	"GtkLabel#title_5 { font-family: Sans; font-size: 12px; color: #e00b40;}"
	"GtkLabel#status { font-family: Sans; font-size: 12px; color: #b8860b; font-style: italic; }"
	"GtkEntry#ent_1 { color: @DARK_BLUE; }"
	"GtkRadioButton#rad_1 { color: @DARK_BLUE; font-family: Sans; font-size: 12px; }"
	"GtkRadioButton > GtkLabel { color: @DARK_BLUE; font-family: Sans; font-size: 12px; }"
	"GtkFrame { background-color: #e6e6fa; border-radius: 8px}"
	"GtkFrame > GtkLabel { color: #800000; font-weight: 500; }"
	"GtkComboboxText * { color: @METAL_GREY; font-family: Sans; font-size: 12px; }"
	"GtkProgressBar#pbar_1 { min-width: 180px; color: @DARK_BLUE; font-family: Sans; font-size: 10px; }"
	"#button_1 * { color: #708090; font-weight: bold; }"
	"GtkNotebook * { font-family: Sans; font-size: 11px; }"
	"GtkTextView { font-family: Sans; font-size: 12px; }"
	"GtkTextView#txtview_1 { font-family: Sans; font-size: 11px; }"
	"GtkLinkButton { font-family: Sans; font-size: 12px; color: @DARK_BLUE; }";
*/

/*  18.04  
*/
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
    char *css_data;

    css_data = check_screen_res(&sd_flg);

    GtkCssProvider *provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *screen = gdk_display_get_default_screen(display);

    gtk_style_context_add_provider_for_screen(screen,
    					      GTK_STYLE_PROVIDER(provider),
    					      GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider),
				    (const gchar *) css_data,
				    -1,
				    &err);

    if (err != NULL)
    {
    	printf("%s set_css  ****css error  %s\n", debug_hdr, err->message); fflush(stdout);
    	g_clear_error (&err);
    }

    if (sd_flg == TRUE)
    	free(css_data);

    g_object_unref(provider);

    return;
}


/* Check the screen resolution and whether to adjust the font size */

char * check_screen_res(int *sd_flg)
{
    GdkRectangle workarea = {0};
    char *css_data_sd;

    get_screen_res(&workarea); 
    //printf ("%s get_screen_res W: %u x H:%u\n", debug_hdr, workarea.width, workarea.height);
	
    // Default font size suits Full HD resolution, but lesser res needs needs lesser font size to stop
    // AstroCTC looking too large. If approaching FHD, keep the default.
    // SD_W and SD_H are not really standard def, but serve as a good cut-off point.
    if (workarea.width > SD_W || workarea.height > SD_H)
    {
    	*sd_flg = FALSE;
    	return css_data_fhd;
    }
    else
    {
	*sd_flg = TRUE;
	css_adjust_font_sz(&css_data_sd);
    	return css_data_sd;
    }
}


/* Get the screen resolution and apply the appropriate font */

void get_screen_res(GdkRectangle *workarea)
{
    gdouble res;
    GdkScreen *scr;

    /* 16.04
    if ((scr = gdk_screen_get_default ()) == NULL)
    	return;
    
    gdk_screen_get_monitor_workarea (scr, 0, workarea);
    */

    /* 18.04
    */
    gdk_monitor_get_workarea (gdk_display_get_primary_monitor (gdk_display_get_default()),
			      workarea);

    return;
}


/* Adjust the font size down */

void css_adjust_font_sz(char **css)
{
    int i, j, fn_len, new_fn_len;
    char *p, *p_new, *p_fhd;
    char num[4];

    /* Copy to a new css string and extract and adjust the font size */
    *css = (char *) malloc(strlen(css_data_fhd) + 1);
    p_new = *css;
    p_fhd = css_data_fhd;

    while ((p = strstr(p_fhd, "px")) != NULL)
    {
    	/* Determine the number of font bytes */
    	for(fn_len = 1; *(p - fn_len) != ' '; fn_len++);
    	
    	fn_len--;

    	/* Determine the font value */
    	i = 0;

	while(i < fn_len)
	{
	    num[i] = *(p - fn_len + i);
	    i++;
	}

	num[i] = '\0';
	//printf("%s fn_len is: %d  font sz: %s\n", debug_hdr, fn_len, num); fflush(stdout);

    	/* Copy up to font */
    	memcpy(p_new, p_fhd, p - p_fhd - fn_len);
    	p_new = p_new + (p - p_fhd - fn_len);

	/* Adjust to new font size and convert back to string */
	i = atoi(num) - SD_SZ;
	sprintf(num, "%d", i);
	//printf("%s new num is: %s\n", debug_hdr, num); fflush(stdout);

	/* Add to new string */
	for(i = 0; num[i] != '\0'; i++)
	{
	    *p_new = num[i];
	    p_new++;
	}

	*p_new = 'p';
	*(p_new + 1) = 'x';
	p_new += 2;

	/* Advance to next */
	p_fhd = p + 2;
    }

    /* Copy any residual bytes */
    while(*p_fhd != '\0')
    {
    	*p_new = *p_fhd;
    	p_new++;
    	p_fhd++;
    }

    *p_new = '\0';
    //printf("%s new css is: %s\n", debug_hdr, *css); fflush(stdout);

    return;
}
