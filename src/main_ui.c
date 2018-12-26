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
** Description: Functions to create the main user interface for the application.
**
** Author:	Anthony Buckley
**
** History
**	27-Dec-2013	Initial code
**
*/



/* Defines */
#define MAIN_UI


/* Includes */
#include <stdlib.h>  
#include <string.h>  
#include <libgen.h>  
#include <gtk/gtk.h>  
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gdk/gdkkeysyms.h>  
#include <linux/videodev2.h>
#include <cairo/cairo.h>
//#include <gtk/gtkfontchooser.h>
#include <session.h>
#include <main.h>
#include <cam.h>
#include <defs.h>
#include <preferences.h>


/* Prototypes */

void main_ui(CamData *, MainUi *);
GtkWidget* create_toolbar(MainUi *, CamData *);
GtkWidget* create_presetbar(MainUi *);
GtkWidget* create_cntl_panel(MainUi *, CamData *);
void video_settings(int*, CamData *, MainUi *);
void exposure_settings(int*, CamData *, MainUi *);
void create_panel_btn(GtkWidget **, char *, char *, int, int, PangoFontDescription **, MainUi *);
GtkWidget* create_menu(MainUi *, CamData *);
void add_camera_list(GtkWidget**, MainUi *, CamData *);
void colour_fmt(int **, PangoFontDescription **, CamData *, MainUi *);
void clrfmt_res(int **, PangoFontDescription **, CamData *, MainUi *);
void clrfmt_res_list(MainUi *, CamData *);
void clrfmt_res_fps(int **, PangoFontDescription **, CamData *, MainUi *);
void clrfmt_res_fps_list(MainUi *, CamData *);
void setup_label_combobx(char *, PangoFontDescription **, GtkWidget *, int, GtkWidget **, char *);
void cbox_def_vals(GtkWidget **, char *[], int);
void scale_cam_ctrl(struct v4l2_queryctrl *, PangoFontDescription **, int , GtkWidget *, GtkWidget *, CamData *);
void menu_cam_ctrl(struct v4l2_list *, PangoFontDescription **, int, GtkWidget *, GtkWidget *, CamData *);
void radio_cam_ctrl(struct v4l2_list *, PangoFontDescription **, int, GtkWidget *, GtkWidget *, CamData *);
void menu_list_ctrl(struct v4l2_list *, PangoFontDescription **, int, GtkWidget *, GtkWidget *, CamData *);
void menu_radio_item(GtkWidget **, GtkWidget *, PangoFontDescription **, 
		     int, char *, long, struct v4l2_list *, struct v4l2_queryctrl *, CamData *, GtkWidget *);
void boolean_radio_item(GtkWidget **, GtkWidget *, PangoFontDescription **, 
		        int, char *, long, struct v4l2_queryctrl *, CamData *, GtkWidget *);
void check_audio(struct v4l2_queryctrl *, long *);
GtkWidget * ctrl_label(char *, PangoFontDescription **, char *, int, GtkWidget *);
int ctrl_flag_ok(struct v4l2_queryctrl *, GtkWidget *, char *, long, CamData *, GtkWidget *);
void update_main_ui_res(MainUi *, CamData *);
void update_main_ui_fps(MainUi *, CamData *);
void update_main_ui_video(long, long, MainUi *);
int update_main_ui_clrfmt(char *, MainUi *);
void set_scale_val(GtkWidget *, char *, long);
int reset_cntl_panel(MainUi *, CamData *);
void load_profiles(GtkWidget *, char *, int, int);
void set_night_view_on(MainUi *);
void set_night_view_off(MainUi *);
GtkWidget * debug_cntr(GtkWidget *);


extern void get_session(char*, char**);
extern int set_session(char*, char*);
extern int camera_setup(camera_t *, GtkWidget *);
extern char* get_profile_name(int);
extern struct v4l2_queryctrl * get_next_ctrl(int);
extern int std_controls(camera_t *);
extern void pxl2fourcc(pixelfmt pixelformat, char *s);
extern int calc_fps(pixelfmt, pixelfmt);
extern void res_to_long(char *, long *, long *);
extern int get_fps(CamData *, char *);
extern void log_msg(char*, char*, char*, GtkWidget*);
extern struct v4l2_list * find_fmt(CamData *, struct v4l2_fmtdesc **, char *);
extern struct v4l2_list * find_frm(CamData *, struct v4l2_list *, struct v4l2_frmsizeenum **, char *);
extern int cam_fmt_update(CamData *, char *);
extern int cam_fps_update(CamData *, char *);
extern void session_ctrl_val(struct v4l2_queryctrl *, char *, long *);
extern void save_ctrl(struct v4l2_queryctrl *, char *, long, CamData *, GtkWidget *); 
extern GtkWidget * find_widget_by_name(GtkWidget *, char *);
extern void match_session(char *, char *, int, int *);
extern int get_user_pref(char *, char **);
extern void swap_fourcc(char *, char *);
//extern void debug_session();

extern void OnSetProfile(GtkWidget*, gpointer);
extern void OnManagePreset(GtkWidget*, gpointer);
extern void OnSavePreset(GtkWidget*, gpointer);
extern void OnSetClrFmt(GtkWidget*, gpointer);
extern void OnSetFps(GtkWidget*, gpointer);
extern void OnSetRes(GtkWidget*, gpointer);
extern void OnSetCtrl(GtkRange*, gpointer);
extern void OnCtrlDefVal(GtkWidget*, gpointer);
extern void OnCtrlReset(GtkWidget*, gpointer);
extern void OnOtherCtrl(GtkWidget*, gpointer);
extern void OnSetCtrlCbox(GtkWidget*, gpointer);
extern void OnSetCtrlRadio(GtkToggleButton *, gpointer);
extern void OnSetCamMenu(GtkWidget*, gpointer);
extern void OnStartCapUi(GtkWidget*, gpointer);
extern void OnStartCap(GtkWidget*, gpointer);
extern void OnStopCap(GtkWidget*, gpointer);
extern void OnSnapShot(GtkWidget*, gpointer);
extern void OnPauseCap(GtkWidget*, gpointer);
extern void OnSnapUi(GtkWidget*, gpointer);
extern void OnCameraSel(GtkWidget*, gpointer);
extern void OnCamDetail(GtkWidget*, gpointer);
extern void OnCamDefault(GtkWidget*, gpointer);
extern void OnCamRestart(GtkWidget*, gpointer);
extern void OnCamScan(GtkWidget*, gpointer);
extern void OnPrefs(GtkWidget*, gpointer);
extern void OnNightVision(GtkWidget*, gpointer);
extern void OnReticule(GtkWidget*, gpointer);
extern void OnAbout(GtkWidget*, gpointer);
extern void OnViewLog(GtkWidget*, gpointer);
extern void OnQuit(GtkWidget*, gpointer);
//extern GList* gst_camera_devices(gchar*);
extern struct camlistNode* dev_camera_devices(GtkWidget*);
extern void app_msg(char*, char *, GtkWidget *);
extern void OnRealise(GtkWidget*);
extern gboolean OnExpose (GtkWidget*, cairo_t *, gpointer);
extern gboolean OnNvExpose (GtkWidget *, cairo_t *, gpointer);


/* Globals */

static const char *debug_hdr = "DEBUG-main_ui.c ";


/* Create the user interface and set the CallBacks */

