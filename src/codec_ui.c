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
** Description: Codec details and management.
**
** Author:	Anthony Buckley
**
** History
**	20-Oct-2014	Initial code
**
*/


/* Includes */

#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <codec.h>
#include <defs.h>
#include <preferences.h>


/* Defines */

#define MAX_CODEC 25


/* Types */
typedef struct _codec_ui
{
    GtkWidget *window;
    GtkWidget *fixed_cntr;
    GtkWidget *codec_cntr;
    GtkWidget *l_fcc, *l_short_desc, *l_long_desc, *l_extn, *l_enc, *l_mux;
    GtkWidget *cbox_mpg2;
    GtkWidget *tree;
    GtkTreeModel *model;
    int close_handler;
    int row;
    char mpg2_init[5];
    codec_t *codec;
} CodecUi;


/* Prototypes */

int codec_ui_main(int, GtkWidget *);
int codec_prop_init(int, GtkWidget *);
CodecUi * new_prop_ui();
void fixed_details(CodecUi *);
void codec_extra(CodecUi *);
void mpeg2_extra(CodecUi *);
void codec_table(CodecUi *);
void codec_prop_ui(CodecUi *);
void add_to_grid(char *, GtkWidget *, char *, int *, GtkWidget *);
int codec_save_reqd(CodecUi *);
int extra_save_reqd(CodecUi *);
void extra_save_undo(CodecUi *);
void set_enc_defaults(CodecUi *);
char * enc_property_total(char *, int *, int *, CodecUi *);
codec_t * get_codec_arr(int *);
char * get_codec_extn(char *);
codec_t * get_codec(char *);
codec_t * get_codec_idx(int);
int codec_property_type(char *, char *);
void init_codec_prop_prefs();
void init_codec_pref(char *, char *, int);
void init_codec_max(char *, int);
void check_user_default(char *, char *, char **);
void OnCodecClose(GtkWidget*, gpointer);
gboolean OnCodecDelete(GtkWidget*, GdkEvent *, gpointer);
void OnCodecSave(GtkWidget*, gpointer);
void OnPropertyEdit(GtkCellRendererText *, gchar *, gchar *, gpointer);
void OnSetMpeg2(GtkWidget*, gpointer);

extern void log_msg(char*, char*, char*, GtkWidget*);
extern int add_user_pref(char *, char *);
extern int add_user_pref_idx(char *, char *, int);
extern void delete_user_pref(char *);
extern int get_user_pref(char *, char **);
extern void get_user_pref_idx(int, char *, char **);
extern void get_pref_key(int, char *);
extern int set_user_pref(char *, char *);
extern int match_key_val_combo(char *, char *, int, char **);
extern int write_user_prefs(GtkWidget *);
extern void info_dialog(GtkWidget *, char *, char *);
extern void register_window(GtkWidget *);
extern void deregister_window(GtkWidget *);


/* Globals */

static const char *debug_hdr = "DEBUG-codec_ui.c ";

enum
    {
       PROPERTY_NM,
       DEFAULT_VALUE,
       PREF_VALUE,
       TOOL_TIP,
       N_COLUMNS
    };

static codec_t codec_arr[] =		// The codecs that are supported.
{
    { 
    	.fourcc = "YUY2",
    	.short_desc = "YUY2",
    	.long_desc = "Uncompressed YUV 4:2:2 (aka YUYV)",
    	.extn = "avi", 
    	.encoder = "",
    	.muxer = "avimux"
    },
    { 
    	.fourcc = "I420",
    	.short_desc = "I420",
    	.long_desc = "Uncompressed 4:2:0 (aka YU12)",
    	.extn = "avi",
    	.encoder = "",
    	.muxer = "avimux"
    },
    {
    	.fourcc = "BGR",
    	.short_desc = "BGR",
    	.long_desc = "Uncompressed reverse 24 bit RGB",
    	.extn = "avi",
    	.encoder = "",
    	.muxer = "avimux"
    },
    {
    	.fourcc = "MPG2",
    	.short_desc = "MPEG2",
    	.long_desc = "Mpeg 2",
    	.extn = "mpg",
    	.encoder = "avenc_mpeg2video",
    	.muxer = "mpegtsmux"			// "mpegpsmux" ?
    },
    {
    	.fourcc = "MPG4",
    	.short_desc = "MPEG4",
    	.long_desc = "Mpeg 4",
    	.extn = "mp4",
    	.encoder = "avenc_mpeg4",
    	.muxer = "mpegtsmux"
    },
    {
    	.fourcc = "H264",
    	.short_desc = "H264",
    	.long_desc = "H264 Matroska",
    	.extn = "mkv",
    	.encoder = "x264enc",
    	.muxer = "matroskamux"
    },
    {
    	.fourcc = "THEO",
    	.short_desc = "Theora",
    	.long_desc = "Ogg Theora",
    	.extn = "ogg",
    	.encoder = "theoraenc",
    	.muxer = "oggmux"
    }
};

