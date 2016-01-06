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
** Description:	View a file in a scrollable viewing window
**
** Author:	Anthony Buckley
**
** History
**	19-Jun-2014	Initial code
**
*/



/* Defines */


/* Includes */
#include <stdlib.h>  
#include <string.h>  
#include <libgen.h>  
#include <gtk/gtk.h>  
#include <stdio.h>
#include <cam.h>
#include <defs.h>


/* Prototypes */

GtkWidget* view_file_main(char  *);
GtkWidget* view_file_ui(char *);
int view_file_init(char  *);
void OnViewFileClose(GtkWidget*, gpointer);
GtkWidget* view_file_ui_menu(GtkWidget *);
int read_file(char *, int);

extern void register_window(GtkWidget *);
extern void deregister_window(GtkWidget *);


/* Globals */

static const char *debug_hdr = "DEBUG-view_file_ui.c ";
static char *viewfile;
static FILE *fd = NULL;


/* Camera Information display control */

GtkWidget* view_file_main(char *fn)
{
    GtkWidget *view_file_window;  

    /* Check and open the file */
    if (! view_file_init(fn))
    	return NULL;

    /* Create the interface */
    view_file_window = view_file_ui(fn);
    gtk_widget_show_all(view_file_window);

    /* Register the window */
    register_window(view_file_window);

    return view_file_window;
}


/* Initial processing */

int view_file_init(char *fn)
{
    /* Open the file for read */
    if ((fd = fopen(fn, "r")) == (FILE *) NULL)
	return FALSE;

    viewfile = fn;

    return TRUE;
}


/* Create the user interface and set the CallBacks */

GtkWidget* view_file_ui(char *fn)
{  
    GtkWidget *view_file_window;  
    GtkWidget *scrollwin;
    GtkWidget *mbox, *bbox, *lbox;  
    GtkWidget *label_t, *label_f;  
    GtkWidget *close_btn;  
    GtkWidget *txt_view;  
    GtkTextBuffer *txt_buffer;  
    GtkTextIter iter;
    GtkWidget *menu_bar;
    PangoFontDescription *font_desc;
    char buffer[500];
    int rc;
    int close_hndlr_id;

    /* Set up the UI window */
    view_file_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);  
    gtk_window_set_title(GTK_WINDOW(view_file_window), VIEW_FILE_UI);
    gtk_window_set_position(GTK_WINDOW(view_file_window), GTK_WIN_POS_NONE);
    gtk_window_set_default_size(GTK_WINDOW(view_file_window), 400, 450);
    gtk_container_set_border_width(GTK_CONTAINER(view_file_window), 10);

    /* Main view */
    mbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    /* Label for file name */
    font_desc = pango_font_description_from_string ("Sans 9");

    label_t = gtk_label_new("Filename:");
    pango_font_description_set_weight(font_desc, PANGO_WEIGHT_BOLD);
    gtk_widget_override_font (GTK_WIDGET (label_t), font_desc);

    label_f = gtk_label_new(fn);
    pango_font_description_set_weight(font_desc, PANGO_WEIGHT_NORMAL);
    gtk_widget_override_color(label_f, GTK_STATE_FLAG_NORMAL, &DARK_BLUE);
    gtk_widget_override_font (GTK_WIDGET (label_f), font_desc);

    pango_font_description_free (font_desc);

    lbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start (GTK_BOX (lbox), label_t, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (lbox), label_f, FALSE, FALSE, 0);
    
    /* Scrolled window for TextView */
    scrollwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrollwin),
    				   GTK_POLICY_AUTOMATIC,
    				   GTK_POLICY_AUTOMATIC);

    /* Text area for file contents */
    txt_view = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(scrollwin), txt_view);
    txt_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (txt_view));
    gtk_widget_set_size_request (scrollwin, 500, 400);
    gtk_text_buffer_set_text (txt_buffer, "", -1);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (txt_view), FALSE);

    /* Populate the text area */
    rc = TRUE;

    while(rc != EOF)
    {
	rc = read_file(buffer, sizeof(buffer));
	gtk_text_buffer_get_end_iter (txt_buffer, &iter);
	gtk_text_buffer_insert (txt_buffer, &iter, buffer, -1);
	gtk_text_iter_forward_to_end (&iter);
    }
    
    /* Close button */
    close_btn = gtk_button_new_with_label("  Close  ");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(OnViewFileClose), view_file_window);
    bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_end (GTK_BOX (bbox), close_btn, FALSE, FALSE, 0);
    gtk_widget_set_halign(GTK_WIDGET (bbox), GTK_ALIGN_CENTER);

    /* Menu */
    menu_bar = view_file_ui_menu(view_file_window);

    /* Combine everything onto the window */
    gtk_box_pack_start (GTK_BOX (mbox), menu_bar, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (mbox), lbox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (mbox), scrollwin, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (mbox), bbox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(view_file_window), mbox);  

    /* Exit when window closed */
    close_hndlr_id = g_signal_connect(view_file_window, "destroy", G_CALLBACK(OnViewFileClose), view_file_window);  
    g_object_set_data (G_OBJECT (view_file_window), "close_hndlr_id", GINT_TO_POINTER (close_hndlr_id));

    return view_file_window;
}


/*
** Menu function for camera information window
**
**  File
**   - Close
*/

GtkWidget* view_file_ui_menu(GtkWidget *window)
{
    GtkWidget *menu_bar;
    GtkWidget *file_menu;
    GtkWidget *file_hdr;
    GtkWidget *file_close;
    GtkAccelGroup *accel_group = NULL;

    /* Create menubar */
    menu_bar = gtk_menu_bar_new();

    /* File Menu */
    file_menu = gtk_menu_new();

    /* File menu items */
    file_close = gtk_menu_item_new_with_mnemonic ("_Close");

    /* Add to menu */
    gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), file_close);

    /* Callbacks */
    g_signal_connect_swapped (file_close, "activate", G_CALLBACK (OnViewFileClose), window); 

    /* Show menu items */
    gtk_widget_show (file_close);

    /* File header menu */
    file_hdr = gtk_menu_item_new_with_mnemonic ("_File");
    gtk_widget_show (file_hdr);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_hdr), file_menu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_hdr);

    /* Accelerators */
    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

    gtk_widget_add_accelerator(file_close, "activate", accel_group, GDK_KEY_c,
    			       GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 

    return menu_bar;
}


// Callback for window close
// Destroy the window and de-register the window 

void OnViewFileClose(GtkWidget *window, gpointer user_data)
{ 
    int close_hndlr_id;

    close_hndlr_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (window), "close_hndlr_id"));
    g_signal_handler_block (window, close_hndlr_id);
    deregister_window(window);
    gtk_window_close(GTK_WINDOW(window));

    return;
}


/* Read the camera information file */

int read_file(char *buf, int sz_len)
{
    int i, max;
    char c;

    i = 0;
    max = sz_len - 1;
    buf[0] = '\0';
    
    while((c = fgetc(fd)) != EOF)
    {
    	buf[i++] = c;

    	if (i >= max)
	    break;
    }

    buf[i] = '\0';

    if (c == EOF)
    {
    	fclose(fd);
    	fd = NULL;
    }

    return (int) c;
}