void main_ui(CamData *cam_data, MainUi *m_ui)
{  
    GtkWidget *mbox, *vbox;  
    GtkWidget *cntl_frame_grid;  
    GtkWidget *menu_bar;  
    GtkWidget *toolbar, *presetbar;  
    PangoFontDescription *font_desc;

    /* Initial */
    font_desc = pango_font_description_from_string ("Sans 9");

    /* Set up the UI window */
    m_ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);  
    g_object_set_data (G_OBJECT (m_ui->window), "cam_data", cam_data);
    g_object_set_data (G_OBJECT (m_ui->window), "ui", m_ui);
    gtk_window_set_title(GTK_WINDOW(m_ui->window), TITLE);
    gtk_window_set_position(GTK_WINDOW(m_ui->window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(m_ui->window), 200, 200);
    gtk_container_set_border_width(GTK_CONTAINER(m_ui->window), 10);

    /* Main view */
    mbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    gtk_widget_set_halign(GTK_WIDGET (mbox), GTK_ALIGN_START);

    /* DRAWING AREA TO SHOW VIDEO */
    m_ui->video_window = gtk_drawing_area_new();
    gtk_widget_set_double_buffered (m_ui->video_window, FALSE);
    gtk_widget_set_size_request (m_ui->video_window, STD_VWIDTH, STD_VHEIGHT);
    gtk_widget_set_halign (m_ui->video_window, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (m_ui->video_window, GTK_ALIGN_CENTER);

    m_ui->scrollwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (m_ui->scrollwin),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (m_ui->scrollwin), m_ui->video_window);
    gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (m_ui->scrollwin), STD_VWIDTH);
    gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (m_ui->scrollwin), STD_VHEIGHT);
    //gtk_scrolled_window_set_max_content_width (GTK_SCROLLED_WINDOW (m_ui->scrollwin), MAX_VWIDTH);
    //gtk_scrolled_window_set_max_content_height (GTK_SCROLLED_WINDOW (m_ui->scrollwin), MAX_VHEIGHT);

    g_signal_connect (m_ui->video_window, "realize", G_CALLBACK (OnRealise), cam_data);
    g_signal_connect (m_ui->video_window, "draw", G_CALLBACK (OnExpose), m_ui);

    /* MENU */
    menu_bar = create_menu(m_ui, cam_data);

    /* QUICK ACCESS TOOLBARS */
    toolbar = create_toolbar(m_ui, cam_data);
    presetbar = create_presetbar(m_ui);

    /* CONTROL PANEL */
    cntl_frame_grid = create_cntl_panel(m_ui, cam_data);

    /* Box to hold video window and control panel */
    vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_pack_start (GTK_BOX (vbox), m_ui->scrollwin, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), cntl_frame_grid, FALSE, FALSE, 0);

    /* INFORMATION AREA AT BOTTOM OF WINDOW */
    m_ui->status_info = gtk_label_new(NULL);
    gtk_widget_override_font (m_ui->status_info, font_desc);
    gtk_widget_set_margin_top(GTK_WIDGET (m_ui->status_info), 5);
    gtk_label_set_text(GTK_LABEL (m_ui->status_info), " ");
    gtk_widget_set_halign(GTK_WIDGET (m_ui->status_info), GTK_ALIGN_START);

    /* Combine everything onto the window */
    gtk_box_pack_start (GTK_BOX (mbox), menu_bar, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (mbox), toolbar, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (mbox), presetbar, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (mbox), vbox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (mbox), m_ui->status_info, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(m_ui->window), mbox);  

    /* Exit when window closed */
    g_signal_connect(m_ui->window, "destroy", G_CALLBACK(OnQuit), m_ui->window);  

    /* Clean up */
    pango_font_description_free (font_desc);

    /* Show window */
    gtk_widget_show_all(m_ui->window);
    gtk_window_get_size (GTK_WINDOW(m_ui->window), &m_ui->main_width, &m_ui->main_height);

    return;
}


/*
** Menu function for AstroCTC application.
**
**  File	     Camera		Capture		Options		Help
**   - Exit    	      - List of cams	 - Start	 - Preferences	 - About
**   	       	      - Camera Info	 - Stop		 - Night Vision
**		      - Restart Video	 - Pause	 - Reticule
**					 - Snapshot	                 
*/