static const int codec_max = 7;

static encoder_t encoder_arr[] =	// Default values for a range of supported encoder properties
{
    { .encoder = "mpegn", .property_nm = "bitrate", 	  	   .default_val = "300000", .type = 0,
      .tooltip = "0 - 2147483647" },
    { .encoder = "mpegn", .property_nm = "gop-size", 	  	   .default_val = "15",     .type = 0,
      .tooltip = "Range: 0 - 2147483647" },
    { .encoder = "mpegn", .property_nm = "me-method", 	  	   .default_val = "5",      .type = 0,
      .tooltip = "(1): zero, (2): full, (3): logarithmic, (4): phods, (5): epzs, (6): x1" },
    { .encoder = "mpegn", .property_nm = "buffer_size", 	   .default_val = "524288", .type = 0,
      .tooltip = "Range: 0 - 2147483647" },
    { .encoder = "mpegn", .property_nm = "rtp-payload-size", 	   .default_val = "0",      .type = 0,
      .tooltip = "Range: 0 - 2147483647" },
    { .encoder = "mpegn", .property_nm = "compliance", 	  	   .default_val = "0",      .type = 0,
      .tooltip = "(2): verystrict, (1): strict, (0): normal, (-1): unofficial, (-2): experimental" },
    { .encoder = "mpegn", .property_nm = "pass", 	  	   .default_val = "0",      .type = 0,
      .tooltip = "(0): cbr, (2): quant, (512): pass1, (1024): pass2" },
    { .encoder = "mpegn", .property_nm = "quantizer", 	  	   .default_val = "0.01",   .type = 1,
      .tooltip = "Float. Range: 0 - 30" },
    { .encoder = "mpegn", .property_nm = "quant-type", 	  	   .default_val = "0",      .type = 0,
      .tooltip = "(0): h263, (1): mpeg" },
    { .encoder = "mpegn", .property_nm = "qmin", 	  	   .default_val = "2",      .type = 0,
      .tooltip = "Range: 1 - 31" },
    { .encoder = "mpegn", .property_nm = "qmax", 	  	   .default_val = "31",     .type = 0,
      .tooltip = "Range: 1 - 31" },
    { .encoder = "mpegn", .property_nm = "max-qdiff", 	  	   .default_val = "3",      .type = 0,
      .tooltip = "Range: 1 - 31" },
    { .encoder = "mpegn", .property_nm = "qcompress", 	  	   .default_val = "0.5",    .type = 1,
      .tooltip = "Float. Range: 0 - 1" },
    { .encoder = "mpegn", .property_nm = "qblur", 	  	   .default_val = "0.5",    .type = 1,
      .tooltip = "Float. Range: 0 - 1" },
    { .encoder = "mpegn", .property_nm = "rc-qsquish", 	  	   .default_val = "1",      .type = 1,
      .tooltip = "Float. Range: 0 - 99" },
    { .encoder = "mpegn", .property_nm = "rc-qmod-amp", 	   .default_val = "0",      .type = 1,
      .tooltip = "Float. Range: 0 - 99" },
    { .encoder = "mpegn", .property_nm = "rc-qmod-freq", 	   .default_val = "0",      .type = 0,
      .tooltip = "Integer. Range: 0 - 0" },
    { .encoder = "mpegn", .property_nm = "rc-buffer-size", 	   .default_val = "0",      .type = 0,
      .tooltip = "Range: 0 - 2147483647" },
    { .encoder = "mpegn", .property_nm = "rc-buffer-aggressivity", .default_val = "1",      .type = 1,
      .tooltip = "Float. Range: 0 - 99" },
    { .encoder = "mpegn", .property_nm = "rc-max-rate", 	   .default_val = "0",      .type = 0,
      .tooltip = "Range: 0 - 2147483647" },
    { .encoder = "mpegn", .property_nm = "rc-min-rate", 	   .default_val = "0",      .type = 0,
      .tooltip = "Range: 0 - 2147483647" },
    { .encoder = "mpegn", .property_nm = "noise-reduction", 	   .default_val = "0",      .type = 0,
      .tooltip = "Range: 0 - 1000000" },
    { .encoder = "mpegn", .property_nm = "interlaced", 	   	   .default_val = "FALSE",  .type = 2,
      .tooltip = "Boolean. TRUE, FALSE" },
    { .encoder = "mpegn", .property_nm = "trellis", 	  	   .default_val = "1", 	    .type = 0,
      .tooltip = "Range: 0 - 1" },

    { .encoder = "x264enc", .property_nm = "threads", 	      .default_val = "0", 	   .type = 0,
      .tooltip = "Range: 0 - 4" },
    { .encoder = "x264enc", .property_nm = "sliced-threads",  .default_val = "FALSE", 	   .type = 2,
      .tooltip = "Boolean. TRUE, FALSE" },
    { .encoder = "x264enc", .property_nm = "sync-lookahead",  .default_val = "-1", 	   .type = 0,
      .tooltip = "Range: -1 - 250" },
    { .encoder = "x264enc", .property_nm = "pass", 	      .default_val = "0", 	   .type = 0,
      .tooltip = "(0): cbr, (4): quant, (5): qual, (17): pass1, (18): pass2, (19): pass3" },
    { .encoder = "x264enc", .property_nm = "quantizer",       .default_val = "21", 	   .type = 0,
      .tooltip = "Range: 0 - 50" },
    { .encoder = "x264enc", .property_nm = "bitrate", 	      .default_val = "2048", 	   .type = 0,
      .tooltip = "Range: 1 - 102400" },
    { .encoder = "x264enc", .property_nm = "noise-reduction", .default_val = "0", 	   .type = 0,
      .tooltip = "Range: 0 - 100000" },
    { .encoder = "x264enc", .property_nm = "interlaced",      .default_val = "FALSE", 	   .type = 2,
      .tooltip = "Boolean. TRUE, FALSE" },
    { .encoder = "x264enc", .property_nm = "speed-preset",    .default_val = "6", 	   .type = 0,
      .tooltip = "(0): None, (1): ultrafast, (2): superfast, (3): veryfast, (4): faster,"
                 "(5): fast, (6): medium, (7): slow, (8): slower, (9): veryslow, (10): placebo" },
    { .encoder = "x264enc", .property_nm = "psy-tune", 	      .default_val = "0", 	   .type = 0,
      .tooltip = "(0): none, (1): film, (2): animation, (3): grain, (4): psnr, (5): ssim" },
    { .encoder = "x264enc", .property_nm = "tune", 	      .default_val = "", 	   .type = 3,
      .tooltip = "(0x00000001): stillimage, (0x00000002): fastdecode, (0x00000004): zerolatency" },

    { .encoder = "theoraenc", .property_nm = "bitrate",        .default_val = "0", 	   .type = 0,
      .tooltip = "Range: 0 - 16777215" },
    { .encoder = "theoraenc", .property_nm = "quality",        .default_val = "48", 	   .type = 0,
      .tooltip = "Range: 0 - 63" },
    { .encoder = "theoraenc", .property_nm = "speed-level",    .default_val = "1", 	   .type = 0,
      .tooltip = "Range: 0 - 3" },
    { .encoder = "theoraenc", .property_nm = "drop-frames",    .default_val = "TRUE", 	   .type = 2,
      .tooltip = "Boolean. TRUE, FALSE" },
    { .encoder = "theoraenc", .property_nm = "rate-buffer",    .default_val = "0", 	   .type = 0,
      .tooltip = "Range: 0 - 1000" },
    { .encoder = "theoraenc", .property_nm = "multipass-mode", .default_val = "0", 	   .type = 0,
      .tooltip = "(0): single-pass, (1): first-pass, (2): second-pass" }
};

