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
** Description: Preferences user interface and management.
**
** Author:	Anthony Buckley
**
** History
**	8-Aug-2014	Initial code
**      20-Nov-2020     Changes to move to css
**
*/


/* Includes */

#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <codec.h>
#include <cam.h>
#include <defs.h>
#include <preferences.h>


/* Defines */


/* Types */

typedef struct _prefs_ui
{
    GtkWidget *window;
    GtkWidget *pref_cntr;
    GtkWidget *cbox_fmt;
    GtkWidget *jpeg_qual;
    GtkWidget *snap_delay;
    GtkWidget *snap_cntr;
    GtkWidget *jqual_cntr;
    GtkWidget *opt_cntr;
    GtkWidget *cbox_fits_bits;
    GtkWidget *cbox_codec;
    GtkWidget *capt_duration;
    GtkWidget *capt_frames;
    GtkWidget *fn_grid;
    GtkWidget *fn_tmpl;
    GtkWidget *capt_dir;
    GtkWidget *audio_hbox;
    GtkWidget *title_hbox;
    GtkWidget *meta_hbox;
    int close_handler;
    int fn_err, qual_alloc_width;
    GList *hide_list;
} PrefUi;


/* Prototypes */

int user_prefs_main(GtkWidget *);
int user_prefs_init(GtkWidget *);
PrefUi * new_pref_ui();
void user_prefs_ui(PrefUi *);
void pref_control(PrefUi *);
void image_type(PrefUi *);
void video_capture(PrefUi *);
void fn_template(PrefUi *);
void file_location(PrefUi *);
void audio_mute(PrefUi *);
void empty_title(PrefUi *);
void meta_data_file(PrefUi *);
void pref_label_1(char *, GtkWidget **, GtkAlign, int);
void pref_label_2(char *, GtkWidget **, GtkAlign, int, int);
void pref_label_3(char *, GtkWidget *, int *);
void pref_radio(char *, GtkWidget *, char, int *);
void pref_boolean(char *, char *, int, GtkWidget **);
void set_fn_template(char, char, char, char *, PrefUi *);
int validate_fn_prefs(char, char, char);
int set_fn_idx(char, int, char [][10]);
int set_fn_val(char);
void set_fn_pref(char *, char *, char *, const gchar *);
void set_fn_handler(PrefUi *);
void set_tmpl_colour(GtkWidget *, char *);
char find_active_by_parent(GtkWidget *, char);
void get_file_name(char *, int, char *, char *, char *, char, char, char);
void file_name_item(char *, int, char, char, char, char, char *, char *, char *); 
int read_user_prefs(GtkWidget *);
int write_user_prefs(GtkWidget *);
void set_default_prefs();
void init_snapshot_prefs();
void init_capture_prefs();
void init_dir_prefs();
void init_fn_prefs();
void init_profile_prefs();
void init_audio_prefs();
void init_title_prefs();
void init_metadata_prefs();
void set_user_prefs(PrefUi *);
int get_user_pref(char *, char **);
void get_user_pref_idx(int, char *, char **);
int match_key_val_combo(char *, char *, int, char **);
void get_pref_key(int, char *);
void get_pref_val(int, char **);
int set_user_pref(char *, char *);
int add_user_pref(char *, char *);
int add_user_pref_idx(char *, char *, int);
void delete_user_pref(char *);
int pref_save_reqd(PrefUi *);
int pref_changed(char *, char *);
int validate_pref(PrefUi *);
void free_prefs();
void OnSetFmt(GtkWidget *, gpointer);
void OnFnPref(GtkWidget *, gpointer);
void OnPrefBrowse(GtkWidget*, gpointer);
void OnCodecProp(GtkWidget*, gpointer);
void OnPrefClose(GtkWidget*, gpointer);
gboolean OnPrefDelete(GtkWidget*, GdkEvent *, gpointer);
void OnPrefSave(GtkWidget*, gpointer);


extern void log_msg(char*, char*, char*, GtkWidget*);
extern void register_window(GtkWidget *);
extern void deregister_window(GtkWidget *);
extern char * app_dir_path();
extern char * home_dir();
extern GtkWidget * find_widget_by_name(GtkWidget *, char *);
extern void info_dialog(GtkWidget *, char *, char *);
extern gint query_dialog(GtkWidget *, char *, char *);
extern int check_dir(char *);
extern int make_dir(char *);
extern codec_t * get_codec_arr(int *);
extern codec_t * get_codec_idx(int);
extern int is_ui_reg(char *, int);
extern int codec_ui_main(int, GtkWidget *);
extern void init_codec_prop_prefs();
extern int close_ui(char *);
extern int val_str2numb(char *, int *, char *, GtkWidget *);
extern void string_trim(char *);


/* Globals */

static const char *debug_hdr = "DEBUG-prefs_ui.c ";
static int save_indi;
static GList *pref_list = NULL;
static GList *pref_list_head = NULL;
static int pref_count;


/* Display and maintenance of user preferences */

int user_prefs_main(GtkWidget *window)
{
    PrefUi *ui;

    /* Initial */
    if (! user_prefs_init(window))
    	return FALSE;

    ui = new_pref_ui();

    /* Create the interface */
    user_prefs_ui(ui);

    /* Register the window */
    register_window(ui->window);

    return TRUE;
}


/* Initial checks and values */

int user_prefs_init(GtkWidget *window)
{
    /* Initial */
    save_indi = FALSE;

    /* Should at least be a default set of user preferences */
    if (pref_count == 0)
    {
	log_msg("CAM0008", "No user preferences", "CAM0008", window);
    	return FALSE;
    }

    return TRUE;
}


/* Create new screen data variable */

PrefUi * new_pref_ui()
{
    PrefUi *ui = (PrefUi *) malloc(sizeof(PrefUi));
    memset(ui, 0, sizeof(PrefUi));

    return ui;
}


/* Create the user interface and set the CallBacks */

void user_prefs_ui(PrefUi *p_ui)
{
    int init;
    GtkWidget *save_btn, *close_btn;
    GtkWidget *main_vbox, *btn_box;
    GtkWidget *tmp;

    /* Set up the UI window */
    p_ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(p_ui->window), USER_PREFS_UI);
    gtk_window_set_position(GTK_WINDOW(p_ui->window), GTK_WIN_POS_NONE);
    gtk_window_set_default_size(GTK_WINDOW(p_ui->window), 475, 400);
    //gtk_window_set_default_size(GTK_WINDOW(p_ui->window), 475, 350);
    gtk_container_set_border_width(GTK_CONTAINER(p_ui->window), 10);
    g_object_set_data (G_OBJECT (p_ui->window), "ui", p_ui);

    /* Main view */
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    /* Main update or view grid */
    pref_control(p_ui);

    /* Box container for action buttons */
    btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(GTK_WIDGET (btn_box), GTK_ALIGN_CENTER);

    /* Close button */
    close_btn = gtk_button_new_with_label("  Close  ");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(OnPrefClose), p_ui->window);
    gtk_box_pack_end (GTK_BOX (btn_box), close_btn, FALSE, FALSE, 0);

    /* Save button */
    save_btn = gtk_button_new_with_label("  Apply  ");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(OnPrefSave), (gpointer) p_ui);
    gtk_box_pack_end (GTK_BOX (btn_box), save_btn, FALSE, FALSE, 0);

    /* Combine everything onto the window */
    gtk_box_pack_start (GTK_BOX (main_vbox), p_ui->pref_cntr, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), btn_box, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(p_ui->window), main_vbox);

    /* Exit when window closed */
    p_ui->close_handler = g_signal_connect(p_ui->window, "delete-event", G_CALLBACK(OnPrefDelete), NULL);

    /* Show (or not) */
    gtk_widget_show_all(p_ui->window);

    p_ui->hide_list = g_list_first(p_ui->hide_list);

    while(p_ui->hide_list != NULL)
    {
	tmp = p_ui->hide_list->data;
	gtk_widget_hide (tmp);
	p_ui->hide_list = g_list_next(p_ui->hide_list);
    }

    g_list_free(p_ui->hide_list);
    OnSetFmt(p_ui->cbox_fmt, p_ui);
    gtk_window_resize(GTK_WINDOW(p_ui->window), 475, 350);

    return;
}