GtkWidget* create_menu(MainUi *m_ui, CamData *cam_data)
{
    GtkWidget *menu_bar;
    GtkWidget *file_menu, *cap_menu, *opt_menu, *help_menu;
    GtkWidget *file_hdr, *cap_hdr, *opt_hdr, *help_hdr;
    GtkWidget *file_exit;
    GtkWidget *cam_detail, *cam_default, *cam_restart, *cam_rescan;
    GtkWidget *opt_prefs, *opt_night;
    GtkWidget *help_about, *view_log;
    GtkWidget *sep, *sep2;
    GtkAccelGroup *accel_group = NULL;

    /* Create menubar */
    menu_bar = gtk_menu_bar_new();


    /* FILE MENU */
    file_menu = gtk_menu_new();

    /* File menu items */
    file_exit = gtk_menu_item_new_with_mnemonic ("E_xit");

    /* Add to menu */
    gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), file_exit);

    /* Callbacks */
    g_signal_connect_swapped (file_exit, "activate", G_CALLBACK (OnQuit), m_ui->window); 

    /* Show menu items */
    gtk_widget_show (file_exit);


    /* CAMERA MENU */
    m_ui->cam_menu = gtk_menu_new();

    /* Camera information, Defaults and Restart */
    sep = gtk_separator_menu_item_new();
    sep2 = gtk_separator_menu_item_new();
    cam_detail = gtk_menu_item_new_with_label ("Camera Details...");
    cam_default = gtk_menu_item_new_with_label ("Set All Defaults");
    cam_restart = gtk_menu_item_new_with_label ("Restart Video");
    cam_rescan = gtk_menu_item_new_with_label ("Reload Cameras");

    /* Add to menu */
    gtk_menu_shell_append (GTK_MENU_SHELL (m_ui->cam_menu), cam_detail);
    gtk_menu_shell_append (GTK_MENU_SHELL (m_ui->cam_menu), sep);
    gtk_menu_shell_append (GTK_MENU_SHELL (m_ui->cam_menu), cam_default);
    gtk_menu_shell_append (GTK_MENU_SHELL (m_ui->cam_menu), cam_restart);
    gtk_menu_shell_append (GTK_MENU_SHELL (m_ui->cam_menu), cam_rescan);
    gtk_menu_shell_append (GTK_MENU_SHELL (m_ui->cam_menu), sep2);

    /* Show menu items */
    gtk_widget_show (cam_detail);
    gtk_widget_show (cam_default);
    gtk_widget_show (cam_restart);
    gtk_widget_show (cam_rescan);

    /* Callbacks */
    g_signal_connect (cam_detail, "activate", G_CALLBACK (OnCamDetail), (gpointer) cam_data);
    g_signal_connect (cam_default, "activate", G_CALLBACK (OnCamDefault), m_ui->window);
    g_signal_connect (cam_restart, "activate", G_CALLBACK (OnCamRestart), m_ui->window);
    g_signal_connect (cam_rescan, "activate", G_CALLBACK (OnCamScan), m_ui->window);

    /* Camera menu items - build a list of available cameras */
    add_camera_list(&(m_ui->cam_menu), m_ui, cam_data);

    if (cam_data->camlist == NULL)
    	gtk_widget_set_sensitive (cam_detail, FALSE);


    /* CAPTURE MENU */
    cap_menu = gtk_menu_new();

    m_ui->cap_ui = gtk_menu_item_new_with_mnemonic ("_Start...");
    m_ui->cap_stop = gtk_menu_item_new_with_mnemonic ("S_top");
    m_ui->cap_pause = gtk_menu_item_new_with_label ("Pause");
    m_ui->snap_ui = gtk_menu_item_new_with_label ("Snapshot...");

    /* Add to menu */
    gtk_menu_shell_append (GTK_MENU_SHELL (cap_menu), m_ui->cap_ui);
    gtk_menu_shell_append (GTK_MENU_SHELL (cap_menu), m_ui->cap_stop);
    gtk_menu_shell_append (GTK_MENU_SHELL (cap_menu), m_ui->cap_pause);
    gtk_menu_shell_append (GTK_MENU_SHELL (cap_menu), m_ui->snap_ui);

    /* Callbacks */
    g_signal_connect (m_ui->cap_ui, "activate", G_CALLBACK (OnStartCapUi), m_ui->window);
    g_signal_connect (m_ui->cap_stop, "activate", G_CALLBACK (OnStopCap), m_ui->window);
    g_signal_connect (m_ui->cap_pause, "activate", G_CALLBACK (OnPauseCap), m_ui->window);
    g_signal_connect (m_ui->snap_ui, "activate", G_CALLBACK (OnSnapUi), m_ui->window);

    /* Show menu items */
    gtk_widget_show (m_ui->cap_ui);
    gtk_widget_show (m_ui->cap_stop);
    gtk_widget_show (m_ui->cap_pause);
    gtk_widget_show (m_ui->snap_ui);

    gtk_widget_set_sensitive (m_ui->cap_stop, FALSE);
    gtk_widget_set_sensitive (m_ui->cap_pause, FALSE);


    /* OPTIONS MENU */
    opt_menu = gtk_menu_new();

    /* Option menu items */
    opt_prefs = gtk_menu_item_new_with_mnemonic ("_Preferences...");
    sep = gtk_separator_menu_item_new();
    opt_night = gtk_check_menu_item_new_with_label ("Night Vision");
    m_ui->opt_ret = gtk_check_menu_item_new_with_label ("Reticule");

    /* Add to menu */
    gtk_menu_shell_append (GTK_MENU_SHELL (opt_menu), opt_prefs);
    gtk_menu_shell_append (GTK_MENU_SHELL (opt_menu), sep);
    gtk_menu_shell_append (GTK_MENU_SHELL (opt_menu), opt_night);
    gtk_menu_shell_append (GTK_MENU_SHELL (opt_menu), m_ui->opt_ret);

    /* Callbacks */
    g_signal_connect (opt_prefs, "activate", G_CALLBACK (OnPrefs), m_ui->window);
    g_signal_connect (opt_night, "toggled", G_CALLBACK (OnNightVision), m_ui);
    g_signal_connect (m_ui->opt_ret, "activate", G_CALLBACK (OnReticule), m_ui);

    /* Show menu items */
    gtk_widget_show (opt_prefs);
    gtk_widget_show (opt_night);
    gtk_widget_show (m_ui->opt_ret);


    /* HELP MENU */
    help_menu = gtk_menu_new();

    /* Option menu items */
    help_about = gtk_menu_item_new_with_mnemonic ("About...");
    view_log = gtk_menu_item_new_with_mnemonic ("View Log...");

    /* Add to menu */
    gtk_menu_shell_append (GTK_MENU_SHELL (help_menu), help_about);
    gtk_menu_shell_append (GTK_MENU_SHELL (help_menu), view_log);

    /* Callbacks */
    g_signal_connect (help_about, "activate", G_CALLBACK (OnAbout), m_ui);
    g_signal_connect (view_log, "activate", G_CALLBACK (OnViewLog), m_ui->window);

    /* Show menu items */
    gtk_widget_show (help_about);
    gtk_widget_show (view_log);



    /* File header menu */
    file_hdr = gtk_menu_item_new_with_mnemonic ("_File");
    gtk_widget_show (file_hdr);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_hdr), file_menu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_hdr);

    /* Camera header menu */
    m_ui->cam_hdr = gtk_menu_item_new_with_mnemonic ("_Camera");
    gtk_widget_show (m_ui->cam_hdr);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (m_ui->cam_hdr), m_ui->cam_menu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), m_ui->cam_hdr);

    /* Capture header menu */
    cap_hdr = gtk_menu_item_new_with_mnemonic ("_Capture");
    gtk_widget_show (cap_hdr);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (cap_hdr), cap_menu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), cap_hdr);

    /* Option header menu */
    opt_hdr = gtk_menu_item_new_with_mnemonic ("_Options");
    gtk_widget_show (opt_hdr);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (opt_hdr), opt_menu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), opt_hdr);

    /* Help header menu */
    help_hdr = gtk_menu_item_new_with_mnemonic ("_Help");
    gtk_widget_show (help_hdr);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (help_hdr), help_menu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), help_hdr);


    /* Accelerators */
    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW (m_ui->window), accel_group);

    gtk_widget_add_accelerator(file_exit, "activate", accel_group, GDK_KEY_q,
    			       GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
    gtk_widget_add_accelerator(m_ui->cap_ui, "activate", accel_group, GDK_KEY_g,
    			       GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
    gtk_widget_add_accelerator(m_ui->cap_stop, "activate", accel_group, GDK_KEY_h,
    			       GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
    gtk_widget_add_accelerator(opt_prefs, "activate", accel_group, GDK_KEY_F10,
    			       GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);		/* Alt key */ 

    return menu_bar;
}  


/* Obtain a list of available cameras and add to menu */

void add_camera_list(GtkWidget **cam_menu, MainUi *m_ui, CamData *cam_data)
{
    GtkWidget *cam_sel;
    struct camlistNode *tmp;
    char *cam_nm, *cam_dev, *prev_nm;
    int i;
    char s[10];

    //camlist = gst_camera_devices("v4l2src");
    //camlist = gst_camera_devices("dshowvideosrc");
    cam_data->camlist = dev_camera_devices(m_ui->window);

    /* Empty list */
    if (cam_data->camlist == NULL)
    {
	cam_sel = gtk_menu_item_new_with_label ("None");
	gtk_widget_set_name (cam_sel, "cam_none");
	strcpy(cam_data->current_cam, "No Camera");
	gtk_menu_shell_append (GTK_MENU_SHELL (*cam_menu), cam_sel);
	gtk_widget_show (cam_sel);
	gtk_widget_set_sensitive (cam_sel, FALSE);
	cam_data->mode = CAM_MODE_UNDEF;
	app_msg("CAM0010", NULL, m_ui->window);
    	return;
    }

    /* Camera list */
    tmp = cam_data->camlist;
    get_session("CAMERA", &prev_nm);
    i = 0;

    while(tmp != NULL)
    {
	cam_nm = tmp->cam->vcaps.card;
	cam_dev = tmp->cam->video_dev;
	cam_sel = gtk_menu_item_new_with_label (cam_nm);

	sprintf(s, "cam_%d", i++);
	gtk_widget_set_name (cam_sel, s);

	/* Add to menu */
	gtk_menu_shell_append (GTK_MENU_SHELL (*cam_menu), cam_sel);

	/* Callbacks */
	g_signal_connect (cam_sel, "activate", G_CALLBACK (OnCameraSel), (gpointer) cam_data);
	g_object_set_data (G_OBJECT (cam_sel), "ui", m_ui);
	g_object_set_data (G_OBJECT (cam_sel), "camera", tmp->cam);

	/* Show menu item */
	gtk_widget_show (cam_sel);
	
	if ((strlen(cam_data->current_cam) == 0) || (strcmp(cam_nm, prev_nm) == 0))
	{
	    strcpy(cam_data->current_cam, cam_nm);
	    strcpy(cam_data->current_dev, cam_dev);

	    memcpy(cam_data->current_cam_abbr, cam_data->current_cam, CAM_ABBR_SZ);
	    cam_data->current_cam_abbr[CAM_ABBR_SZ] = '\0';
	    memcpy(cam_data->current_dev_abbr, cam_data->current_dev, CAM_ABBR_SZ);
	    cam_data->current_dev_abbr[CAM_ABBR_SZ] = '\0';

	    cam_data->cam = tmp->cam;
	}

	tmp = tmp->next;
    }

    set_session("CAMERA", cam_data->current_cam);

    // Accumulate the remaining camera setup controls (already have capabilities for all cameras)
    // The data in the camera list entries is associated a menu item for each camera
    // and will be freed when the association is removed (normally program closure).
    camera_setup(cam_data->cam, m_ui->window);
     
    return;
}  


/* Create a quick access toolbar with callbacks */

GtkWidget* create_toolbar(MainUi *m_ui, CamData *cam_data)
{  
    GtkWidget *toolbar;  
    GtkToolItem *item;  
    GtkWidget *tool_icon;  
    GtkWidget *label;
    PangoFontDescription *font_desc, *font_desc2;
    int i, idx;
    char s[100];
    static char *secs[] = { "60", "90", "120", "150", "180" };
    const int DUR_COUNT = 5;
    static char *seq[] = { "x1", "x5", "x10", "x15", "x20" };
    const int SEQ_COUNT = 5;

    /* New toolbar */
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_icon_size(GTK_TOOLBAR (toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_toolbar_set_style(GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH_HORIZ);
    font_desc = pango_font_description_from_string ("Sans 9");
    font_desc2 = pango_font_description_from_string ("Sans 8");

    /* Start capture */
    idx = 0;
    tool_icon = gtk_image_new();
    tool_icon = gtk_image_new_from_icon_name("media-record", GTK_ICON_SIZE_SMALL_TOOLBAR);
    m_ui->cap_start_tb = gtk_tool_button_new(tool_icon, "Start Capture");
    gtk_tool_item_set_is_important (m_ui->cap_start_tb, TRUE);
    gtk_widget_override_font (GTK_WIDGET (m_ui->cap_start_tb), font_desc);
    gtk_tool_item_set_expand(m_ui->cap_start_tb, FALSE);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), m_ui->cap_start_tb, idx);

    g_signal_connect(m_ui->cap_start_tb, "clicked", G_CALLBACK(OnStartCap), m_ui->window);

    /* Separator */
    idx++;
    item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    /* Capture duration */
    idx++;
    label = gtk_label_new("Duration  ");
    gtk_widget_override_font (label, font_desc);
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, FALSE);
    gtk_container_add(GTK_CONTAINER (item), label);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    idx++;
    m_ui->cbox_dur = gtk_combo_box_text_new_with_entry();
    gtk_widget_override_font (m_ui->cbox_dur, font_desc2);
    m_ui->cbox_entry_dur = gtk_bin_get_child(GTK_BIN (m_ui->cbox_dur));
    gtk_widget_override_font (m_ui->cbox_entry_dur, font_desc2);
    gtk_entry_set_width_chars(GTK_ENTRY (m_ui->cbox_entry_dur), 4);

    for(i = 0; i < DUR_COUNT; i++)
    {
	sprintf(s, "%d", i);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (m_ui->cbox_dur), s, secs[i]);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (m_ui->cbox_dur), 1);	// 90 secs

    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, FALSE);
    gtk_container_add(GTK_CONTAINER (item), m_ui->cbox_dur);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    /* Separator */
    idx++;
    item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    /* Stop capture */
    idx++;
    tool_icon = gtk_image_new();
    tool_icon = gtk_image_new_from_icon_name("media-playback-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
    m_ui->cap_stop_tb = gtk_tool_button_new(tool_icon, "Stop");
    gtk_tool_item_set_is_important (m_ui->cap_stop_tb, TRUE);
    gtk_widget_override_font (GTK_WIDGET (m_ui->cap_stop_tb), font_desc);
    gtk_tool_item_set_expand(m_ui->cap_stop_tb, FALSE);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), m_ui->cap_stop_tb, idx);
    gtk_widget_set_sensitive (GTK_WIDGET (m_ui->cap_stop_tb), FALSE);

    g_signal_connect(m_ui->cap_stop_tb, "clicked", G_CALLBACK(OnStopCap), m_ui->window);

    /* Separator */
    idx++;
    item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    /* Pause capture */
    idx++;
    tool_icon = gtk_image_new();
    tool_icon = gtk_image_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_SMALL_TOOLBAR);
    m_ui->cap_pause_tb = gtk_tool_button_new(tool_icon, "Pause");
    gtk_tool_item_set_is_important (m_ui->cap_pause_tb, TRUE);
    gtk_widget_override_font (GTK_WIDGET (m_ui->cap_pause_tb), font_desc);
    gtk_tool_item_set_expand(m_ui->cap_pause_tb, FALSE);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), m_ui->cap_pause_tb, idx);
    gtk_widget_set_sensitive (GTK_WIDGET (m_ui->cap_pause_tb), FALSE);

    g_signal_connect(m_ui->cap_pause_tb, "clicked", G_CALLBACK(OnPauseCap), m_ui->window);

    /* Separator */
    idx++;
    item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    /* Snapshot */
    idx++;
    m_ui->snap_tb = gtk_tool_button_new(NULL, "Snapshot");
    gtk_tool_item_set_is_important (m_ui->snap_tb, TRUE);
    gtk_widget_override_font (GTK_WIDGET (m_ui->snap_tb), font_desc);
    gtk_tool_item_set_expand(m_ui->snap_tb, FALSE);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), m_ui->snap_tb, idx);

    g_signal_connect(m_ui->snap_tb, "clicked", G_CALLBACK(OnSnapShot), (gpointer) m_ui->window);

    /* Snapshot count - normally one, but a sequence is allowed */
    idx++;
    m_ui->cbox_seq = gtk_combo_box_text_new();
    gtk_widget_override_font (m_ui->cbox_seq, font_desc2);
    gtk_widget_set_name(m_ui->cbox_seq, "seq_count");

    for(i = 0; i < SEQ_COUNT; i++)
    {
	sprintf(s, "%d", i);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (m_ui->cbox_seq), s, seq[i]);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (m_ui->cbox_seq), 0);

    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, FALSE);
    gtk_container_add(GTK_CONTAINER (item), m_ui->cbox_seq);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    //GtkWidgetPath *widget_path = gtk_widget_get_path(cbox_seq);
    //char *pp = gtk_widget_path_to_string(widget_path);
    //printf("%s seq path: %s\n", debug_hdr, pp);

    /* Separator */
    idx++;
    item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    /* Object title */
    idx++;
    label = gtk_label_new("Title  ");
    gtk_widget_override_font (label, font_desc);
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, FALSE);
    gtk_container_add(GTK_CONTAINER (item), label);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    idx++;
    m_ui->obj_title = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY (m_ui->obj_title), 30);
    gtk_entry_set_width_chars(GTK_ENTRY (m_ui->obj_title), 15);
    gtk_widget_override_font (m_ui->obj_title, font_desc);
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, FALSE);
    gtk_container_add(GTK_CONTAINER (item), m_ui->obj_title);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    pango_font_description_free (font_desc);
    pango_font_description_free (font_desc2);

    return toolbar;
}