static const int encoder_max = 40;

static const char *codec_def[][2] =	// Initial settings for some codec properties
    {
    	{"x264enc", "speed-preset=4" },			// (4)faster
    	{"x264enc", "tune=zerolatency" },		
    	{"avenc_mpeg2video", "bitrate=3000000" },
    	{"avenc_mpeg4", "bitrate=3000000" }
    };

static const int init_pref_max = 3;

static const char *mpg2_fps_arr[][2] =	// Possible framerate values for mpeg2 codec
    {
    	{"PAL (25)", "25" },
    	{"NTSC (30)", "30" },
    	{"ATSC (60)", "60" }
    };

static const int mpg2_max = 3;



/* Display codec properties and save user settings */

int codec_ui_main(int idx, GtkWidget *window)
{
    CodecUi *ui;

    /* Initial */
    if (! codec_prop_init(idx, window))
    	return FALSE;

    ui = new_prop_ui();
    ui->codec = &(codec_arr[idx]);

    /* Create the interface */
    codec_prop_ui(ui);
    gtk_widget_show_all(ui->window);

    /* Register the window */
    register_window(ui->window);

    return TRUE;
}


/* Initial checks and values */

int codec_prop_init(int idx, GtkWidget *window)
{
    char s[30];

    /* Ensure index is valid */
    if (idx < 0 || idx >= codec_max)
    {
    	sprintf(s, "Codec for index %d", idx);
	log_msg("CAM0008", s, "CAM0008", window);
    	return FALSE;
    }

    return TRUE;
}