/* Control container for user preference adjustments */

void pref_control(PrefUi *p_ui)
{  
    GtkWidget *label;  
    int row;

    /* Layout setup */
    p_ui->pref_cntr = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_name(p_ui->pref_cntr, "pref_cntr");
    gtk_container_set_border_width (GTK_CONTAINER (p_ui->pref_cntr), 10);

    /* Image type (and optional quality) for snapshots */
    image_type(p_ui);

    /* Video capture */
    video_capture(p_ui);

    /* Filename template */
    fn_template(p_ui);

    /* Capture and snapshot location */
    file_location(p_ui);

    /* General Capture and Snapshot options - Audio mute, Empty Title, Meta Data */
    pref_label_1("General Capture & Snapshot", &(p_ui->pref_cntr), GTK_ALIGN_START, 10);
    audio_mute(p_ui);
    empty_title(p_ui);
    meta_data_file(p_ui);

    return;
}


/* Image type options */

void image_type(PrefUi *p_ui)
{  
    int i, curr_idx;
    char *p, *img_p;
    char s[50];

    const char *fmts[] = { "jpg", "bmp", "png", "ppm", "fits" };
    const int fmt_count = 5;

    const char *fits_bits[] = { "16 bit", "32 bit" };
    const int fits_count = 2;

    /* Heading */
    pref_label_1("Snapshot", &(p_ui->pref_cntr), GTK_ALIGN_START, 0);

    /* Put in horizontal box */
    p_ui->snap_cntr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

    /* Label */
    pref_label_2("Image Type", &(p_ui->snap_cntr), GTK_ALIGN_END, 20, 0);

    /* Combobox */
    p_ui->cbox_fmt = gtk_combo_box_text_new();
    gtk_widget_set_name (p_ui->cbox_fmt, "combobox_2");

    /* Add list items, note current value index and set active */
    curr_idx = 0;
    get_user_pref(IMAGE_TYPE, &img_p);

    for(i = 0; i < fmt_count; i++)
    {
    	sprintf(s, "%d", i);
    	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (p_ui->cbox_fmt), s, fmts[i]);

    	if (strcmp(img_p, fmts[i]) == 0)
	    curr_idx = i;
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (p_ui->cbox_fmt), curr_idx);
    gtk_box_pack_start (GTK_BOX (p_ui->snap_cntr), p_ui->cbox_fmt, FALSE, FALSE, 5);
    
    /* Put Optional widgets in vertical box */
    p_ui->opt_cntr = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    /* Put Quality widgets in horizontal box */
    p_ui->jqual_cntr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    g_object_set_data (G_OBJECT (p_ui->jqual_cntr), "all_pad", GINT_TO_POINTER (16));
    gtk_widget_set_margin_top (p_ui->jqual_cntr, 2);

    /* Label */
    pref_label_2("Quality (%)", &(p_ui->jqual_cntr), GTK_ALIGN_END, 0, 0);

    /* Jpeg quality */
    p_ui->jpeg_qual = gtk_entry_new();
    gtk_widget_set_name(p_ui->jpeg_qual, "jpeg_qual");
    gtk_widget_set_halign(GTK_WIDGET (p_ui->jpeg_qual), GTK_ALIGN_START);
    gtk_entry_set_max_length(GTK_ENTRY (p_ui->jpeg_qual), 4);
    gtk_entry_set_width_chars(GTK_ENTRY (p_ui->jpeg_qual), 4);
    gtk_box_pack_start (GTK_BOX (p_ui->jqual_cntr), p_ui->jpeg_qual, FALSE, FALSE, 3);

    get_user_pref(JPEG_QUALITY, &p);
    gtk_entry_set_text(GTK_ENTRY (p_ui->jpeg_qual), p);
    gtk_box_pack_start (GTK_BOX (p_ui->opt_cntr), p_ui->jqual_cntr, FALSE, FALSE, 0);

    if (strcmp(img_p, fmts[0]) != 0)
	p_ui->hide_list = g_list_prepend(p_ui->hide_list, p_ui->jqual_cntr);

    /* FITS bits per pixel */
    p_ui->cbox_fits_bits = gtk_combo_box_text_new();
    gtk_widget_set_name(p_ui->cbox_fits_bits, "combobox_2");
    gtk_widget_set_halign(GTK_WIDGET (p_ui->cbox_fits_bits), GTK_ALIGN_START);
    g_object_set_data (G_OBJECT (p_ui->cbox_fits_bits), "all_pad", GINT_TO_POINTER (16));

    curr_idx = 0;
    get_user_pref(FITS_BITS, &p);

    for(i = 0; i < fits_count; i++)
    {
    	sprintf(s, "%d", i);
    	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (p_ui->cbox_fits_bits), s, fits_bits[i]);

    	if (strncmp(p, fits_bits[i], 2) == 0)
	    curr_idx = i;
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (p_ui->cbox_fits_bits), curr_idx);
    gtk_box_pack_start (GTK_BOX (p_ui->opt_cntr), p_ui->cbox_fits_bits, FALSE, FALSE, 0);

    if (strcmp(img_p, fmts[4]) != 0)
	p_ui->hide_list = g_list_prepend(p_ui->hide_list, p_ui->cbox_fits_bits);

    i = gtk_widget_get_margin_end (p_ui->snap_cntr);
    g_object_set_data (G_OBJECT (p_ui->snap_cntr), "init_pad", GINT_TO_POINTER (i));
    gtk_box_pack_start (GTK_BOX (p_ui->snap_cntr), p_ui->opt_cntr, FALSE, FALSE, 5);

    /* Label */
    pref_label_2("Delay (secs)", &(p_ui->snap_cntr), GTK_ALIGN_END, 0, 5);

    /* Default delay */
    p_ui->snap_delay = gtk_entry_new();
    gtk_widget_set_name(p_ui->snap_delay, "snap_delay");
    gtk_widget_set_halign(GTK_WIDGET (p_ui->snap_delay), GTK_ALIGN_START);
    gtk_entry_set_max_length(GTK_ENTRY (p_ui->snap_delay), 2);
    gtk_entry_set_width_chars(GTK_ENTRY (p_ui->snap_delay), 4);
    gtk_box_pack_start (GTK_BOX (p_ui->snap_cntr), p_ui->snap_delay, FALSE, FALSE, 5);

    get_user_pref(SNAPSHOT_DELAY, &p);
    gtk_entry_set_text(GTK_ENTRY (p_ui->snap_delay), p);

    gtk_box_pack_start (GTK_BOX (p_ui->pref_cntr), p_ui->snap_cntr, FALSE, FALSE, 0);
    g_signal_connect(p_ui->cbox_fmt, "changed", G_CALLBACK(OnSetFmt), (gpointer) p_ui);

    return;
}