/* Create a toolbar with callbacks to select and manage saved capture profiles */

GtkWidget* create_presetbar(MainUi *m_ui)
{  
    GtkWidget *toolbar;  
    GtkToolItem *item;  
    GtkWidget *label;
    GtkWidget *mbtn;
    PangoFontDescription *font_desc;
    int idx;
    char *p;
    gchar *nm;

    /* New toolbar */
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR (toolbar), GTK_TOOLBAR_TEXT);
    font_desc = pango_font_description_from_string ("Sans 9");

    /* List of saved profiles */
    idx = 0;
    label = gtk_label_new("  Capture Presets  ");
    gtk_widget_override_font (label, font_desc);
    gtk_widget_override_color(label, GTK_STATE_FLAG_NORMAL, &DARK_BLUE);
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, FALSE);
    gtk_container_add(GTK_CONTAINER (item), label);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    idx++;
    m_ui->cbox_profile = gtk_combo_box_text_new();
    gtk_widget_override_font (m_ui->cbox_profile, font_desc);

    /* Load any preset profiles */
    get_user_pref(DEFAULT_PROFILE, &p);
    load_profiles(m_ui->cbox_profile, p, m_ui->preset_hndlr_id, FALSE);

    /* Place in window */
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, FALSE);
    gtk_container_add(GTK_CONTAINER (item), m_ui->cbox_profile);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    m_ui->preset_hndlr_id = g_signal_connect(m_ui->cbox_profile, "changed", G_CALLBACK(OnSetProfile), m_ui->window);

    /* Preset profile maintenance button */
    idx++;
    mbtn = gtk_button_new_with_label(" Manage Presets... ");  
    gtk_widget_override_font (mbtn, font_desc);
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, FALSE);
    gtk_container_add(GTK_CONTAINER (item), mbtn);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    g_signal_connect(mbtn, "clicked", G_CALLBACK(OnManagePreset), m_ui->window);

    /* Save current profile button */
    idx++;
    m_ui->save_profile_btn = gtk_button_new_with_label("  Save  ");  
    gtk_widget_override_font (m_ui->save_profile_btn, font_desc);
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, FALSE);
    gtk_container_add(GTK_CONTAINER (item), m_ui->save_profile_btn);
    gtk_toolbar_insert(GTK_TOOLBAR (toolbar), item, idx);

    nm = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (m_ui->cbox_profile));

    if ((strcmp(nm, LAST_SESSION) == 0) || (strcmp(nm, PRF_NONE) == 0))
	gtk_widget_set_sensitive (m_ui->save_profile_btn, FALSE);
    else
	gtk_widget_set_sensitive (m_ui->save_profile_btn, TRUE);

    g_free(nm);

    g_signal_connect(m_ui->save_profile_btn, "clicked", G_CALLBACK(OnSavePreset), m_ui);

    pango_font_description_free (font_desc);

    return toolbar;
}


/* Control Panel for video adjustments */

GtkWidget* create_cntl_panel(MainUi *m_ui, CamData *cam_data)
{  
    GtkWidget *frame;  
    PangoFontDescription *font_desc;
    char s[100];
    int row;

    /* Place all the control settings into a grid */
    m_ui->cntl_grid = gtk_grid_new();
    gtk_widget_set_name(m_ui->cntl_grid, "ctrl_cntr_main");
    gtk_grid_set_row_spacing(GTK_GRID (m_ui->cntl_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID (m_ui->cntl_grid), 5);
    gtk_container_set_border_width (GTK_CONTAINER (m_ui->cntl_grid), 5);

    /* Overall label - apply a b/g colour and bolding */
    sprintf(s, "%s Camera Settings", TITLE);
    m_ui->cntl_hdg = gtk_label_new(s);
    gtk_grid_attach(GTK_GRID (m_ui->cntl_grid), m_ui->cntl_hdg, 0, 0, 2, 2);

    gtk_widget_override_background_color(m_ui->cntl_hdg, GTK_STATE_FLAG_NORMAL, &BLUE_GRAY);
    font_desc = pango_font_description_from_string ("Sans 10");
    pango_font_description_set_weight(font_desc, PANGO_WEIGHT_BOLD);
    gtk_widget_override_font (GTK_WIDGET (m_ui->cntl_hdg), font_desc);
    pango_font_description_free (font_desc);

    if (cam_data->cam == NULL)
	return m_ui->cntl_grid;

    /* Video settings sub-panel */
    row = 2;
    video_settings(&row, cam_data, m_ui);

    /* Image exposure settings sub-panel */
    exposure_settings(&row, cam_data, m_ui);

    /* Keep a reference to the control panel */
    g_object_set_data (G_OBJECT (m_ui->window), "cntl_grid", m_ui->cntl_grid);

    /* Add some decoration to the control grid */ 
    m_ui->cntl_ev_box = gtk_event_box_new ();
    gtk_widget_override_background_color(m_ui->cntl_ev_box, GTK_STATE_FLAG_NORMAL, &BLUE_GRAY);
    gtk_container_add(GTK_CONTAINER (m_ui->cntl_ev_box), m_ui->cntl_grid);  

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER (frame), m_ui->cntl_ev_box);  

    return frame;
}


/* Control Panel - Video settings sub-panel */

void video_settings(int *row,
		    CamData *cam_data,
		    MainUi *m_ui)
{  
    GtkWidget *label;
    PangoFontDescription *font_desc;

    /* Video settings heading */
    label = gtk_label_new("Video Settings");
    gtk_widget_set_name(label, "fmt_head");
    gtk_widget_set_margin_top(GTK_WIDGET (label), 10);
    gtk_grid_attach(GTK_GRID (m_ui->cntl_grid), label, 0, *row, 1, 1);
    gtk_widget_override_color(label, GTK_STATE_FLAG_NORMAL, &DARK_BLUE);
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_START);
    (*row)++;

    /* Enumerate the supported video formats, resolutions, fps */
    font_desc = pango_font_description_from_string ("Sans 9");
    
    /* Colour format */
    colour_fmt(&row, &font_desc, cam_data, m_ui);

    /* Resolution */
    clrfmt_res(&row, &font_desc, cam_data, m_ui);

    /* Frame rate - fps */
    clrfmt_res_fps(&row, &font_desc, cam_data, m_ui);

    /* Free the font */
    pango_font_description_free (font_desc);

    return;
}