/* Create new screen data variable */

CodecUi * new_prop_ui()
{
    CodecUi *ui = (CodecUi *) malloc(sizeof(CodecUi));
    memset(ui, 0, sizeof(CodecUi));

    return ui;
}


/* Create the user interface and set the CallBacks */

void codec_prop_ui(CodecUi *p_ui)
{
    GtkWidget *save_btn, *close_btn;
    GtkWidget *main_vbox, *btn_box;

    /* Set up the UI window */
    p_ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(p_ui->window), CODEC_UI);
    gtk_window_set_position(GTK_WINDOW(p_ui->window), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_container_set_border_width(GTK_CONTAINER(p_ui->window), 10);
    g_object_set_data (G_OBJECT (p_ui->window), "ui", p_ui);

    /* Main view */
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    /* Main update or view grid */
    fixed_details(p_ui);
    codec_extra(p_ui);
    codec_table(p_ui);

    /* Box container for action buttons */
    btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(GTK_WIDGET (btn_box), GTK_ALIGN_CENTER);

    /* Close button */
    close_btn = gtk_button_new_with_label("  Close  ");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(OnCodecClose), p_ui->window);
    gtk_box_pack_end (GTK_BOX (btn_box), close_btn, FALSE, FALSE, 0);

    /* Save button */
    save_btn = gtk_button_new_with_label("  Save  ");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(OnCodecSave), (gpointer) p_ui);
    gtk_box_pack_end (GTK_BOX (btn_box), save_btn, FALSE, FALSE, 0);

    if (*(p_ui->codec->encoder) == '\0')
    	gtk_widget_set_sensitive (save_btn, FALSE);

    /* Combine everything onto the window */
    gtk_box_pack_start (GTK_BOX (main_vbox), p_ui->fixed_cntr, FALSE, FALSE, 0);

    if (*(p_ui->codec->encoder) != '\0')
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (p_ui->codec_cntr), FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (main_vbox), btn_box, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(p_ui->window), main_vbox);

    /* Exit when window closed */
    p_ui->close_handler = g_signal_connect(p_ui->window, "delete-event", G_CALLBACK(OnCodecDelete), NULL);

    return;
}


/* Dispaly the fixed details */

void fixed_details(CodecUi *p_ui)
{  
    /* Create the grid */
    p_ui->fixed_cntr = gtk_grid_new();
    gtk_widget_set_halign(GTK_WIDGET (p_ui->fixed_cntr), GTK_ALIGN_CENTER);
    p_ui->row = 0;

    /* Fixed codec details */
    add_to_grid ("Id (fourcc)", p_ui->l_fcc, p_ui->codec->fourcc, 
    		 &(p_ui->row), p_ui->fixed_cntr);
   
    add_to_grid ("Description", p_ui->l_short_desc, p_ui->codec->short_desc, 
    		 &(p_ui->row), p_ui->fixed_cntr);
   
    add_to_grid (" ", p_ui->l_long_desc, p_ui->codec->long_desc, 
    		 &(p_ui->row), p_ui->fixed_cntr);
   
    add_to_grid ("File Extension", p_ui->l_extn, p_ui->codec->extn, 
    		 &(p_ui->row), p_ui->fixed_cntr);
   
    if (*(p_ui->codec->encoder) != '\0')
	add_to_grid ("Encoder", p_ui->l_enc, p_ui->codec->encoder, 
		     &(p_ui->row), p_ui->fixed_cntr);
    else
	add_to_grid ("Encoder", p_ui->l_enc, "N/A", 
		     &(p_ui->row), p_ui->fixed_cntr);
   
    add_to_grid ("Muxer", p_ui->l_mux, p_ui->codec->muxer, 
    		 &(p_ui->row), p_ui->fixed_cntr);
   
    return;
}


/* Extra preferences, settings, etc, if any, required for a specific codec */

void codec_extra(CodecUi *p_ui)
{  
    /* Mpeg2 */
    if (strcmp(p_ui->codec->fourcc, MPEG2) == 0)
    	mpeg2_extra(p_ui);

    return;
}


/* Mpeg2 requires a frame rate of 25, 30 or 60 */