/* Video capture options */

void video_capture(PrefUi *p_ui)
{  
    GtkWidget *prop_btn;
    GtkWidget *h_box;
    int i, curr_idx, codec_max;
    char *p;
    char s[50];
    codec_t *p_codec;

    /* Heading */
    pref_label_1("Video Capture", &(p_ui->pref_cntr), GTK_ALIGN_START, 0);

    /* Put in horizontal box */
    h_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

    /* Label */
    pref_label_2("Capture Format", &h_box, GTK_ALIGN_END, 20, 0);

    /* Combobox */
    p_ui->cbox_codec = gtk_combo_box_text_new();
    gtk_widget_set_name (p_ui->cbox_codec, "combobox_2");

    /* Get the codec array */
    p_codec = get_codec_arr(&codec_max);

    /* Add list items, note current value index and set active */
    curr_idx = 0;
    get_user_pref(CAPTURE_FORMAT, &p);

    for(i = 0; i < codec_max; i++, p_codec++)
    {
    	sprintf(s, "%d", i);
    	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (p_ui->cbox_codec), s, p_codec->short_desc);

    	if (strcmp(p, p_codec->fourcc) == 0)
	    curr_idx = i;
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (p_ui->cbox_codec), curr_idx);
    gtk_box_pack_start (GTK_BOX (h_box), p_ui->cbox_codec, FALSE, FALSE, 5);
    
    /* Properties */
    prop_btn = gtk_button_new_with_label("Properties...");
    gtk_widget_set_name (prop_btn, "btn_1");
    g_signal_connect(prop_btn, "clicked", G_CALLBACK(OnCodecProp), (gpointer) p_ui);
    gtk_box_pack_start (GTK_BOX (h_box), prop_btn, FALSE, FALSE, 3);

    /* Label */
    pref_label_2("Duration (secs)", &h_box, GTK_ALIGN_END, 0, 0);

    /* Duration (default) */
    p_ui->capt_duration = gtk_entry_new();
    gtk_widget_set_name(p_ui->capt_duration, "capt_dur");
    gtk_widget_set_halign(GTK_WIDGET (p_ui->capt_duration), GTK_ALIGN_START);
    gtk_entry_set_max_length(GTK_ENTRY (p_ui->capt_duration), 4);
    gtk_entry_set_width_chars(GTK_ENTRY (p_ui->capt_duration), 4);
    gtk_box_pack_start (GTK_BOX (h_box), p_ui->capt_duration, FALSE, FALSE, 3);

    get_user_pref(CAPTURE_DURATION, &p);
    gtk_entry_set_text(GTK_ENTRY (p_ui->capt_duration), p);

    gtk_box_pack_start (GTK_BOX (p_ui->pref_cntr), h_box, FALSE, FALSE, 0);

    return;
}


/* Timestamp inclusion */

void fn_template(PrefUi *p_ui)
{  
    int row;
    char *p;
    char id, tt, ts;
    char s[30];

    /* Heading */
    pref_label_1("File Names", &(p_ui->pref_cntr), GTK_ALIGN_START, 7);

    /* Place the file name template items in a grid */
    p_ui->fn_grid = gtk_grid_new();
    row = 0;

    /* Sequence No */
    get_user_pref(FN_ID, &p);
    id = *p;
    pref_label_3("Sequence Id", p_ui->fn_grid, &row);
    pref_radio("id", p_ui->fn_grid, id, &row);
    row++;

    /* Title */
    get_user_pref(FN_TITLE, &p);
    tt = *p;
    pref_label_3("Title", p_ui->fn_grid, &row);
    pref_radio("tt", p_ui->fn_grid, tt, &row);
    row++;

    /* Timestamp */
    get_user_pref(FN_TIMESTAMP, &p);
    ts = *p;
    pref_label_3("Timestamp", p_ui->fn_grid, &row);
    pref_radio("ts", p_ui->fn_grid, ts, &row);
    row++;

    /* Connect signal handler for radio buttons */
    set_fn_handler(p_ui);

    /* Mock-up */
    set_fn_template(id, tt, ts, s, p_ui);
    p_ui->fn_tmpl = gtk_label_new(s);
    set_tmpl_colour(p_ui->fn_tmpl, s);
    gtk_widget_set_halign(GTK_WIDGET (p_ui->fn_tmpl), GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID (p_ui->fn_grid), p_ui->fn_tmpl, 1, row, 3, 1);

    /* Add grid to main vbox */
    gtk_box_pack_start (GTK_BOX (p_ui->pref_cntr), p_ui->fn_grid, FALSE, FALSE, 0);

    return;
}


/* Capture and Snapshot location directory */

void file_location(PrefUi *p_ui)
{  
    GtkWidget *browse_btn;
    GtkWidget *h_box;
    char *p;

    /* Heading */
    pref_label_1("Capture Location", &(p_ui->pref_cntr), GTK_ALIGN_START, 7);

    /* Put in horizontal box */
    h_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

    /* Directory */
    p_ui->capt_dir = gtk_entry_new();
    gtk_widget_set_name(p_ui->capt_dir, "capt_dir");
    gtk_widget_set_halign(GTK_WIDGET (p_ui->capt_dir), GTK_ALIGN_START);
    gtk_entry_set_max_length(GTK_ENTRY (p_ui->capt_dir), 256);
    gtk_entry_set_width_chars(GTK_ENTRY (p_ui->capt_dir), 40);
    gtk_box_pack_start (GTK_BOX (h_box), p_ui->capt_dir, FALSE, FALSE, 3);

    get_user_pref(CAPTURE_LOCATION, &p);
    gtk_entry_set_text (GTK_ENTRY (p_ui->capt_dir), p);

    /* Browse button */
    browse_btn = gtk_button_new_with_label("Browse...");
    gtk_widget_set_name (browse_btn, "btn_1");
    g_signal_connect(browse_btn, "clicked", G_CALLBACK(OnPrefBrowse), (gpointer) p_ui);
    gtk_box_pack_start (GTK_BOX (h_box), browse_btn, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (p_ui->pref_cntr), h_box, FALSE, FALSE, 0);

    return;
}


/* Audio mute - True means silence */

void audio_mute(PrefUi *p_ui)
{  
    int i;
    char *p;

    /* Put in horizontal box */
    p_ui->audio_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

    /* Label */
    pref_label_2("Audio Mute", &p_ui->audio_hbox, GTK_ALIGN_END, 20, 0);

    /* Set up current preference */
    get_user_pref(AUDIO_MUTE, &p);

    i = TRUE;

    if (p != NULL)
    	if (atoi(p) == 0)
	    i = FALSE;

    pref_boolean("Off", "On", i, &p_ui->audio_hbox);
    gtk_widget_set_tooltip_text (p_ui->audio_hbox, "This will only apply if the camera supports audio");
    gtk_box_pack_start (GTK_BOX (p_ui->pref_cntr), p_ui->audio_hbox, FALSE, FALSE, 0);

    return;
}


/* Empty Title warning */

void empty_title(PrefUi *p_ui)
{  
    int i;
    char *p;

    /* Put in horizontal box */
    p_ui->title_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

    /* Label */
    pref_label_2("Display Empty Title warning message", &p_ui->title_hbox, GTK_ALIGN_END, 20, 0);

    /* Set up current preference */
    get_user_pref(WARN_EMPTY_TITLE, &p);

    i = TRUE;

    if (p != NULL)
    	if (atoi(p) == 0)
	    i = FALSE;

    pref_boolean("Off", "On", i, &p_ui->title_hbox);
    gtk_box_pack_start (GTK_BOX (p_ui->pref_cntr), p_ui->title_hbox, FALSE, FALSE, 0);

    return;
}