/* Add the Colour Format combobox to the Video Settings sub-panel */

void colour_fmt(int **row,
		PangoFontDescription **font_desc,
		CamData *cam_data,
		MainUi *m_ui)
{  
    GtkWidget *label;
    struct v4l2_fmtdesc *vfmt;
    struct v4l2_list *fmt_node;
    int i, sess_idx;
    char s[50];
    char fourcc[5];
    char *p;
    int hndlr_id;

    static char *clr[] = { "I420", "RGB3" };			// Standby default
    const int clr_count = 2;
    
    /* Combobox and label */
    setup_label_combobx("Colour Format", &(*font_desc), m_ui->cntl_grid, **row, 
    			&(m_ui->cbox_clrfmt), "fmt");
    (**row)++;

    /* Add list items and allow for the previous session value if any */
    i = -1;
    sess_idx = i;
    get_session(CLRFMT, &p);
    fmt_node = cam_data->cam->fmt_head;

    while(fmt_node != NULL)
    {
    	vfmt = (struct v4l2_fmtdesc *) fmt_node->v4l2_data;
	sprintf(s, "%d", ++i);
	pxl2fourcc(vfmt->pixelformat, fourcc);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (m_ui->cbox_clrfmt), s, fourcc);
	match_session(p, fourcc, i, &sess_idx);
    	fmt_node = fmt_node->next;
    }

    /* Check if there is a match on a previous session */
    if (sess_idx >= 0)
    {
	strcpy(fourcc, p);
    }
    else
    {
	/* Check for empty list (unable to query) and apply a (hopefully) default list */
	if (i == -1)
	{
	    cbox_def_vals(&(m_ui->cbox_clrfmt), clr, clr_count);
	    strcpy(fourcc, clr[0]);
	    sess_idx = 0;
	    log_msg("CAM0040", "format setting", NULL, NULL);
	}
	else
	{
	    sess_idx = i;				// Use final list item
	}
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (m_ui->cbox_clrfmt), sess_idx);
    set_session(CLRFMT, fourcc);

    hndlr_id = g_signal_connect(m_ui->cbox_clrfmt, "changed", G_CALLBACK(OnSetClrFmt), (gpointer) cam_data);
    g_object_set_data (G_OBJECT (m_ui->cbox_clrfmt), "ui", m_ui);
    g_object_set_data (G_OBJECT (m_ui->cbox_clrfmt), "hndlr_id", GINT_TO_POINTER (hndlr_id));

    /* Latest Version: Only use negotiated colour code for viewing, but keep list as validator */
    gtk_widget_set_sensitive (m_ui->cbox_clrfmt, FALSE);

    return;
}


/* Add the Resolution combobox to the Video Settings sub-panel */

void clrfmt_res(int **row,
		PangoFontDescription **font_desc,
		CamData *cam_data,
		MainUi *m_ui)
{  
    int hndlr_id;

    /* Combobox and label */
    setup_label_combobx("Resolution", &(*font_desc), m_ui->cntl_grid, **row, 
    			&(m_ui->cbox_res), "res");
    (**row)++;

    /* Set up the list items */
    clrfmt_res_list(m_ui, cam_data);

    hndlr_id = g_signal_connect(m_ui->cbox_res, "changed", G_CALLBACK(OnSetRes), (gpointer) cam_data);
    g_object_set_data (G_OBJECT (m_ui->cbox_res), "ui", m_ui);
    g_object_set_data (G_OBJECT (m_ui->cbox_res), "hndlr_id", GINT_TO_POINTER (hndlr_id));

    return;
}


/* Add the Resolution combobox to the Video Settings sub-panel */

void clrfmt_res_list(MainUi *m_ui, CamData *cam_data)
{  
    struct v4l2_fmtdesc *vfmt;
    struct v4l2_frmsizeenum *vfrm;
    struct v4l2_list *v_node, *v_node_fmt;
    int i, sess_idx;
    char s[50];
    char res_str[20];
    char *p;
    long width, height;

    static char *res[] = { "320 x 240", "640 x 480" };		// Standby default
    const int res_count = 2;

    /* Match the colour format */
    get_session(CLRFMT, &p);
    v_node_fmt = find_fmt(cam_data, &vfmt, p);

    if (v_node_fmt != NULL)
	v_node = v_node_fmt->sub_list_head;
    else
    	v_node = NULL;

    /* Add list items for the format and allow for the previous session value if any */
    i = -1;
    sess_idx = i;
    get_session(RESOLUTION, &p);

    while(v_node != NULL)
    {
    	vfrm = (struct v4l2_frmsizeenum *) v_node->v4l2_data;
	sprintf(s, "%d", ++i);
	sprintf(res_str, "%d x %d", vfrm->discrete.width, vfrm->discrete.height);	// Stepwise ?
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (m_ui->cbox_res), s, res_str);
	match_session(p, res_str, i, &sess_idx);
    	v_node = v_node->next;
    }

    /* Check if there is a match on a previous session */
    if (sess_idx >= 0)
    {
	strcpy(res_str, p);
    }
    else
    {
	/* Check for empty list (unable to query) and apply a (hopefully) default list */
	if (i == -1)
	{
	    cbox_def_vals(&(m_ui->cbox_res), res, res_count);
	    strcpy(res_str, res[0]);
	    sess_idx = 0;
	    log_msg("CAM0040", "resolution setting", NULL, NULL);
	}
	else
	{
	    sess_idx = i;				// Use final list item
	}
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (m_ui->cbox_res), sess_idx);
    set_session(RESOLUTION, res_str);

    // Set the camera (Note this covers resolution AND colour format) and 
    // adjust video window area accordingly if necessary */
    if (cam_fmt_update(cam_data, res_str) == FALSE)
    {
	log_msg("CAM0014", NULL, NULL, NULL);
    	return;
    }

    res_to_long(res_str, &width, &height);
    update_main_ui_video(width, height, m_ui);

    return;
}


/* Add the Frames per Second combobox to the Video Settings sub-panel */

void clrfmt_res_fps(int **row,
		    PangoFontDescription **font_desc,
		    CamData *cam_data,
		    MainUi *m_ui)
{  
    int hndlr_id;

    /* Combobox and label */
    setup_label_combobx("Frame Rate", &(*font_desc), m_ui->cntl_grid, **row, 
    			&(m_ui->cbox_fps), "fps");
    (**row)++;

    /* Set up the list items */
    clrfmt_res_fps_list(m_ui, cam_data);

    hndlr_id = g_signal_connect(m_ui->cbox_fps, "changed", G_CALLBACK(OnSetFps), (gpointer) cam_data);
    g_object_set_data (G_OBJECT (m_ui->cbox_fps), "ui", m_ui);
    g_object_set_data (G_OBJECT (m_ui->cbox_fps), "hndlr_id", GINT_TO_POINTER (hndlr_id));

    return;
}


/* Add the Frames per Second combobox to the Video Settings sub-panel */

void clrfmt_res_fps_list(MainUi *m_ui, CamData *cam_data)
{  
    struct v4l2_fmtdesc *vfmt;
    struct v4l2_frmsizeenum *vfrm;
    struct v4l2_frmivalenum *vfps;
    struct v4l2_list *v_node, *v_node_fmt, *v_node_res;
    int i, j, sess_idx;
    char s[50];
    char fps_str[20];
    char *p;

    static char *fps[] = { "10", "15", "25", "30" };		// Standby default
    const int fps_count = 4;

    /* Match the format */
    get_session(CLRFMT, &p);
    v_node_fmt = find_fmt(cam_data, &vfmt, p);

    /* Match the resolution */
    v_node = NULL;

    if (v_node_fmt != NULL)
    {
	get_session(RESOLUTION, &p);
	v_node_res = find_frm(cam_data, v_node_fmt, &vfrm, p);

	if (v_node_res != NULL)
	    v_node = v_node_res->sub_list_head;
    }

    /* Add list items for the format and resolution and allow for the previous session value if any */
    i = -1;
    sess_idx = i;
    get_session(FPS, &p);

    while(v_node != NULL)
    { 
    	vfps = (struct v4l2_frmivalenum *) v_node->v4l2_data;
	sprintf(s, "%d", ++i);
	j = calc_fps(vfps->discrete.denominator, vfps->discrete.numerator);
	sprintf(fps_str, "%d", j);						
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (m_ui->cbox_fps), s, fps_str);	 // Stepwise ?
	match_session(p, fps_str, i, &sess_idx);
    	v_node = v_node->next;
    }

    /* Check if there is a match on a previous session */
    if (sess_idx >= 0)
    {
	strcpy(fps_str, p);
    }
    else
    {
	/* Check for empty list (unable to query) and apply a (hopefully) default list */
	if (i == -1)
	{
	    cbox_def_vals(&(m_ui->cbox_fps), fps, fps_count);
	    strcpy(fps_str, fps[0]);
	    sess_idx = 0;
	    log_msg("CAM0040", "frame rate setting", NULL, NULL);

	    get_session(CLRFMT, &p);
	    j = get_fps(cam_data, p);

	    /* Try to get an Fps and if so check if its in the default list or not */
	    if (j > 0)
	    {
		sprintf(s, "%d", j);
		strcpy(fps_str, s);

		for(i = 0; i < fps_count; i++)
		{
		    if (strcmp(fps[i], s) == 0)
		    	break;
		}

		if (i >= fps_count)
		{
		    sprintf(s, "%d", i);
		    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (m_ui->cbox_fps), s, fps_str);
		    sess_idx = i;
		}
	    }
	}
	else
	{
	    sess_idx = i;			// Use final list item
	}
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (m_ui->cbox_fps), sess_idx);
    set_session(FPS, fps_str);

    /* Set the camera */
    if (cam_fps_update(cam_data, fps_str) == FALSE)
    {
	log_msg("CAM0016", NULL, NULL, NULL);
    	return;
    }

    return;
}