void mpeg2_extra(CodecUi *p_ui)
{  
    int i, curr_idx;
    char s[10];
    char *p;
    GtkWidget *label;

    /* Label */
    label = gtk_label_new("Frame Rate");
    gtk_widget_set_name (label, "lbl_11");
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_END);
    gtk_widget_set_margin_bottom (label, 10);
    gtk_widget_set_margin_end (label, 10);
    gtk_grid_attach(GTK_GRID (p_ui->fixed_cntr), label, 0, p_ui->row, 1, 1);

    /* Combobox */
    p_ui->cbox_mpg2 = gtk_combo_box_text_new();
    gtk_widget_set_name (p_ui->cbox_mpg2, "combobox_2");

    /* Add list items, note current value index and set active */
    p_ui->mpg2_init[0] = '\0';
    get_user_pref(MPG2_FRAMERATE, &p);

    if (p == NULL)
    {
	add_user_pref(MPG2_FRAMERATE, (char *) mpg2_fps_arr[0][1]);
	get_user_pref(MPG2_FRAMERATE, &p);
    }
    else
    {
    	strcpy(p_ui->mpg2_init, p);
    }

    for (i = 0; i < mpg2_max; i++)
    {
	sprintf(s, "%d", i);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (p_ui->cbox_mpg2), s, mpg2_fps_arr[i][0]);

	if (strcmp(p, mpg2_fps_arr[i][1]) == 0)
	    curr_idx = i;
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (p_ui->cbox_mpg2), curr_idx);
    gtk_grid_attach(GTK_GRID (p_ui->fixed_cntr), p_ui->cbox_mpg2, 1, p_ui->row, 1, 1);
    g_signal_connect(p_ui->cbox_mpg2, "changed", G_CALLBACK(OnSetMpeg2), (gpointer) p_ui);

    return;
}


/* Main table */

void codec_table(CodecUi *p_ui)
{  
    int i;
    char *p;
    GtkWidget *colhdr;
    GtkListStore *store;
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    PangoFontDescription *pf;

    /* See if encoder is present */
    if (*(p_ui->codec->encoder) == '\0')
    	return;

    /* Build a list view for the encoder properties */
    pf = pango_font_description_from_string ("Sans 9");
    store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING); 

    /* Acquire an iterator and load the data, check for user preferences */
    for(i = 0; i < encoder_max; i++)
    {
	if (((strstr(p_ui->codec->encoder, "mpeg") != NULL) &&
	     (strncmp(encoder_arr[i].encoder, "mpeg", 4) == 0)) ||
	    (strcmp(p_ui->codec->encoder, encoder_arr[i].encoder) == 0))
	{
	    check_user_default(p_ui->codec->encoder, encoder_arr[i].property_nm, &p);

	    gtk_list_store_append (store, &iter);
	    gtk_list_store_set (store, &iter,
				PROPERTY_NM, encoder_arr[i].property_nm,
				DEFAULT_VALUE, encoder_arr[i].default_val,
				PREF_VALUE, p,
				TOOL_TIP, encoder_arr[i].tooltip,
				-1);
	}
    }

    /* Tree (list) view */
    p_ui->tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    g_object_unref (G_OBJECT (store));
    p_ui->model = gtk_tree_view_get_model (GTK_TREE_VIEW (p_ui->tree));
    gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (p_ui->tree), TOOL_TIP);

    /* Column */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Property", renderer, 
    						       "text", PROPERTY_NM, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (p_ui->tree), column);
    g_object_set (G_OBJECT (renderer), "font-desc", pf, NULL);

    /* Header font */
    colhdr = gtk_tree_view_column_get_button (column);
    gtk_widget_set_name (colhdr, "lbl_11");

    /* Column */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Default Value", renderer, 
    						       "text", DEFAULT_VALUE, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (p_ui->tree), column);
    gtk_cell_renderer_set_sensitive (GTK_CELL_RENDERER (renderer), TRUE);
    g_object_set (G_OBJECT (renderer), "foreground-rgba", &RED1,
    				       "font-desc", pf, NULL);

    /* Header font */
    colhdr = gtk_tree_view_column_get_button (column);
    gtk_widget_set_name (colhdr, "lbl_11");

    /* Column */
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("User Setting", renderer, 
    						       "text", PREF_VALUE, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (p_ui->tree), column);
    gtk_cell_renderer_set_sensitive (GTK_CELL_RENDERER (renderer), TRUE);
    g_object_set (G_OBJECT (renderer), "foreground", "blue",
    				       "editable", TRUE,
    				       "font-desc", pf, NULL);
    g_signal_connect(renderer, "edited", (GCallback) OnPropertyEdit, p_ui);

    /* Header font */
    colhdr = gtk_tree_view_column_get_button (column);
    gtk_widget_set_name (colhdr, "lbl_11");

    /* Create a container for the view */
    p_ui->codec_cntr = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (p_ui->codec_cntr),
    				   GTK_POLICY_NEVER,
    				   GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (p_ui->codec_cntr), p_ui->tree);
    gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (p_ui->codec_cntr), 400);
    gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (p_ui->tree), GTK_TREE_VIEW_GRID_LINES_BOTH);

    /* Free font */
    pango_font_description_free (pf);

    return;
}