/* Write Meta Data file for capture and snapshot */

void meta_data_file(PrefUi *p_ui)
{  
    int i;
    char *p;

    /* Put in horizontal box */
    p_ui->meta_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

    /* Label */
    pref_label_2("Write a Meta Data file", &p_ui->meta_hbox, GTK_ALIGN_END, 20, 0);

    /* Set up current preference */
    get_user_pref(META_DATA, &p);

    i = TRUE;

    if (p != NULL)
    	if (atoi(p) == 0)
	    i = FALSE;

    pref_boolean("Off", "On", i, &p_ui->meta_hbox);
    gtk_box_pack_start (GTK_BOX (p_ui->pref_cntr), p_ui->meta_hbox, FALSE, FALSE, 0);

    return;
}


/* Create a label */

void pref_label_1(char *title, GtkWidget **cntr, GtkAlign align, int top)
{  
    GtkWidget *label;

    label = gtk_label_new(title);
    gtk_widget_set_name (label, "lbl_9");
    gtk_widget_set_halign(GTK_WIDGET (label), align);
    gtk_widget_set_margin_top (label, top);
    gtk_box_pack_start (GTK_BOX (*cntr), label, FALSE, FALSE, 0);

    return;
}


/* Create a label */

void pref_label_2(char *title, GtkWidget **cntr, GtkAlign align, int start, int pad)
{  
    GtkWidget *label;

    label = gtk_label_new(title);
    gtk_widget_set_name (label, "lbl_11");
    gtk_widget_set_halign(GTK_WIDGET (label), align);
    gtk_widget_set_margin_start (label, start);
    gtk_box_pack_start (GTK_BOX (*cntr), label, FALSE, FALSE, pad);

    return;
}


/* Create a label */

void pref_label_3(char *title, GtkWidget *grid, int *row)
{  
    GtkWidget *label;

    label = gtk_label_new(title);
    gtk_widget_set_name (label, "lbl_11");
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_END);
    gtk_widget_set_margin_start (label, 20);
    gtk_widget_set_margin_end (label, 10);
    gtk_grid_attach(GTK_GRID (grid), label, 0, *row, 1, 1);

    return;
}


/* Create radio buttons */

void pref_radio(char *nm, GtkWidget *grid, char active, int *row)
{  
    int i;
    char s[10];
    GtkWidget *radio, *radio_grp;
    const char *rads[] = { "Prefix", "Mid", "Suffix", "None" };
    const int rad_count = 4;

    for(i = 0; i < rad_count; i++)
    {
    	if (i == 0)
    	{
	    radio = gtk_radio_button_new_with_label (NULL, rads[i]);
	    radio_grp = radio;
	}
    	else
    	{
	    radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio_grp), rads[i]);
	}

	sprintf(s, "%s_%s", nm, rads[i]);
	gtk_widget_set_name(radio, s);

	if (active == rads[i][0])
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);

	gtk_grid_attach(GTK_GRID (grid), radio, i + 1, *row, 1, 1);
    }

    return;
}


/* Create boolean radio buttons */

void pref_boolean(char *s1, char *s2, int active, GtkWidget **h_box)
{  
    GtkWidget *radio_grp;
    GtkWidget *radio;

    radio_grp = gtk_radio_button_new_with_label (NULL, s1);
    radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio_grp), s2);
    gtk_widget_set_name(radio_grp, "rb_0");
    gtk_widget_set_name(radio, "rb_1");

    if (active == FALSE)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_grp), TRUE);
    else
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);

    gtk_widget_set_margin_start (radio_grp, 15);
    gtk_box_pack_start (GTK_BOX (*h_box), radio_grp, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (*h_box), radio, FALSE, FALSE, 0);

    return;
}


/* Set the signal handler for each radio button */

void set_fn_handler(PrefUi *p_ui)
{
    GtkWidget *radio;

    GList *child_widgets = gtk_container_get_children(GTK_CONTAINER (p_ui->fn_grid));

    child_widgets = g_list_first(child_widgets);

    while (child_widgets != NULL)
    {
	radio = child_widgets->data;

	if (GTK_IS_TOGGLE_BUTTON (radio))
	    g_signal_connect(radio, "toggled", G_CALLBACK(OnFnPref), (gpointer) p_ui);

	child_widgets = g_list_next(child_widgets);
    }

    g_list_free (child_widgets);

    return;
}


/* Derive the sample template filename from the preferences */

void set_fn_template(char id, char tt, char ts, char *tmpl, PrefUi *p_ui)
{
    int i;
    char s_pref[3][10];
    char s[10];

    /* Validate values */
    p_ui->fn_err = FALSE;

    if (validate_fn_prefs(id, tt, ts) == FALSE)
    {
    	p_ui->fn_err = TRUE;
    	strcpy(tmpl, "capture.type");
    	return;
    }

    /* Fields to empty */
    tmpl[0] = '\0';
    s_pref[0][0] = '\0';
    s_pref[1][0] = '\0';
    s_pref[2][0] = '\0';

    /* Match preference to position */
    i = set_fn_idx(id, 0, s_pref);
    i = set_fn_idx(ts, 2, s_pref);
    i = set_fn_idx(tt, 1, s_pref);

    /* There must be at least a default title in the first available slot */
    if (i == -1)
    {
    	for(i = 0; i < 3; i++)
    	{
	    if (s_pref[i][0] == '\0')
	    {
    		strcpy(s_pref[i], "capture");
    		break;
	    }
    	}
    }

    /* Build up a template */
    for(i = 0; i < 3; i++)
    {
    	if (s_pref[i][0] == '\0')
	    continue;

	if (tmpl[0] != '\0')
	    strcat(tmpl, "_");

	strcat(tmpl, s_pref[i]);
    }

    strcat(tmpl, ".type");

    return;
}


/* Determine position of item in template */

int set_fn_idx(char cc, int trx, char s_pref[][10])
{
    int idx;
    const char *desc[] = {"nn", "title", "timestamp"};

    switch(cc)
    {
	case 'P': idx = 0; break;
	case 'M': idx = 1; break;
	case 'S': idx = 2; break;
	default: idx = -1; break;
    }
     
    if (idx != -1)
    	strcpy(&s_pref[idx][0], desc[trx]);

    return idx;
}


/* Validate filename user preferences */

int validate_fn_prefs(char id, char tt, char ts)
{
    int idn, ttn, tsn;
    int i, val_all;
    const int valid_vals[] = { 0, 4, 16, 20, 64, 68, 80, 84 };

    idn = set_fn_val(id);
    ttn = set_fn_val(tt);
    tsn = set_fn_val(ts);

    val_all = idn + ttn + tsn;

    for(i = 0; i < 8; i++)
    {
    	if (val_all == valid_vals[i])
	    break;
    }

    if (i >= 8)
    	return FALSE;

    return TRUE;
}


/* Set a unique number combination for prefix, middle, suffix and none */

int set_fn_val(char cc)
{
    int val;

    switch(cc)
    {
	case 'P': val = 4; break;
	case 'M': val = 16; break;
	case 'S': val = 64; break;
	default: val = 0; break;
    }

    return val;
}


/* Set the template colour - red = error (capture.type) */