/* Create a label and combobox */

void setup_label_combobx(char *label_str,
			 PangoFontDescription **font_desc,
			 GtkWidget *grid, 
			 int row, 
			 GtkWidget **cbox,
			 char *prefix)
{  
    GtkWidget *label;
    char nm[40];

    label = gtk_label_new(label_str);
    pango_font_description_set_weight(*font_desc, PANGO_WEIGHT_BOLD);
    gtk_widget_override_font (label, *font_desc);
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_END);
    sprintf(nm, "%s_lbl", prefix);
    gtk_widget_set_name(label, nm);
    gtk_grid_attach(GTK_GRID (grid), label, 0, row, 1, 1);

    *cbox = gtk_combo_box_text_new();
    pango_font_description_set_weight(*font_desc, PANGO_WEIGHT_NORMAL);
    gtk_widget_override_font (*cbox, *font_desc);
    sprintf(nm, "%s_cbx", prefix);
    gtk_widget_set_name(*cbox, nm);
    gtk_grid_attach(GTK_GRID (grid), *cbox, 1, row, 1, 1);

    return;
}


/* The driver is sick (!), load default lists and keep fingers crossed */

void cbox_def_vals(GtkWidget **cbox, char *arr[], int max_rows)
{
    int i;
    char s[10];
    char *p;

    /* Highlight the problem */
    gtk_widget_override_color(*cbox, GTK_STATE_FLAG_NORMAL, &RED1);
    gtk_widget_set_tooltip_text (*cbox, "The camera does not support this function. "
    					"An alternative method will be used for a sample set.");

    /* Default list */
    for(i = 0; i < max_rows; i++)
    {
	sprintf(s, "%d", i);
	p = arr[i];
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (*cbox), s, p);
    }

    return;
}


/* Control Panel - Image exposure (all controls) settings sub-panel */

void exposure_settings(int *row, CamData *cam_data, MainUi *m_ui)
{  
    int init;
    GtkWidget *label;  
    struct v4l2_queryctrl *qctrl;
    PangoFontDescription *font_desc;

    /* Control settings heading */
    label = gtk_label_new("Control Settings");
    gtk_widget_set_name(label, "exp_head");
    gtk_widget_set_margin_top(GTK_WIDGET (label), 10);
    gtk_grid_attach(GTK_GRID (m_ui->cntl_grid), label, 0, *row, 1, 1);
    gtk_widget_override_color(label, GTK_STATE_FLAG_NORMAL, &DARK_BLUE);
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_START);
    (*row)++;

    /* Enumerate the standard controls found for the camera */
    font_desc = pango_font_description_from_string ("Sans 9");
    init = TRUE;
    std_controls(cam_data->cam);

    while((qctrl = get_next_ctrl(init)) != NULL)
    {
	/* Create a new slider control */
	scale_cam_ctrl(qctrl, &font_desc, *row, m_ui->cntl_grid, m_ui->window, cam_data);
	init = FALSE;
	(*row)++;
    }

    /* More Settings */
    create_panel_btn(&(m_ui->oth_ctrls_btn), " More Settings... ", "more_ctl_btn", 0, *row, &font_desc, m_ui);
    g_signal_connect(m_ui->oth_ctrls_btn, "clicked", G_CALLBACK(OnOtherCtrl), cam_data);

    /* Default control values */
    create_panel_btn(&(m_ui->def_val_btn), "  Default  ", "def_val_btn", 1, *row, &font_desc, m_ui);
    g_signal_connect(m_ui->def_val_btn, "clicked", G_CALLBACK(OnCtrlDefVal), (gpointer) cam_data);

    /* Reset */
    create_panel_btn(&(m_ui->reset_btn), "Reset", "reset_val_btn", 2, *row, &font_desc, m_ui);
    g_signal_connect(m_ui->reset_btn, "clicked", G_CALLBACK(OnCtrlReset), (gpointer) cam_data);

    (*row)++;
    pango_font_description_free (font_desc);

    return;
}


/* Control Panel - create standard button */

void create_panel_btn(GtkWidget **btn, 
		      char *lbl, char *nm, 
		      int col, int row, 
		      PangoFontDescription **pf,
		      MainUi *m_ui)
{  
    pango_font_description_set_weight(*pf, PANGO_WEIGHT_NORMAL);
    *btn = gtk_button_new_with_label(lbl);  
    gtk_widget_set_name(*btn, nm);
    gtk_widget_override_font (*btn, *pf);
    gtk_widget_set_margin_top(GTK_WIDGET (*btn), 20);
    gtk_grid_attach(GTK_GRID (m_ui->cntl_grid), *btn, col, row, 1, 1);
    g_object_set_data (G_OBJECT (*btn), "ui", m_ui);

    return;
}


/* Create a label and slider (scale) for a camera control */

void scale_cam_ctrl(struct v4l2_queryctrl *qctrl, 
		    PangoFontDescription **pf, 
		    int row,
		    GtkWidget *grid, 
		    GtkWidget *window, 
		    CamData *cam_data)
{
    long ctl_val;
    char ctl_key[10];
    GtkWidget *exp_scale;  

    /* Get last session value if any or the default */
    session_ctrl_val(qctrl, ctl_key, &ctl_val);

    /* Label */
    ctrl_label(qctrl->name, &(*pf), ctl_key, row, grid);

    /* Slider control */
    exp_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 
					  qctrl->minimum, 
					  qctrl->maximum,
					  qctrl->step);
    pango_font_description_set_weight(*pf, PANGO_WEIGHT_NORMAL);
    gtk_widget_override_font (exp_scale, *pf);
    gtk_scale_set_draw_value (GTK_SCALE (exp_scale), TRUE);
    gtk_range_set_value(GTK_RANGE (exp_scale), ctl_val);
    gtk_grid_attach(GTK_GRID (grid), exp_scale, 1, row, 1, 1);

    //g_signal_connect(exp_scale, "button-release-event", G_CALLBACK(OnSetCtrl), NULL);
    g_signal_connect(exp_scale, "change-value", G_CALLBACK(OnSetCtrl), (gpointer) cam_data);
    g_object_set_data (G_OBJECT (exp_scale), "ui_window", window);
    g_object_set_data (G_OBJECT (exp_scale), "control", qctrl);
    g_object_set_data (G_OBJECT (exp_scale), "resetable", GINT_TO_POINTER (TRUE));
    gtk_widget_set_name(exp_scale, ctl_key);

    /* Set the camera for this control value and store for the session */
    ctrl_flag_ok(qctrl, exp_scale, ctl_key, ctl_val, cam_data, window);
	
    return;
}


/* Create a menu control (list or radio) for a camera control */

void menu_cam_ctrl(struct v4l2_list *ctlNode, 
		   PangoFontDescription **pf, 
		   int row,
		   GtkWidget *grid, 
		   GtkWidget *window, 
		   CamData *cam_data)
{
    struct v4l2_queryctrl *qctrl;

    /* Depending on the number of menu items, create either a list or radio */
    qctrl = (struct v4l2_queryctrl *) ctlNode->v4l2_data;

    if (((qctrl->maximum - qctrl->minimum) / qctrl->step) > 3)
	menu_list_ctrl(ctlNode, &(*pf), row, grid, window, cam_data);
    else
	radio_cam_ctrl(ctlNode, &(*pf), row, grid, window, cam_data);
	
    return;
}


/* Create a drop-down list for a camera control */