/* Add a row to a grid */

void add_to_grid(char *nm, GtkWidget *item, char *txt, 
		 int *row, GtkWidget *grid)
{  
    GtkWidget *label;

    /* Label */
    label = gtk_label_new(nm);
    gtk_widget_set_name (label, "lbl_11");
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_END);
    gtk_widget_set_margin_bottom (label, 10);
    gtk_widget_set_margin_end (label, 10);
    gtk_grid_attach(GTK_GRID (grid), label, 0, *row, 1, 1);

    /* Detail */
    item = gtk_label_new(txt);
    gtk_widget_set_name (item, "lbl_1");
    gtk_widget_set_halign(GTK_WIDGET (item), GTK_ALIGN_START);
    gtk_widget_set_margin_bottom (item, 10);
    gtk_grid_attach(GTK_GRID (grid), item, 1, *row, 1, 1);

    (*row)++;

    return;
}


/* Return (a pointer to) the codec array and the size */

codec_t * get_codec_arr(int *max)
{
    *max = codec_max;

    return codec_arr;
}


/* Return the file extension for a codec */

char * get_codec_extn(char *fcc)
{
    int i;

    for(i = 0; i < codec_max; i++)
    {
    	if (strcmp(fcc, codec_arr[i].fourcc) == 0)
	    return codec_arr[i].extn;
    }

    return NULL;
}


/* Return the codec data for a codec */

codec_t * get_codec(char *fcc)
{
    int i;

    for(i = 0; i < codec_max; i++)
    {
    	if (strcmp(fcc, codec_arr[i].fourcc) == 0)
	    return &(codec_arr[i]);
    }

    return NULL;
}


/* Return the codec data by row number */

codec_t * get_codec_idx(int idx)
{
    if (idx < 0 || idx >= codec_max)
	return NULL;

    return &(codec_arr[idx]);
}


/* Return the type for a property */

int codec_property_type(char *enc, char *nm)
{
    int i, pr_type;

    pr_type = -1;

    for(i = 0; i < encoder_max; i++)
    {
	if (((strstr(enc, "mpeg") != NULL) && (strncmp(encoder_arr[i].encoder, "mpeg", 4) == 0)) ||
	    (strcmp(enc, encoder_arr[i].encoder) == 0))
	{
	    if (strcmp(encoder_arr[i].property_nm, nm) == 0)
	    {
	    	pr_type = encoder_arr[i].type;
	    	break;
	    }
	}
    }

    return pr_type;
}


/* Check if property changes have been made */

int codec_save_reqd(CodecUi *p_ui)
{
    int pr_len, fnd;
    char *p;
    GtkTreeIter iter;
    gboolean valid;

    /* Check for encoder */
    if (*(p_ui->codec->encoder) == '\0')
    	return FALSE;

    /* Iterate the user preference column values */
    if ((valid = gtk_tree_model_get_iter_first (p_ui->model, &iter)) == FALSE)
    	return FALSE;

    fnd = FALSE;

    while(valid == TRUE)
    {
	gchar *nm;
	gchar *usr_val;

	/* Get the property name and any user value */
	gtk_tree_model_get (p_ui->model, &iter,
			    PROPERTY_NM, &nm,
			    PREF_VALUE, &usr_val, -1);

	/* Check if the user value has changed or if it is a new preference */
	if (usr_val != NULL)
	{
	    pr_len = strlen(nm);
	    match_key_val_combo(p_ui->codec->encoder, nm, pr_len, &p);
	    
	    if (p == NULL)
	    {
	    	if (strlen(usr_val) != 0)
		    fnd = TRUE;
	    }
	     
	    else if (strcmp((p + pr_len + 1), usr_val) != 0)
	    {
		fnd = TRUE;
	    }
	}
	
	g_free (usr_val);

	if (fnd == TRUE)
	    break;

	valid = gtk_tree_model_iter_next (p_ui->model, &iter);
    };

    return fnd;
}


/* Check if extra changes have been made */