void set_tmpl_colour(GtkWidget *tmpl, char *s)
{
    if (strncmp(s, "capture.", 8) == 0)
	gtk_widget_set_name(tmpl, "lbl_3");
    else
	gtk_widget_set_name(tmpl, "lbl_4");

    return;
}


/* Set a preference based on a radio button name */

void set_fn_pref(char *id, char *tt, char *ts, const gchar *nm)
{
    // A bit quirky but the second character tells which preference 
    // and fourth character is the value (P, M, S, N)
    switch(nm[1])
    {
    	case 'd': *id = nm[3]; break; 		// id (Id)
    	case 't': *tt = nm[3]; break; 		// tt (Title)
    	case 's': *ts = nm[3]; break; 		// ts (Timestamp)
    	default: break;
    }

    return;
}


/* Construct a filename */

void get_file_name(char *fn, int fn_sz, 
		   char *id, char *title, char *tm_stmp, 
		   char idp, char ttp, char tsp)
{
    int len;
    char *s;

    /* Setup */
    *fn = '\0';
    len = strlen(title);
    
    if (len < 8)
    	len = 8;

    s = (char *) malloc(len + 1);

    if (*title == '\0')
	strcpy(s, "capture");
    else
	strcpy(s, title);

    /* Build name */
    file_name_item(fn, fn_sz, 'P', idp, ttp, tsp, id, s, tm_stmp);
    file_name_item(fn, fn_sz, 'M', idp, ttp, tsp, id, s, tm_stmp);
    file_name_item(fn, fn_sz, 'S', idp, ttp, tsp, id, s, tm_stmp);

    /* Fail safe in case all are set to 'N', otherwise remove lazy '_' */
    if (*fn == '\0')
    	strcpy(fn, s);
    else
    	fn[strlen(fn) - 1] = '\0';

    free(s);

    return;
}


/* Set a part of a file name */

void file_name_item(char *fn, int fn_sz, char cc, 
		    char idp, char ttp, char tsp,
		    char *id, char *title, char *tm_stmp) 
{
    char *dup_fn;

    dup_fn = strdup(fn);

    if (idp == cc)
    	snprintf(fn, fn_sz, "%s%s_", dup_fn, id);

    else if (ttp == cc)
    	snprintf(fn, fn_sz, "%s%s_", dup_fn, title);

    else if (tsp == cc)
    	snprintf(fn, fn_sz, "%s%s_", dup_fn, tm_stmp);

    free(dup_fn);

    return;
}


/* Set a field based on a radio button name */

char find_active_by_parent(GtkWidget *parent, char nm)
{
    GtkWidget *radio;
    const gchar *widget_name;

    GList *child_widgets = gtk_container_get_children(GTK_CONTAINER (parent));

    child_widgets = g_list_first(child_widgets);

    while (child_widgets != NULL)
    {
	radio = child_widgets->data;

	if (GTK_IS_TOGGLE_BUTTON (radio))
	{
	    widget_name = gtk_widget_get_name (radio);

	    if (widget_name[1] == nm)
	    {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio)) == TRUE)
		{
		    g_list_free (child_widgets);
		    return widget_name[3];
		}
	    }
	}

	child_widgets = g_list_next(child_widgets);
    }

    g_list_free (child_widgets);

    return '\0';
}


/* Read the user preferences file */

int read_user_prefs(GtkWidget *window)
{
    FILE *fd = NULL;
    struct stat fileStat;
    char buf[256];
    char *pref_fn;
    char *app_dir;
    char *p, *p2;
    int app_dir_len;
    int err;

    /* Initial */
    pref_count = 0;

    /* Get the full path for the preferences file */
    app_dir = app_dir_path();
    app_dir_len = strlen(app_dir);
    pref_fn = (char *) malloc(app_dir_len + 19);
    sprintf(pref_fn, "%s/%s", app_dir, USER_PREFS);

    /* If no preferences exist, create a default set */
    err = stat(pref_fn, &fileStat);

    if ((err < 0) || (fileStat.st_size == 0))
    {
	log_msg("SYS9015", NULL, NULL, NULL);
    	set_default_prefs();
	free(pref_fn);

	return TRUE;
    }
    
    /* Read existing user preferences */
    if ((fd = fopen(pref_fn, "r")) == (FILE *) NULL)
    {
	free(pref_fn);
	return FALSE;
    }
    
    /* Store the preferences */
    while ((fgets(buf, sizeof(buf), fd)) != NULL)
    {
	/* Check and save key */
	if ((p = strchr(buf, '|')) == NULL)
	{
	    free(pref_fn);
	    sprintf(app_msg_extra, "%s", buf);
	    log_msg("SYS9014", "Invalid user preference key format", "SYS9014", window);
	    return FALSE;
	}

	if ((p - buf) > (PREF_KEY_SZ - 1))
	{
	    free(pref_fn);
	    sprintf(app_msg_extra, "%s", buf);
	    log_msg("SYS9014", "Invalid user preference key size", "SYS9014", window);
	    return FALSE;
	}

	/* Check and save value */
	if ((p2 = strchr((p), '\n')) == NULL)
	{
	    free(pref_fn);
	    sprintf(app_msg_extra, "%s", buf);
	    log_msg("SYS9014", "Invalid user preference value", "SYS9014", window);
	    return FALSE;
	}

	/* Create a preference entry */
	UserPrefData *Preference = (UserPrefData *) malloc(sizeof(UserPrefData));
	memset(Preference, 0, sizeof (UserPrefData));
	strncpy(Preference->key, buf, p - buf);
	Preference->key[p - buf] = '\0';
	string_trim(Preference->key);

	p++;
	*p2 = '\0';

	Preference->val = (char *) malloc(strlen(p) + 1);
	strcpy(Preference->val, p);
	string_trim(Preference->val);
	    
	pref_list = g_list_prepend(pref_list, Preference);
	pref_count++;
    }

    /* Still may need set up some default preferences */
    pref_list_head = g_list_reverse(pref_list);
    set_default_prefs();

    /* Close off */
    fclose(fd);
    free(pref_fn);
    save_indi = FALSE;

    return TRUE;
}


/* Write the user preferences file */

int write_user_prefs(GtkWidget *window)
{
    FILE *fd = NULL;
    char buf[256];
    char *pref_fn;
    char *app_dir;
    int app_dir_len;

    /* Get the full path for the preferecnes file */
    app_dir = app_dir_path();
    app_dir_len = strlen(app_dir);
    pref_fn = (char *) malloc(app_dir_len + 19);
    sprintf(pref_fn, "%s/user_preferences", app_dir);

    /* New or overwrite file */
    if ((fd = fopen(pref_fn, "w")) == (FILE *) NULL)
    {
	free(pref_fn);
	return FALSE;
    }

    /* Write new values */
    pref_list = g_list_first(pref_list_head);

    while(pref_list != NULL)
    {
    	UserPrefData *Preference = (UserPrefData *) pref_list->data;

    	if (Preference->val != NULL)
    	{
	    sprintf(buf, "%s|%s\n", Preference->key, Preference->val);
	    
	    if ((fputs(buf, fd)) == EOF)
	    {
		free(pref_fn);
		log_msg("SYS9005", pref_fn, "SYS9005", window);
		return FALSE;
	    }
    	}

	pref_list = g_list_next(pref_list);
    }

    /* Close off */
    fclose(fd);
    free(pref_fn);
    save_indi = FALSE;

    return TRUE;
}


/* Set up default user preferences. All preferences may not be present */

