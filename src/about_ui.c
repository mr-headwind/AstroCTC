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
** Description:	Show application version, license, credits etc.
**
** Author:	Anthony Buckley
**
** History
**	27-May-2015	Initial code
**
*/



/* Defines */


/* Includes */

#include <stdlib.h>  
#include <string.h>  
#include <libgen.h>  
#include <gtk/gtk.h>  
#include <stdio.h>
#include <defs.h>
#include <version.h>


/* Types */

typedef struct _about_ui
{
    GtkWidget *window;
    GtkWidget *main_window;
    GtkWidget *icon;
    GtkWidget *home_page;
    int close_hndlr_id;
} AboutUi;


/* Prototypes */

int about_main(GtkWidget *);
void about_ui(AboutUi *);
AboutUi * new_about_ui();
GtkWidget * about_ui_hdr(AboutUi *);
GtkWidget * about_ui_misc(AboutUi *);
GtkWidget * about_ui_tabnb(AboutUi *);
GtkWidget * new_page(int);
void add_lic_link(GtkTextBuffer **, GtkWidget **);
void OnAboutClose(GtkWidget*, gpointer);

extern void register_window(GtkWidget *);
extern void deregister_window(GtkWidget *);
extern void strlower(char *, char *);


/* Globals */

static const char *debug_hdr = "DEBUG-about_ui.c ";

static const char *about_text[][2] =
{
    { "License", "Copyright (C) 2016  Anthony Buckley\n\n"
    		 "This program comes with ABSOLUTELY NO WARRANTY.\n"
		 "See the GNU General Public License, version 3 or later for details.\n" },
    { "Credits", "Tony Buckley\t (tony.buckley000@gmail.com)\n" }
};
static const int txt_max = 2;

static const char *license_url[] =
{
    "http://www.gnu.org/licenses/gpl.html",
    "http://www.gnu.org/licenses/gpl-3.0.html"
};
static const int url_max = 2;



/* Application 'About' Information display control */

int about_main(GtkWidget *window)
{
    AboutUi *ui;  

    /* Initial */
    ui = new_about_ui();
    ui->main_window = window;

    /* Create the interface */
    about_ui(ui);
    gtk_widget_show_all(ui->window);

    /* Register the window */
    register_window(ui->window);

    return TRUE;
}


/* Create new screen data variable */

AboutUi * new_about_ui()
{
    AboutUi *ui = (AboutUi *) malloc(sizeof(AboutUi));
    memset(ui, 0, sizeof(AboutUi));

    return ui;
}


/* Create the user interface and set the CallBacks */

void about_ui(AboutUi *p_ui)
{  
    GtkWidget *close_btn;  
    GtkWidget *hdr_box, *misc_box;  
    GtkWidget *mbox, *bbox;  
    GtkWidget *tab_nb;

    GtkWidget *scrollwin;
    GtkWidget *lbox;  
    GtkWidget *label_t, *label_f;  
    GtkWidget *txt_view;  
    GtkTextBuffer *txt_buffer;  
    GtkTextIter iter;
    PangoFontDescription *font_desc;
    char buffer[500];
    int rc;

    /* Set up the UI window */
    p_ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);  
    gtk_window_set_title(GTK_WINDOW(p_ui->window), ABOUT_UI);
    gtk_window_set_position(GTK_WINDOW(p_ui->window), GTK_WIN_POS_NONE);
    gtk_window_set_default_size(GTK_WINDOW(p_ui->window), 360, 370);
    gtk_container_set_border_width(GTK_CONTAINER(p_ui->window), 10);
    gtk_window_set_transient_for (GTK_WINDOW (p_ui->window), GTK_WINDOW (p_ui->main_window));
    gtk_window_set_modal (GTK_WINDOW (p_ui->window), TRUE);

    /* Header */
    hdr_box = about_ui_hdr(p_ui);

    /* General information */
    misc_box = about_ui_misc(p_ui);

    /* License and Credits */
    tab_nb = about_ui_tabnb(p_ui);
    
    /* Close button */
    close_btn = gtk_button_new_with_label("  Close  ");
    g_signal_connect(close_btn, "clicked", G_CALLBACK(OnAboutClose), p_ui);
    bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign (bbox, GTK_ALIGN_CENTER);
    gtk_box_pack_end (GTK_BOX (bbox), close_btn, FALSE, FALSE, 0);

    /* Combine everything onto the window */
    mbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    gtk_box_pack_start (GTK_BOX (mbox), hdr_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (mbox), misc_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (mbox), tab_nb, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (mbox), bbox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(p_ui->window), mbox);  

    /* Exit when window closed */
    p_ui->close_hndlr_id = g_signal_connect(p_ui->window, "destroy", G_CALLBACK(OnAboutClose), p_ui);  

    return;
}