int extra_save_reqd(CodecUi *p_ui)
{
    char *p;

    /* Mpeg2 */
    if (strcmp(p_ui->codec->fourcc, MPEG2) == 0)
    {
	get_user_pref(MPG2_FRAMERATE, &p);

	if (strcmp(p, p_ui->mpg2_init) != 0)
	    return TRUE;
    }

    return FALSE;
}


/* Extra changes may need undoing */

void extra_save_undo(CodecUi *p_ui)
{
    char *p;

    /* Mpeg2 */
    if (strcmp(p_ui->codec->fourcc, MPEG2) == 0)
    {
	if (p_ui->mpg2_init[0] == '\0')
	    delete_user_pref(MPG2_FRAMERATE);
	else
	    set_user_pref(MPG2_FRAMERATE, p_ui->mpg2_init);
    }

    return;
}


/* Reset any intial extra values */

void set_extra_default(CodecUi *p_ui)
{
    char *p;

    /* Mpeg2 */
    if (strcmp(p_ui->codec->fourcc, MPEG2) == 0)
    {
	get_user_pref(MPG2_FRAMERATE, &p);
	strcpy(p_ui->mpg2_init, p);
    }

    return;
}


/* Set the user default encoder property values */

void set_enc_defaults(CodecUi *p_ui)
{
    int pr_len, v_len, p2_total, p2_idx, idx;
    char *p, *p2;
    char p2_key[30];
    char key[30];
    char val[30];
    GtkTreeIter iter;
    gboolean valid;

    /* Get the current number user settings for the encoder */
    p2 = enc_property_total(p2_key, &p2_total, &p2_idx, p_ui);

    /* Iterate the user preference column values */
    if ((valid = gtk_tree_model_get_iter_first (p_ui->model, &iter)) == FALSE)
    	return;

    while(valid == TRUE)
    {
	gchar *nm;
	gchar *usr_val;

	/* Get the property name and any user value */
	gtk_tree_model_get (p_ui->model, &iter,
			    PROPERTY_NM, &nm,
			    PREF_VALUE, &usr_val, -1);

	/* No action */
	if (usr_val == NULL)
	{
	    g_free (usr_val);
	    valid = gtk_tree_model_iter_next (p_ui->model, &iter);
	    continue;
	}

	/* Try to find an existing setting */
	pr_len = strlen(nm);
	v_len = strlen(usr_val);
	idx = match_key_val_combo(p_ui->codec->encoder, nm, pr_len, &p);
	    
	// New preference - ignore if the value is blank
	// Create the encoder property count setting if necessary
	if (p == NULL && v_len != 0)
	{
	    if (p2 == NULL)
	    {
		sprintf(val, "%d", p2_total);
		add_user_pref(p2_key, val);
		p2 = enc_property_total(p2_key, &p2_total, &p2_idx, p_ui);
	    }

	    sprintf(key, "%s%02d", p_ui->codec->encoder, p2_total);
	    sprintf(val, "%s=%s", nm, usr_val);

	    add_user_pref_idx(key, val, p2_idx);
	    p2_idx++;
	    p2_total++;
	}
	     
	/* Delete - match found but the value is blank */
	else if (p != NULL && v_len == 0)
	{
	    get_pref_key(idx, key);
	    delete_user_pref(key);
	    p2_total--;
	    p2_idx--;
	}

	/* Update - match found and the value has changed */
	else if (p != NULL)
	{
	    if (strcmp((p + pr_len + 1), usr_val) != 0)
	    {
		get_pref_key(idx, key);
		sprintf(val, "%s=%s", nm, usr_val);
		set_user_pref(key, val);
	    }
	}
	
	/* Next */
	g_free (usr_val);
	valid = gtk_tree_model_iter_next (p_ui->model, &iter);
    };


    if (p2_total == 0)
    {
	delete_user_pref(p2_key);
    }
    else
    {
	sprintf(val, "%d", p2_total);
	set_user_pref(p2_key, val);
    }

    return;
}


/* Look up the property count setting for the encoder (if exists) */

char * enc_property_total(char *key, int *cnt, int *idx, CodecUi *p_ui)
{
    char *p;

    *cnt = 0;
    *idx = -1;

    sprintf(key, "%sMAX", p_ui->codec->encoder);
    *idx = get_user_pref(key, &p);

    if (p != NULL)
    	*cnt = atoi(p);

    return p;
}


/* Set up default user preferences. All preferences may not be present */