void set_default_prefs()
{
    char *p;

    /* Snapshots defaults */
    get_user_pref(IMAGE_TYPE, &p);

    if (p == NULL)
	init_snapshot_prefs();

    /* Capture defaults */
    get_user_pref(CAPTURE_FORMAT, &p);

    if (p == NULL)
	init_capture_prefs();

    /* Capture defaults */
    get_user_pref(CAPTURE_LOCATION, &p);

    if (p == NULL)
	init_dir_prefs();

    /* File name defaults */
    get_user_pref(FN_ID, &p);

    if (p == NULL)
	init_fn_prefs();

    /* Preset Profile defaults */
    get_user_pref(DEFAULT_PROFILE, &p);

    if (p == NULL)
	init_profile_prefs();

    /* Audio mute default */
    get_user_pref(AUDIO_MUTE, &p);

    if (p == NULL)
	init_audio_prefs();

    /* Empty Title warning default */
    get_user_pref(WARN_EMPTY_TITLE, &p);

    if (p == NULL)
	init_title_prefs();

    /* Meta Data */
    get_user_pref(META_DATA, &p);

    if (p == NULL)
	init_metadata_prefs();

    /* Initial codec property defaults */
    init_codec_prop_prefs();

    /* Save to file */
    write_user_prefs(NULL);

    return;
}


/* Default snapshot preferences - jpeg type, 70% quality */

void init_snapshot_prefs()
{
    add_user_pref(IMAGE_TYPE, "jpg");
    add_user_pref(JPEG_QUALITY, "70");
    add_user_pref(SNAPSHOT_DELAY, "1");
    add_user_pref(FITS_BITS, "32");

    return;
}


/* Default capture preferences - YUY2 avi capture, 90 seconds duration, Mpeg2 framerate 25 */

void init_capture_prefs()
{
    add_user_pref(CAPTURE_FORMAT, "YUY2");
    add_user_pref(CAPTURE_DURATION, "90");
    add_user_pref(MPG2_FRAMERATE, "25");

    return;
}


/* Default directory preferences  - $HOME/AstroCTC */

void init_dir_prefs()
{
    char *home_str, *val;
    int len;

    home_str = home_dir();
    len = strlen(home_str) + strlen(TITLE) + 11;
    val = (char *) malloc(len);
    sprintf(val, "%s/%s_Captures", home_str, TITLE);

    add_user_pref(CAPTURE_LOCATION, val);

    if (! check_dir(val))
	make_dir(val);

    free(val);

    return;
}


/* Default file name (template) preferences */

void init_fn_prefs()
{
    add_user_pref(FN_ID, "P");
    add_user_pref(FN_TITLE, "M");
    add_user_pref(FN_TIMESTAMP, "S");

    return;
}


/* Default preset profile - Last session */

void init_profile_prefs()
{
    add_user_pref(DEFAULT_PROFILE, LAST_SESSION);

    return;
}


/* Default audio mute - on (silence) */

void init_audio_prefs()
{
    add_user_pref(AUDIO_MUTE, "1");

    return;
}


/* Default empty Title warning - on */

void init_title_prefs()
{
    add_user_pref(WARN_EMPTY_TITLE, "1");

    return;
}


/* Default Meta Data action - on */

void init_metadata_prefs()
{
    add_user_pref(META_DATA, "1");

    return;
}


/* Update all user preferences */

void set_user_prefs(PrefUi *p_ui)
{
    char cc;
    char s[3];
    int idx;
    codec_t *p_codec;
    const gchar *img_type;
    const gchar *jpg_qual;
    const gchar *fits_bits;
    const gchar *snap_delay;
    const gchar *capt_duration;
    const gchar *capt_dir;

    /* Initial */
    s[1] = '\0';

    /* Image type */
    img_type = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (p_ui->cbox_fmt));
    set_user_pref(IMAGE_TYPE, (char *) img_type);

    /* Jpeg quality */
    jpg_qual = gtk_entry_get_text(GTK_ENTRY (p_ui->jpeg_qual));
    set_user_pref(JPEG_QUALITY, (char *) jpg_qual);

    /* Sanpshot delay */
    snap_delay = gtk_entry_get_text(GTK_ENTRY (p_ui->snap_delay));
    set_user_pref(SNAPSHOT_DELAY, (char *) snap_delay);

    /* Video format */
    idx = gtk_combo_box_get_active (GTK_COMBO_BOX (p_ui->cbox_codec));
    p_codec = get_codec_idx(idx);
    set_user_pref(CAPTURE_FORMAT, p_codec->fourcc);

    /* Capture duration */
    capt_duration = gtk_entry_get_text(GTK_ENTRY (p_ui->capt_duration));
    set_user_pref(CAPTURE_DURATION, (char *) capt_duration);

    /* File name - ID */
    cc = find_active_by_parent(p_ui->fn_grid, 'd');
    s[0] = cc;
    set_user_pref(FN_ID, s);

    /* File name - Title */
    cc = find_active_by_parent(p_ui->fn_grid, 't');
    s[0] = cc;
    set_user_pref(FN_TITLE, s);

    /* File name - Timestamp */
    cc = find_active_by_parent(p_ui->fn_grid, 's');
    s[0] = cc;
    set_user_pref(FN_TIMESTAMP, s);

    /* File location */
    capt_dir = gtk_entry_get_text(GTK_ENTRY (p_ui->capt_dir));
    set_user_pref(CAPTURE_LOCATION, (char *) capt_dir);

    /* Fits bits */
    fits_bits = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (p_ui->cbox_fits_bits));
    strncpy(s, fits_bits, 2);
    s[2] = '\0';
    set_user_pref(FITS_BITS, s);

    /* Audio Mute */
    cc = find_active_by_parent(p_ui->audio_hbox, 'b');
    s[0] = cc;
    s[1] = '\0';
    set_user_pref(AUDIO_MUTE, s);

    /* Empty Title warning */
    cc = find_active_by_parent(p_ui->title_hbox, 'b');
    s[0] = cc;
    s[1] = '\0';
    set_user_pref(WARN_EMPTY_TITLE, s);

    /* Meta Data */
    cc = find_active_by_parent(p_ui->meta_hbox, 'b');
    s[0] = cc;
    s[1] = '\0';
    set_user_pref(META_DATA, s);

    return;
}


/* Add a user preference */

int add_user_pref(char *key, char *val)
{
    UserPrefData *Preference;

    Preference = (UserPrefData *) malloc(sizeof(UserPrefData));
    strcpy(Preference->key, key);
    Preference->val = (char *) malloc(strlen(val) + 1);
    strcpy(Preference->val, val);

    pref_list = g_list_append(pref_list_head, (gpointer) Preference);
    pref_count++;

    if (pref_list_head == NULL)
    	pref_list_head = pref_list;

    return TRUE;
}


/* Add a user preference at a given reference */

int add_user_pref_idx(char *key, char *val, int idx)
{
    UserPrefData *Preference;

    Preference = (UserPrefData *) malloc(sizeof(UserPrefData));
    strcpy(Preference->key, key);
    Preference->val = (char *) malloc(strlen(val) + 1);
    strcpy(Preference->val, val);

    pref_list_head = g_list_insert(pref_list_head, (gpointer) Preference, idx);
    pref_count++;

    return TRUE;
}


/* Update a user preference */