void menu_list_ctrl(struct v4l2_list *ctlNode, 
		    PangoFontDescription **pf, 
		    int row,
		    GtkWidget *grid, 
		    GtkWidget *window, 
		    CamData *cam_data)
{
    long ctl_val;
    char ctl_key[10], s[100];
    GtkWidget *cam_ctrl_cbox;  
    struct v4l2_queryctrl *qctrl;
    struct v4l2_querymenu *qmenu;
    struct v4l2_list *tmp;
    int handler_id;

    /* Get last session value if any or the default */
    qctrl = (struct v4l2_queryctrl *) ctlNode->v4l2_data;
    session_ctrl_val(qctrl, ctl_key, &ctl_val);

    /* Combobox and label */
    setup_label_combobx(qctrl->name, &(*pf), grid, row, &(cam_ctrl_cbox), "");
    gtk_widget_set_name(cam_ctrl_cbox, ctl_key);

    /* List items */
    tmp = ctlNode->sub_list_head;

    while(tmp != NULL)
    {
	qmenu = (struct v4l2_querymenu *) tmp->v4l2_data;
	sprintf(s, "%d", qmenu->index);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (cam_ctrl_cbox), s, qmenu->name);

	tmp = tmp->next;
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX (cam_ctrl_cbox), ctl_val);

    handler_id = g_signal_connect(cam_ctrl_cbox, "changed", G_CALLBACK(OnSetCtrlCbox), (gpointer) cam_data);
    g_object_set_data (G_OBJECT (cam_ctrl_cbox), "ui_window", window);
    g_object_set_data (G_OBJECT (cam_ctrl_cbox), "control", qctrl);
    g_object_set_data (G_OBJECT (cam_ctrl_cbox), "resetable", GINT_TO_POINTER (TRUE));
    g_object_set_data (G_OBJECT (cam_ctrl_cbox), "cb_handler", GINT_TO_POINTER (handler_id));

    /* Set the camera for this control value and store for the session */
    ctrl_flag_ok(qctrl, cam_ctrl_cbox, ctl_key, ctl_val, cam_data, window);
	
    return;
}


/* Create a radio group for a camera control */

void radio_cam_ctrl(struct v4l2_list *ctlNode, 
		    PangoFontDescription **pf, 
		    int row,
		    GtkWidget *grid, 
		    GtkWidget *window, 
		    CamData *cam_data)
{
    int i;
    long ctl_val;
    char ctl_key[10];
    GtkWidget *radio_grp, *vbox, *label, *frame;
    struct v4l2_queryctrl *qctrl;
    struct v4l2_list *tmp;

    /* Get last session value if any or the default */
    qctrl = (struct v4l2_queryctrl *) ctlNode->v4l2_data;
    session_ctrl_val(qctrl, ctl_key, &ctl_val);
    check_audio(qctrl, &ctl_val);

    /* Label (plus overrides) */
    label = ctrl_label(qctrl->name, &(*pf), ctl_key, row, grid);
    gtk_widget_set_margin_top(GTK_WIDGET (label), 2);

    /* Radio group */
    pango_font_description_set_weight(*pf, PANGO_WEIGHT_NORMAL);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);

    /* Radio for what */
    if (qctrl->type == V4L2_CTRL_TYPE_BOOLEAN)
    {
	boolean_radio_item(&radio_grp, vbox, &(*pf), 0, ctl_key, ctl_val, qctrl, cam_data, window);
	boolean_radio_item(&radio_grp, vbox, &(*pf), 1, ctl_key, ctl_val, qctrl, cam_data, window);
    }
    else if (qctrl->type == V4L2_CTRL_TYPE_MENU)
    {
	tmp = ctlNode->sub_list_head;
	i = 0;

	while(tmp != NULL)
	{
	    menu_radio_item(&radio_grp, vbox, &(*pf), i, ctl_key, ctl_val, tmp, qctrl, cam_data, window);
	    tmp = tmp->next;
	    i++;
	}
    }

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER (frame), vbox);
    gtk_grid_attach(GTK_GRID (grid), frame, 1, row, 1, 1);

    /* Set the camera for this control value and store for the session */
    ctrl_flag_ok(qctrl, radio_grp, ctl_key, ctl_val, cam_data, window);
	
    return;
}


/* Create a radio group items for a camera menu control */

void menu_radio_item(GtkWidget **radio_grp, 
		     GtkWidget *vbox,
		     PangoFontDescription **pf, 
		     int i, 
		     char *ctl_key,
		     long ctl_val,
		     struct v4l2_list *item,
		     struct v4l2_queryctrl *qctrl, 
		     CamData *cam_data,
		     GtkWidget *window)
{
    struct v4l2_querymenu *qmenu;
    GtkWidget *radio;
    int handler_id;

    qmenu = (struct v4l2_querymenu *) item->v4l2_data;

    if (i == 0)
    {
	radio = gtk_radio_button_new_with_label (NULL, qmenu->name);
	*radio_grp = radio;
    }
    else
    {
	radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (*radio_grp), qmenu->name);
    }

    if (ctl_val == i)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);

    gtk_widget_override_font (radio, *pf);
    gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);

    handler_id = g_signal_connect(radio, "toggled", G_CALLBACK(OnSetCtrlRadio), (gpointer) cam_data);
    gtk_widget_set_name(radio, ctl_key);
    g_object_set_data (G_OBJECT (radio), "ui_window", window);
    g_object_set_data (G_OBJECT (radio), "control", qctrl);
    g_object_set_data (G_OBJECT (radio), "index", GINT_TO_POINTER (qmenu->index));
    g_object_set_data (G_OBJECT (radio), "resetable", GINT_TO_POINTER (TRUE));
    g_object_set_data (G_OBJECT (radio), "cb_handler", GINT_TO_POINTER (handler_id));
	
    return;
}


/* Create a radio group items for a camera boolean control */

void boolean_radio_item(GtkWidget **radio_grp, 
		        GtkWidget *vbox,
		        PangoFontDescription **pf, 
		        int i,
			char *ctl_key,
		        long ctl_val,
		        struct v4l2_queryctrl *qctrl, 
		        CamData *cam_data,
		        GtkWidget *window)
{
    GtkWidget *radio;
    int handler_id;

    if (i == 0)
    {
	radio = gtk_radio_button_new_with_label (NULL, "Off");
	*radio_grp = radio;
    }
    else
    {
	radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (*radio_grp), "On");
    }

    if (ctl_val == i)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);

    gtk_widget_override_font (radio, *pf);
    gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);

    handler_id = g_signal_connect(radio, "toggled", G_CALLBACK(OnSetCtrlRadio), (gpointer) cam_data);
    gtk_widget_set_name(radio, ctl_key);
    g_object_set_data (G_OBJECT (radio), "ui_window", window);
    g_object_set_data (G_OBJECT (radio), "control", qctrl);
    g_object_set_data (G_OBJECT (radio), "index", GINT_TO_POINTER (i));
    g_object_set_data (G_OBJECT (radio), "resetable", GINT_TO_POINTER (TRUE));
    g_object_set_data (G_OBJECT (radio), "cb_handler", GINT_TO_POINTER (handler_id));
	
    return;
}


/* Check for audio override */

void check_audio(struct v4l2_queryctrl *qctrl, long *ctl_val)
{
    char *p;

    if (qctrl->id != V4L2_CID_AUDIO_MUTE)
    	return;
	
    get_user_pref(AUDIO_MUTE, &p);
    *ctl_val = atol(p);

    return;
}


/* Create a label for a camera control */

GtkWidget * ctrl_label(char *nm, PangoFontDescription **pf, char *ctl_key, int row, GtkWidget *grid)
{
    GtkWidget *label;  
    char s[100];

    label = gtk_label_new(nm);
    pango_font_description_set_weight(*pf, PANGO_WEIGHT_BOLD);
    gtk_widget_override_font (label, *pf);
    gtk_grid_attach(GTK_GRID (grid), label, 0, row, 1, 1);
    gtk_widget_set_halign(GTK_WIDGET (label), GTK_ALIGN_END);
    gtk_widget_set_valign(GTK_WIDGET (label), GTK_ALIGN_START);
    gtk_widget_set_margin_end(GTK_WIDGET (label), 10);
    gtk_widget_set_margin_top(GTK_WIDGET (label), 10);
    gtk_widget_set_margin_bottom(GTK_WIDGET (label), 10);
    sprintf(s, "%s_lbl", ctl_key);
    gtk_widget_set_name(label, s);
	
    return label;
}


/* Check if the control may be changed */

int ctrl_flag_ok(struct v4l2_queryctrl *qctrl, GtkWidget *widget,
		 char *ctl_key, long ctl_val, CamData *cam_data, GtkWidget *window)
{
    char s[10];

    if (qctrl->flags & V4L2_CTRL_FLAG_INACTIVE)
    {
    	gtk_widget_set_sensitive (widget, FALSE);
    	sprintf(s, "%ld", ctl_val);
	set_session(ctl_key, s);
	return FALSE;
    }
    else
    {
	save_ctrl(qctrl, ctl_key, ctl_val, cam_data, window);
	return TRUE;
    }
}


/* Update the Frame Size (resolution) combobox */