void init_codec_prop_prefs()
{
    int i, len, p_max;
    char s[30];
    char *p, *p2;

    /* Initial codec property defaults */
    for(i = 0; i < init_pref_max; i++)
    {
	sprintf(s, "%sMAX", codec_def[i][0]);
	get_user_pref(s, &p);

	if (p == NULL)
	    p_max = 0;
	else
	    p_max = atoi(p);

	if ((p2 = strstr(codec_def[i][1], "=")) == NULL)
	    continue;

	len = p2 - codec_def[i][1];
	match_key_val_combo((char *) codec_def[i][0], (char *) codec_def[i][1], len, &p);

	if (p != NULL)
	    continue;

	init_codec_pref((char *) codec_def[i][0], (char *) codec_def[i][1], p_max);
	p_max++;
	init_codec_max(s, p_max);
    };

    return;
}


/* Default codec preferences - encoder name plus a 2 digit counter */

void init_codec_pref(char *key, char *val, int cnt)
{
    char s[30];

    sprintf(s, "%s%02d", key, cnt);
    add_user_pref(s, val);

    return;
}


/* Default codec maximum preference */

void init_codec_max(char *key, int cnt)
{
    char s[10];
    char *p;

    sprintf(s, "%d", cnt);
    get_user_pref(key, &p);

    if (p == NULL)
	add_user_pref(key, s);
    else
	set_user_pref(key, s);

    return;
}


/* Determine if the user a setting for a property */

void check_user_default(char *enc, char *prop, char **usr)
{
    int pr_len;
    char *p;

    /* Search preferences for the key/property */
    *usr = NULL;
    pr_len = strlen(prop);
    match_key_val_combo(enc, prop, pr_len, &p);

    if (p == NULL)
    	return;

    if (*(p + pr_len) == '=')
	*usr = p + pr_len + 1;
	
    return;
}


/* Callback for changes to codec property value */

void OnPropertyEdit(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer user_data)
{
    CodecUi *ui;
    GtkTreeIter iter;
    char s[30];

    /* Get data */
    ui = (CodecUi *) user_data;

    /* Change the store */
    if (gtk_tree_model_get_iter_from_string (ui->model, &iter, path_string) == FALSE)
    {
    	sprintf(s, "Table data found at %s", path_string);
	log_msg("CAM0008", s, "CAM0008", ui->window);
    	return;
    }

    gtk_list_store_set (GTK_LIST_STORE (ui->model), &iter, PREF_VALUE, new_text, -1);

    return;
}


/* Callback for setting Mpegw frame rate */

void OnSetMpeg2(GtkWidget *cbox, gpointer user_data)
{
    int idx;

    if ((idx = gtk_combo_box_get_active(GTK_COMBO_BOX (cbox))) == -1)
    	return;

    set_user_pref(MPG2_FRAMERATE, (char *) mpg2_fps_arr[idx][1]);

    return;
}


/* Callback for apply changes and close */

void OnCodecSave(GtkWidget *btn, gpointer user_data)
{
    CodecUi *ui;
    int extra_sv, codec_sv;

    /* Get data */
    ui = (CodecUi *) user_data;

    /* Check for changes */
    extra_sv = extra_save_reqd(ui);
    codec_sv = codec_save_reqd(ui);

    if (codec_sv == FALSE && extra_sv == FALSE)
    {
    	info_dialog(ui->window, "There are no changes to save!", "");
    	return;
    }

    /* Store preferences */
    if (codec_sv == TRUE)
	set_enc_defaults(ui);

    /* Save to file */
    write_user_prefs(ui->window);

    if (extra_sv == TRUE)
    	set_extra_default(ui);

    return;
}


// Callback for window close
// Destroy the window and de-register the window 
// Check for changes

void OnCodecClose(GtkWidget *window, gpointer user_data)
{ 
    GtkWidget *dialog;
    CodecUi *ui;
    gint response;
    int extra_sv, codec_sv;

    /* Get data */
    ui = (CodecUi *) g_object_get_data (G_OBJECT (window), "ui");

    /* Check for changes */
    extra_sv = extra_save_reqd(ui);
    codec_sv = codec_save_reqd(ui);

    if (codec_sv == TRUE || extra_sv == TRUE)
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

    /* Undo any extra specific settings */
    if (extra_sv == TRUE)
    	extra_save_undo(ui);

    /* Close the window, free the screen data and block any secondary close signal */
    g_signal_handler_block (window, ui->close_handler);

    deregister_window(window);
    gtk_window_close(GTK_WINDOW(window));

    free(ui);

    return;
}


/* Window delete event */

gboolean OnCodecDelete(GtkWidget *window, GdkEvent *ev, gpointer user_data)
{
    OnCodecClose(window, user_data);

    return TRUE;
}