int set_user_pref(char *key, char *val)
{
    int i;
    UserPrefData *Preference;

    /* Find the key entry and set the new value */
    pref_list = g_list_first(pref_list_head);
    i = 0;

    while(pref_list != NULL)
    {
    	Preference = (UserPrefData *) pref_list->data;

    	if (strcmp(Preference->key, key) == 0)
    	{
	    i = strlen(val);

	    if (i > strlen(Preference->val))
		Preference->val = realloc(Preference->val, i + 1);

	    strcpy(Preference->val, val);
	    break;
    	}

	pref_list = g_list_next(pref_list);
    }

    if (i == 0)
    	return FALSE;

    return TRUE;
}


/* Return a pointer to a user preference value for a key or NULL */

int get_user_pref(char *key, char **val)
{
    int i;
    UserPrefData *Preference;

    *val = NULL;
    i = 0;

    pref_list = g_list_first(pref_list_head);

    while(pref_list != NULL)
    {
    	Preference = (UserPrefData *) pref_list->data;

    	if (strcmp(Preference->key, key) == 0)
    	{
	    *val = Preference->val;
	    break;
    	}

	pref_list = g_list_next(pref_list);
	i++;
    }

    return i;
}


/* Return a pointer to a user preference value for a key and index or NULL */

void get_user_pref_idx(int idx, char *key, char **val)
{
    UserPrefData *Preference;

    *val = NULL;

    pref_list = g_list_nth(pref_list_head, idx);
    Preference = (UserPrefData *) pref_list->data;

    if (strcmp(Preference->key, key) == 0)
	*val = Preference->val;

    return;
}


/* Return the key at an index */

void get_pref_key(int idx, char *key)
{
    UserPrefData *Preference;

    pref_list = g_list_nth(pref_list_head, idx);
    Preference = (UserPrefData *) pref_list->data;

    if (pref_list == NULL)
    	key = NULL;
    else
    	strcpy(key, Preference->key);

    return;
}


/* Return a pointer to the value at an index */

void get_pref_val(int idx, char **val)
{
    UserPrefData *Preference;

    pref_list = g_list_nth(pref_list_head, idx);
    Preference = (UserPrefData *) pref_list->data;

    if (pref_list == NULL)
    	*val = NULL;
    else
	*val = Preference->val;

    return;
}


// Return a pointer to a user preference value for a key or NULL
// String match the key and part value

int match_key_val_combo(char *key, char *s_val, int s_len, char **val)
{
    int i, k_len;
    UserPrefData *Preference;

    i = 0;
    *val = NULL;
    k_len = strlen(key);

    pref_list = g_list_first(pref_list_head);

    while(pref_list != NULL)
    {
    	Preference = (UserPrefData *) pref_list->data;

    	if ((strncmp(Preference->key, key, k_len) == 0) &&
	    (strncmp(Preference->val, s_val, s_len) == 0))
    	{
	    *val = Preference->val;
	    break;
    	}

	pref_list = g_list_next(pref_list);
	i++;
    }

    return i;
}


/* Delete a preference setting */

void delete_user_pref(char *key)
{
    UserPrefData *Preference;
    GList *llink = NULL;

    llink = g_list_first(pref_list_head);

    while(llink != NULL)
    {
    	Preference = (UserPrefData *) llink->data;

    	if (strcmp(Preference->key, key) == 0)
    	{
	    pref_list_head = g_list_remove_link(pref_list_head, llink);
	    free(Preference->val);
	    free(Preference);
	    g_list_free(llink);
	    break;
    	}

	llink = g_list_next(llink);
    }

    return;
}


/* Check if changes have been made */

int pref_save_reqd(PrefUi *p_ui)
{
    gint res;
    char cc;
    char s[3];
    int idx;
    codec_t *p_codec;
    const gchar *img_type;
    const gchar *jpg_qual;
    const gchar *fits_bits;
    const gchar *snap_delay;
    const gchar *capt_duration;
    const gchar *capt_dir;

    /* Initial */
    s[1] = '\0';

    /* File location */
    capt_dir = gtk_entry_get_text(GTK_ENTRY (p_ui->capt_dir));

    /* Capture location exists */
    if (! check_dir((char *) capt_dir))
    {
	res = query_dialog(p_ui->window, "Location (%s) does not exist. Create it?", (char *) capt_dir);

	if (res == GTK_RESPONSE_YES)
	    make_dir((char *) capt_dir);
    }

    if (pref_changed(CAPTURE_LOCATION, (char *) capt_dir))
    	return TRUE;

    /* Image type */
    img_type = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (p_ui->cbox_fmt));

    if (pref_changed(IMAGE_TYPE, (char *) img_type))
    	return TRUE;

    /* Jpeg quality */
    jpg_qual = gtk_entry_get_text(GTK_ENTRY (p_ui->jpeg_qual));

    if (pref_changed(JPEG_QUALITY, (char *) jpg_qual))
    	return TRUE;

    /* Sanpshot delay */
    snap_delay = gtk_entry_get_text(GTK_ENTRY (p_ui->snap_delay));

    if (pref_changed(SNAPSHOT_DELAY, (char *) snap_delay))
    	return TRUE;

    /* Codec format */
    idx = gtk_combo_box_get_active (GTK_COMBO_BOX (p_ui->cbox_codec));
    p_codec = get_codec_idx(idx);

    if (pref_changed(CAPTURE_FORMAT, p_codec->fourcc))
    	return TRUE;

    /* Capture duration */
    capt_duration = gtk_entry_get_text (GTK_ENTRY (p_ui->capt_duration));

    if (pref_changed(CAPTURE_DURATION, (char *) capt_duration))
    	return TRUE;

    /* File name - ID */
    cc = find_active_by_parent(p_ui->fn_grid, 'd');
    s[0] = cc;
    
    if (pref_changed(FN_ID, s))
    	return TRUE;

    /* File name - Title */
    cc = find_active_by_parent(p_ui->fn_grid, 't');
    s[0] = cc;
    
    if (pref_changed(FN_TITLE, s))
    	return TRUE;

    /* File name - Timestamp */
    cc = find_active_by_parent(p_ui->fn_grid, 's');
    s[0] = cc;
    
    if (pref_changed(FN_TIMESTAMP, s))
    	return TRUE;

    /* Fits bits */
    fits_bits = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (p_ui->cbox_fits_bits));
    strncpy(s, fits_bits, 2);
    s[2] = '\0';

    if (pref_changed(FITS_BITS, s))
    	return TRUE;

    /* Audio */
    cc = find_active_by_parent(p_ui->audio_hbox, 'b');
    s[0] = cc;
    s[1] = '\0';
    
    if (pref_changed(AUDIO_MUTE, s))
    	return TRUE;

    /* Empty Title */
    cc = find_active_by_parent(p_ui->title_hbox, 'b');
    s[0] = cc;
    s[1] = '\0';
    
    if (pref_changed(WARN_EMPTY_TITLE, s))
    	return TRUE;

    /* Meta Data */
    cc = find_active_by_parent(p_ui->meta_hbox, 'b');
    s[0] = cc;
    s[1] = '\0';
    
    if (pref_changed(META_DATA, s))
    	return TRUE;

    return FALSE;
}


/* Check if changes have been made */

int pref_changed(char *key, char *val)
{
    char *p;

    get_user_pref(key, &p);

    if (strcmp(p, val) != 0)
    	return TRUE;

    return FALSE;
}


/* Validate screen contents */