void update_main_ui_res(MainUi *m_ui, CamData *cam_data)
{
    int hndlr_id;

    /* Disable the handler first and unblock when finished */
    hndlr_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (m_ui->cbox_res), "hndlr_id"));
    g_signal_handler_block (m_ui->cbox_res, hndlr_id);

    /* Clear the list */
    gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (m_ui->cbox_res));

    /* Rebuild the list */
    clrfmt_res_list(m_ui, cam_data);

    /* Re-enable callback */
    g_signal_handler_unblock (m_ui->cbox_res, hndlr_id);

    /* Cascade changes to Frame Rate */
    update_main_ui_fps(m_ui, cam_data);

    return;
}


/* Update the Frame Rate combobox */

void update_main_ui_fps(MainUi *m_ui, CamData *cam_data)
{
    int hndlr_id;

    /* Check the sensitivity */
    if (gtk_widget_get_sensitive (m_ui->cbox_fps) == FALSE)
    	return;

    /* Disable the handler first and unblock when finished */
    hndlr_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (m_ui->cbox_fps), "hndlr_id"));
    g_signal_handler_block (m_ui->cbox_fps, hndlr_id);

    /* Clear the list */
    gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (m_ui->cbox_fps));

    /* Rebuild the list */
    clrfmt_res_fps_list(m_ui, cam_data);

    /* Re-enable callback */
    g_signal_handler_unblock (m_ui->cbox_fps, hndlr_id);

    return;
}


/* Adjust the video window area */

void update_main_ui_video(long width, long height, MainUi *m_ui)
{
    gtk_widget_set_size_request (m_ui->video_window, width, height);

    if (width <= MAX_VWIDTH)
	gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (m_ui->scrollwin), width);

    if (height <= MAX_VHEIGHT)
	gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (m_ui->scrollwin), height);

    return;
}


/* Set the colour format combobox to the given fourcc code, no need to cascade changes to Screen Res or Frame Rate */

int update_main_ui_clrfmt(char *clrfmt, MainUi *m_ui)
{
    int hndlr_id, idx;
    char fourcc[5], tmp_fcc[5];
    char *p;
    struct v4l2_fmtdesc *vfmt;
    struct v4l2_list *fmt_node;
    CamData *cam_data;

    /* Check setting against negotiated */
    get_session(CLRFMT, &p);

    if (strcmp(p, clrfmt) == 0)
    	return TRUE;

    /* Disable the handler first and unblock when finished */
    hndlr_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (m_ui->cbox_clrfmt), "hndlr_id"));
    g_signal_handler_block (m_ui->cbox_clrfmt, hndlr_id);

    /* Set the list */
    cam_data = g_object_get_data (G_OBJECT(m_ui->window), "cam_data");
    fmt_node = cam_data->cam->fmt_head;
    idx = 0;

    while(fmt_node != NULL)
    {
    	vfmt = (struct v4l2_fmtdesc *) fmt_node->v4l2_data;
	pxl2fourcc(vfmt->pixelformat, tmp_fcc);
	swap_fourcc(tmp_fcc, fourcc);

printf("%s update_main_ui_clrfmt  fourcc: %s   clrfmt: %s\n", debug_hdr, fourcc, clrfmt); fflush(stdout);
	if (strcmp(fourcc, clrfmt) == 0)
	{
	    gtk_combo_box_set_active(GTK_COMBO_BOX (m_ui->cbox_clrfmt), idx);
	    set_session(CLRFMT, fourcc);
	    idx = -1;
	    break;
	}

    	fmt_node = fmt_node->next;
    	idx++;
    }

    if (idx != -1)				// No match found
    {
	log_msg("CAM0032", clrfmt, NULL, NULL);
	idx = FALSE;
    }
    else
    {	
    	idx = TRUE;
    }

    /* Re-enable callback */
    g_signal_handler_unblock (m_ui->cbox_clrfmt, hndlr_id);

    return idx;
}


/* Set the value of a scale widget */

void set_scale_val(GtkWidget *grid, char *nm, long val)
{
    GtkWidget *scale;
    char s[10];

    /* Find the scale widget */
    scale = find_widget_by_name(grid, nm);

    if (! scale)
    	return;

    gtk_range_set_value(GTK_RANGE (scale), val);
    sprintf(s, "%ld", val);
    set_session(nm, s);

    return;
}


/* Reset the control panel (new camera selected) */

int reset_cntl_panel(MainUi *m_ui, CamData *cam_data)
{
    GtkWidget *widget;
    int row;
    const gchar *widget_name;

    /* Remove all the old camera format and control widgets (plus labels) */
    GList *child_widgets = gtk_container_get_children(GTK_CONTAINER (m_ui->cntl_grid));

    child_widgets = g_list_last(child_widgets);

    while (child_widgets != NULL)
    {
	widget = child_widgets->data;
	widget_name = gtk_widget_get_name (widget);

	if ((widget_name != NULL) && (strcmp(widget_name, "GtkLabel") != 0))
	    gtk_widget_destroy (widget);

	child_widgets = g_list_previous(child_widgets);
    }

    g_list_free (child_widgets);

    /* Recreate the new control widgets */
    row = 2;
    video_settings(&row, cam_data, m_ui);
    exposure_settings(&row, cam_data, m_ui);

    gtk_widget_show_all(m_ui->window);

    return TRUE;
}


/* Load a list a available profiles */

void load_profiles(GtkWidget *cbox, char *active, int hndlr_id, int clear_indi)
{
    int i, init;
    char *nm;
    char s[20];

    /* Disable callback if any */
    if (hndlr_id != 0)
    	g_signal_handler_block (cbox, hndlr_id);

    /* Clear list if necessary */
    if (clear_indi == TRUE)
    	gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (cbox));

    /* Fixed entry - None */
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (cbox), "0", PRF_NONE);
    gtk_combo_box_set_active(GTK_COMBO_BOX (cbox), 0);

    /* Iterate through each of the saved profiles */
    i = 1;
    init = TRUE;

    while((nm =  get_profile_name(init)) != (char *) NULL)
    {
	sprintf(s, "%d", i);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (cbox), s, nm);
	
	if (active != NULL)
	    if (strcmp(active, nm) == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX (cbox), i);

    	i++;
    	init = FALSE;
    }

    /* Enable callbacks */
    if (hndlr_id != 0)
	g_signal_handler_unblock (cbox, hndlr_id);

    return;
}


/* Return to normal viewing from Night View */

void set_night_view_off(MainUi *m_ui)
{
    GtkAllocation allocation;

    /* Initial */
    m_ui->night_view = FALSE;
    GdkWindow *window = gtk_widget_get_window (m_ui->window);
    cairo_t *cr;

    /* Disconnect callback */
    g_signal_handler_disconnect (m_ui->window, (gulong) m_ui->nvexp_hndlr_id);

    /* Normal view */
    cr = gdk_cairo_create (window);
    gtk_widget_draw (m_ui->window, cr);
    cairo_destroy (cr);

    return;
}


/* Set up for Night View */

void set_night_view_on(MainUi *m_ui)
{
    GtkAllocation allocation;

    /* Initial */
    m_ui->night_view = TRUE;
    GdkWindow *window = gtk_widget_get_window (m_ui->window);
    cairo_t *cr;

    /* Main window night vision setup */
    gtk_widget_get_allocation (m_ui->window, &allocation);
    cr = gdk_cairo_create (window);
    cairo_set_source_rgba (cr, NIGHT.red, NIGHT.green, NIGHT.blue, NIGHT.alpha);
    cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    /* Night view */
    cairo_paint (cr);
    cairo_destroy (cr);

    m_ui->nvexp_hndlr_id = g_signal_connect (m_ui->window, "draw", G_CALLBACK (OnNvExpose), m_ui);

    return;
}


/* Debug widget container */

GtkWidget * debug_cntr(GtkWidget *cntr)
{
    const gchar *widget_name;
    GtkWidget *widget;
    GtkWidgetPath *w_path;

    if (! GTK_IS_CONTAINER(cntr))
    {
	log_msg("SYS9011", "btn children", NULL, NULL);
    	return NULL;
    }

    widget_name = gtk_widget_get_name (cntr);
    printf("%s widget structure for %s\n", debug_hdr, widget_name);

    GList *child_widgets = gtk_container_get_children(GTK_CONTAINER (cntr));
    //printf("%s \tno of children %d\n", debug_hdr, g_list_length(child_widgets));

    child_widgets = g_list_first(child_widgets);

    while (child_widgets != NULL)
    {
	widget = child_widgets->data;
	widget_name = gtk_widget_get_name (widget);
	printf("%s \tname %s\n", debug_hdr, widget_name);

	w_path = gtk_container_get_path_for_child (GTK_CONTAINER (cntr), widget);
	printf("%s \tpath %s\n", debug_hdr, gtk_widget_path_to_string (w_path));

	if (GTK_IS_CONTAINER(widget))
	    debug_cntr(widget);

	if (GTK_IS_LABEL (widget))
	    break;

	child_widgets = g_list_next(child_widgets);
    }

    g_list_free (child_widgets);

    return widget;
}