/* Header information - icon, application, version */

GtkWidget * about_ui_hdr(AboutUi *p_ui)
{  
    GtkWidget *hdr_box, *tbox;
    GtkWidget *label_t, *label_v;
    PangoFontDescription *font_desc;
    char *app_icon;
    char *s;

    /* Set up */
    font_desc = pango_font_description_from_string ("Sans 15");

    /* Title and version */
    tbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_halign (tbox, GTK_ALIGN_START);

    label_t = gtk_label_new(TITLE);
    pango_font_description_set_weight(font_desc, PANGO_WEIGHT_BOLD);
    gtk_widget_override_font (GTK_WIDGET (label_t), font_desc);

    label_v = gtk_label_new(VERSION);
    pango_font_description_set_weight(font_desc, PANGO_WEIGHT_NORMAL);
    gtk_widget_override_font (GTK_WIDGET (label_v), font_desc);

    gtk_box_pack_start (GTK_BOX (tbox), label_t, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tbox), label_v, FALSE, FALSE, 0);

    /* Icon */
    s = (char *) malloc(sizeof(TITLE) + 1);
    strlower(TITLE, s);
    app_icon = g_strconcat (PACKAGE_DATA_DIR, "/pixmaps/", s, "/astroctc.png",NULL);
    free(s);
    p_ui->icon = gtk_image_new_from_file(app_icon);
    g_free(app_icon);
    gtk_widget_set_margin_end(GTK_WIDGET (p_ui->icon), 20);

    /* Pack */
    hdr_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign (hdr_box, GTK_ALIGN_START);
    gtk_box_pack_start (GTK_BOX (hdr_box), p_ui->icon, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hdr_box), tbox, FALSE, FALSE, 0);

    pango_font_description_free (font_desc);

    return hdr_box;
}


/* General information - decsription, homepage */

GtkWidget * about_ui_misc(AboutUi *p_ui)
{  
    int i;
    GtkWidget *misc_box, *tbox, *wbox;
    GtkWidget *label_t[3];
    PangoFontDescription *font_desc;
    const char *desc[] = 
    	{
	    "An application to enable planetary and lunar",
	    "astrophotography with a webcam. It can also be",
	    "used as a general purpose webcam application."
    	};
    const int desc_max = 3;

    /* Set up */
    font_desc = pango_font_description_from_string ("Sans 9");
    misc_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(GTK_WIDGET (misc_box), 10);

    /* Description */
    tbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_halign (tbox, GTK_ALIGN_START);
    gtk_widget_set_margin_start(GTK_WIDGET (tbox), 20);

    for(i = 0; i < desc_max; i++)
    {
	label_t[i] = gtk_label_new(desc[i]);
	gtk_widget_override_font (GTK_WIDGET (label_t[i]), font_desc);
	gtk_widget_set_halign (label_t[i], GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (tbox), label_t[i], FALSE, FALSE, 0);
    }

    /* Web page */
    p_ui->home_page = gtk_link_button_new_with_label (APP_URI, "Web page");
    gtk_widget_override_font (GTK_WIDGET (p_ui->home_page), font_desc);
    gtk_widget_override_color(p_ui->home_page, GTK_STATE_FLAG_NORMAL, &DARK_BLUE);
    wbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign (wbox, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (wbox), p_ui->home_page, FALSE, FALSE, 0);

    /* Pack */
    gtk_box_pack_start (GTK_BOX (misc_box), tbox, TRUE, TRUE, 5);
    gtk_box_pack_start (GTK_BOX (misc_box), wbox, FALSE, FALSE, 5);

    pango_font_description_free (font_desc);

    return misc_box;
}