int validate_pref(PrefUi *p_ui)
{
    const gchar *s;
    int i;

    /* Jpeg quality must be numeric */
    s = gtk_entry_get_text (GTK_ENTRY (p_ui->jpeg_qual));

    if (val_str2numb((char *) s, &i, "Quality", p_ui->window) == FALSE)
	return FALSE;

    /* Delay must be numeric */
    s = gtk_entry_get_text (GTK_ENTRY (p_ui->snap_delay));

    if (val_str2numb((char *) s, &i, "Delay", p_ui->window) == FALSE)
	return FALSE;

    /* Duration must be numeric */
    s = gtk_entry_get_text (GTK_ENTRY (p_ui->capt_duration));

    if (val_str2numb((char *) s, &i, "Duration", p_ui->window) == FALSE)
	return FALSE;

    return TRUE;
}


/* Free the user preferences */

void free_prefs()
{
    UserPrefData *Preference;

    pref_list = g_list_first(pref_list_head);

    while(pref_list != NULL)
    {
    	Preference = (UserPrefData *) pref_list->data;
    	free(Preference->val);
    	free(Preference);

	pref_list = g_list_next(pref_list);
    }

    g_list_free(pref_list_head);

    return;
}


/* Callback - Set snapshot image type */

void OnSetFmt(GtkWidget *cbox, gpointer user_data)
{  
    int init_pad, alloc, alloc2, all_pad;
    gchar *img_type;
    PrefUi *p_ui;

    /* Get data */
    p_ui = (PrefUi *) user_data;

    /* None */
    if (gtk_combo_box_get_active (GTK_COMBO_BOX (cbox)) == -1)
    	return;

    /* Enable 'Quality' if Jpeg selected, adjust other widget sizes as required (YUK!) */
    img_type = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cbox));

    if (strcmp(img_type, "jpg") == 0)
    {
	gtk_widget_show (p_ui->jqual_cntr);
	init_pad = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (p_ui->snap_cntr), "init_pad"));
	gtk_widget_hide (p_ui->cbox_fits_bits);
	gtk_widget_set_margin_end (p_ui->snap_cntr, init_pad);
    }
    else if (strcmp(img_type, "fits") == 0)
    {
	alloc = gtk_widget_get_allocated_width (p_ui->jqual_cntr);
	alloc2 = gtk_widget_get_allocated_width (p_ui->cbox_fits_bits);
	gtk_widget_show (p_ui->cbox_fits_bits);
	gtk_widget_hide (p_ui->jqual_cntr);
	gtk_widget_set_margin_end (p_ui->snap_cntr, (alloc - alloc2));
    }
    else
    {
	alloc = gtk_widget_get_allocated_width (p_ui->jqual_cntr);
	all_pad = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (p_ui->jqual_cntr), "all_pad"));
	gtk_widget_hide (p_ui->jqual_cntr);
	gtk_widget_hide (p_ui->cbox_fits_bits);
	gtk_widget_set_margin_end (p_ui->snap_cntr, alloc + all_pad);
    }

    return;
}


/* Callback - Toggle a file name option */

void OnFnPref(GtkWidget *radio, gpointer user_data)
{  
    char id, tt, ts;
    char s[30];
    const gchar *nm;
    GtkWidget *grid;
    PrefUi *p_ui;
    GtkWidget *fn_tmpl;

    /* Determine which toggle is active */
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio)) == FALSE)
    	return;

    /* Initial */
    id = '\0';
    tt = '\0';
    ts = '\0';
    p_ui = (PrefUi *) user_data;
    grid = p_ui->fn_grid;

    nm = gtk_widget_get_name (radio);
    set_fn_pref(&id, &tt, &ts, nm);

    /* Get active value for the other preferences */
    if (id == '\0')
    	id = find_active_by_parent(grid, 'd');
    	
    if (tt == '\0')
    	tt = find_active_by_parent(grid, 't');
    	
    if (ts == '\0')
    	ts = find_active_by_parent(grid, 's');

    /* Show new template */
    set_fn_template(id, tt, ts, s, p_ui);
    gtk_label_set_text (GTK_LABEL (p_ui->fn_tmpl), s);
    set_tmpl_colour(p_ui->fn_tmpl, s);

    return;
}


/* Callback - Set capture directory */

void OnPrefBrowse(GtkWidget *browse_btn, gpointer user_data)
{  
    GtkWidget *dialog;
    PrefUi *p_ui;
    gchar *dir_name;
    gint res;

    /* Get data */
    p_ui = (PrefUi *) user_data;

    /* Selection */
    dialog = gtk_file_chooser_dialog_new ("Capture Location",
					  GTK_WINDOW (p_ui->window),
					  GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
					  "_Cancel", GTK_RESPONSE_CANCEL,
					  "_Apply", GTK_RESPONSE_APPLY,
					  NULL);

    res = gtk_dialog_run (GTK_DIALOG (dialog));

    if (res == GTK_RESPONSE_APPLY)
    {
	GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
	dir_name = gtk_file_chooser_get_filename (chooser);

	if (dir_name)
	{
	    gtk_entry_set_text (GTK_ENTRY (p_ui->capt_dir), dir_name);

	    if (! check_dir(dir_name))
	    {
		res = query_dialog(p_ui->window, "Location (%s) does not exist. Create it?", dir_name);

		if (res == GTK_RESPONSE_YES)
		    make_dir(dir_name);
	    }
	}

	g_free (dir_name);
    }

    gtk_widget_destroy (dialog);

    return;
}


/* Callback for properties window */

void OnCodecProp(GtkWidget *btn, gpointer user_data)
{
    PrefUi *ui;
    int idx;

    /* Get data */
    ui = (PrefUi *) user_data;
    idx = gtk_combo_box_get_active (GTK_COMBO_BOX (ui->cbox_codec));

    /* Check if already open */
    if (is_ui_reg(CODEC_UI, TRUE))
    	close_ui(CODEC_UI);

    /* Open */
    codec_ui_main(idx, ui->window);

    return;
}


/* Callback for apply changes and close */

void OnPrefSave(GtkWidget *btn, gpointer user_data)
{
    PrefUi *ui;

    /* Get data */
    ui = (PrefUi *) user_data;

    /* Check for changes */
    if ((save_indi = pref_save_reqd(ui)) == FALSE)
    {
    	info_dialog(ui->window, "There are no changes to save!", "");
    	return;
    }

    /* Error check */
    if (validate_pref(ui) == FALSE)
    	return;

    if (ui->fn_err == TRUE)
    {
	log_msg("APP0001", NULL, "APP0001", ui->window);
    	return;
    }

    /* Store preferences */
    set_user_prefs(ui);

    /* Save to file */
    write_user_prefs(ui->window);

    return;
}


// Callback for window close
// Destroy the window and de-register the window 
// Check for changes

void OnPrefClose(GtkWidget *window, gpointer user_data)
{ 
    GtkWidget *dialog;
    PrefUi *ui;
    gint response;

    /* Get data */
    ui = (PrefUi *) g_object_get_data (G_OBJECT (window), "ui");

    /* Check for changes */
    if ((save_indi = pref_save_reqd(ui)) == TRUE)
    {
	/* Ask if OK to close without saving */
	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_OK_CANCEL,
					 "Close without saving changes?");

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (response == GTK_RESPONSE_CANCEL)
	    return;
    }

    /* Close the window, free the screen data and block any secondary close signal */
    g_signal_handler_block (window, ui->close_handler);

    deregister_window(window);
    gtk_window_close(GTK_WINDOW(window));

    free(ui);

    return;
}


/* Window delete event */

gboolean OnPrefDelete(GtkWidget *window, GdkEvent *ev, gpointer user_data)
{
    OnPrefClose(window, user_data);

    return TRUE;
}