/* License and Credits */

GtkWidget * about_ui_tabnb(AboutUi *p_ui)
{  
    int i;
    GtkWidget *tab_nb;

    /* Setup */
    tab_nb = gtk_notebook_new();
    gtk_notebook_set_scrollable (GTK_NOTEBOOK(tab_nb), TRUE);

    /* Tab pages */
    for(i = 0; i < txt_max; i++)
    {
	if (gtk_notebook_append_page (GTK_NOTEBOOK (tab_nb), 
				      new_page(i), 
				      gtk_label_new (about_text[i][0])) == -1)
	    return NULL;
    }

    return tab_nb;
}


/* New tabbed notebook page */

GtkWidget * new_page(int i)
{  
    GtkWidget *scroll_win;
    GtkWidget *txt_view;  
    GtkTextBuffer *txt_buffer;  
    PangoFontDescription *font_desc;

    font_desc = pango_font_description_from_string ("Sans 9");

    /* TextView */
    txt_view = gtk_text_view_new();
    gtk_widget_override_font (txt_view, font_desc);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (txt_view), GTK_WRAP_WORD);
    txt_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (txt_view));
    gtk_text_buffer_set_text (txt_buffer, about_text[i][1], -1);

    /* Page specific additions */
    switch(i)
    {
    	case 0:
	    add_lic_link(&txt_buffer, &txt_view);
	    break;

	default:
	    break;
    };

    gtk_text_view_set_editable (GTK_TEXT_VIEW (txt_view), FALSE);

    /* Scrolled window for TextView */
    scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scroll_win),
    				   GTK_POLICY_AUTOMATIC,
    				   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request (scroll_win, 200, 135);
    gtk_container_add(GTK_CONTAINER(scroll_win), txt_view);
    gtk_container_set_border_width(GTK_CONTAINER(scroll_win), 3);

    pango_font_description_free (font_desc);

    return scroll_win;
}


/* Add lins to the License page */

void add_lic_link(GtkTextBuffer **txt_buffer, GtkWidget **txt_view)
{  
    int i;
    GtkTextChildAnchor *anchor_lnk;
    GtkTextIter iter;
    PangoFontDescription *font_desc;

    font_desc = pango_font_description_from_string ("Sans 9");

    for(i = 0; i < url_max; i++)
    {
	GtkWidget *lic_url = gtk_link_button_new (license_url[i]);
	gtk_widget_override_font (lic_url, font_desc);
	gtk_widget_override_color(lic_url, GTK_STATE_FLAG_NORMAL, &DARK_BLUE);
	gtk_text_buffer_get_end_iter (*txt_buffer, &iter);
	anchor_lnk = gtk_text_buffer_create_child_anchor (*txt_buffer, &iter);
	gtk_text_view_add_child_at_anchor (GTK_TEXT_VIEW (*txt_view), lic_url, anchor_lnk);
	gtk_text_iter_forward_to_end (&iter);
	gtk_text_buffer_insert (*txt_buffer, &iter, "\n", -1);
    }

    pango_font_description_free (font_desc);

    return;
}


// Callback for window close
// Destroy the window and de-register the window 

void OnAboutClose(GtkWidget *w, gpointer user_data)
{ 
    AboutUi *p_ui;

    p_ui = (AboutUi *) user_data;

    g_signal_handler_block (p_ui->window, p_ui->close_hndlr_id);
    deregister_window(p_ui->window);
    gtk_window_close(GTK_WINDOW(p_ui->window));

    return;
}
