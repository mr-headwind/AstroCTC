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
** Description:	Module for (Main) Callback functions
**
** Author:	Anthony Buckley
**
** History
**	01-Dec-2013	Initial code
*/


/* Includes */

#include <stdlib.h>  
#include <string.h>  
#include <libgen.h>  
#include <gtk/gtk.h>  
#include <gst/gst.h>  
#include <linux/videodev2.h>
#include <cairo/cairo.h>
#include <cairo/cairo-gobject.h>
#include <session.h>
#include <main.h>
#include <cam.h>
#include <defs.h>


/* Defines */

#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))


/* Prototypes */

void OnCameraSel(GtkWidget*, gpointer);
void OnCamDetail(GtkWidget*, gpointer);
void OnCamDefault(GtkWidget*, gpointer);
void OnCamRestart(GtkWidget*, gpointer);
void OnCamScan(GtkWidget*, gpointer);
void OnStartCapUi(GtkWidget*, gpointer);
void OnStartCap(GtkWidget*, gpointer);
void OnStopCap(GtkWidget*, gpointer);
void OnSnapUi(GtkWidget*, gpointer);
void OnSnapShot(GtkWidget*, gpointer);
void OnPauseCap(GtkWidget*, gpointer);
void OnPrefs(GtkWidget*, gpointer);
void OnNightVision(GtkWidget*, gpointer);
void OnReticule(GtkWidget*, gpointer);
void OnAbout(GtkWidget*, gpointer);
void OnViewLog(GtkWidget*, gpointer);
void OnSetProfile(GtkWidget*, gpointer);
void OnManagePreset(GtkWidget*, gpointer);
void OnSavePreset(GtkWidget*, gpointer);
void OnSetClrFmt(GtkWidget*, gpointer);
void OnSetRes(GtkWidget*, gpointer);
void OnSetFps(GtkWidget*, gpointer);
void OnSetCtrl(GtkRange *, GtkScrollType, gdouble, gpointer);
void OnSetCtrlCbox(GtkWidget *, gpointer);
void OnSetCtrlRadio(GtkToggleButton *, gpointer);
void OnCtrlDefVal(GtkWidget*, gpointer);
void OnCtrlReset(GtkWidget *, gpointer);
void OnOtherCtrl(GtkWidget*, gpointer);
void OnRealise(GtkWidget*, CamData*);
void OnQuit(GtkWidget*, gpointer);
gboolean OnExpose (GtkWidget*, cairo_t *, gpointer);
gboolean OnNvExpose (GtkWidget *, cairo_t *, gpointer);
GstPadProbeReturn OnPadProbe (GstPad *, GstPadProbeInfo *, gpointer);
void OnPrepReticule (GstElement *, GstCaps *, gpointer);
void OnDrawReticule (GstElement *, cairo_t *, guint64, guint64, gpointer);


extern int gst_view(CamData *, MainUi *);
extern int view_clear_pipeline(CamData *, MainUi *);
extern int set_session(char*, char*);
extern void get_session(char*, char**);
extern int camera_setup(camera_t *, GtkWidget *);
extern char *camera_info_file(camera_t *);
extern void clear_camera_list(CamData *);
extern void free_cam_data(struct v4l2_list *);
extern GtkWidget* cam_info_main(CamData *);
extern void free_window_reg();
extern void close_open_ui();
extern int is_ui_reg(char *, int);
extern void save_ctrl(struct v4l2_queryctrl *, char *, long, CamData *, GtkWidget *);
extern int cam_ctrl_reset(CamData *, GtkWidget *, char, GtkWidget *);
extern int cam_defaults(camera_t *, MainUi *, struct v4l2_list *);
extern int cam_fmt_read(CamData *, struct v4l2_format *, struct v4l2_fmtdesc **, int);
extern int set_cam_fmt(camera_t *, struct v4l2_format *, GtkWidget *);
extern int cam_fps_update(CamData *, char *);
extern int cam_fps_read(CamData *, char *, struct v4l2_streamparm *, struct v4l2_frmivalenum **, int);
extern int set_cam_streamparm(camera_t *, struct v4l2_streamparm *, GtkWidget *);
extern pixelfmt fourcc2pxl(char *);
extern void res_to_long(char *, long *, long *);
extern int calc_fps(pixelfmt, pixelfmt);
extern void log_msg(char*, char*, char*, GtkWidget*);
extern void app_msg(char*, char*, GtkWidget*);
extern char * log_name();
extern GtkWidget* view_file_main(char  *);
extern struct v4l2_list * find_frm(CamData *, struct v4l2_list *, struct v4l2_frmsizeenum **, char *);
extern void update_main_ui_res(MainUi *, CamData *);
extern void update_main_ui_fps(MainUi *, CamData *);
extern void update_main_ui_video(long, long, MainUi *);
extern int reset_cntl_panel(MainUi *, CamData *);
extern int cam_set_state(CamData *, GstState, GtkWidget *);
extern int capture_main(GtkWidget *);
extern int snap_ui_main(GtkWidget *);
extern int snap_control(CamData *, MainUi *, int, int, int);
extern int user_prefs_main(GtkWidget *);
extern int gst_capture(CamData *, MainUi *, int, int);
extern int val_str2numb(char *, int *, char *, GtkWidget *);
extern int set_eos(MainUi *);
extern int cancel_snapshot(MainUi *);
extern int profile_main(GtkWidget *, gchar *);
extern void load_profile(char *);
extern void set_night_view_on(MainUi *);
extern void set_night_view_off(MainUi *);
extern void set_reticule(MainUi *, CamData *);
extern int prepare_reticule(MainUi *, CamData *);
extern int remove_reticule(MainUi *, CamData *);
extern int about_main(GtkWidget *);
extern void cam_auto_reset(GtkWidget *, struct v4l2_queryctrl *, long, camera_t *, GtkWidget *);
extern void check_session_save(camera_t *);
extern void save_profile(char *);
extern int other_ctrl_main(GtkWidget *, CamData *);
extern void delete_menu_items(GtkWidget *, char *);
extern void add_camera_list(GtkWidget**, MainUi *, CamData *);
extern int snap_mutex_lock();	
extern int snap_mutex_unlock();	
/*
extern void lock_imgbuf();
extern void unlock_imgbuf();
*/


/* Globals */

static const char *debug_hdr = "DEBUG-callbacks.c ";
extern guintptr video_window_handle;


/* Callbacks */


/* Callback - Connect a cammera */

void OnCameraSel(GtkWidget *cam_sel, gpointer user_data)
{  
    MainUi *m_ui;
    GstElement *source, *filter;
    CamData *cam_data;
    char *cam_nm, *cam_dev;
    GstCaps *caps;
    camera_t *cam;

    /* Get new camera selection */
    cam = g_object_get_data (G_OBJECT(cam_sel), "camera");
    m_ui = g_object_get_data (G_OBJECT(cam_sel), "ui");
    cam_nm = cam->vcaps.card;
    cam_dev = cam->video_dev;

    /* Get current camera and pipeline details */
    cam_data = (CamData *) user_data;

    /* Display a message if no change */
    if ((strcmp(cam_data->current_dev, cam_dev)) == 0)
    {
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (m_ui->window),
						    GTK_DIALOG_MODAL,
						    GTK_MESSAGE_ERROR,
						    GTK_BUTTONS_CLOSE,
						    "'%s' is already currently showing.",
						    cam_nm);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

    	return;
    }

    /* Wipe the pipeline (free all the resources) */
    if (view_clear_pipeline(cam_data, m_ui) == FALSE)
        return;

    /* Set new camera */
    strcpy(cam_data->current_cam, cam_nm);
    strcpy(cam_data->current_dev, cam_dev);
    cam_data->cam = cam;
    camera_setup(cam, m_ui->window);
    set_session(CAMERA, cam_data->current_cam);

    /* New camera selection may require resetting the controls */
    close_ui(OTHER_CTRL_UI); 
    reset_cntl_panel(m_ui, cam_data);

    /* Rebuild the pipeline */
    gst_view(cam_data, m_ui);

    return;
}  


/* Callback - Open a screen to show current cammera details */

void OnCamDetail(GtkWidget *cam_detail, gpointer user_data)
{  
    CamData *cam_data;

    /* Check if already open */
    if (is_ui_reg(CAM_INFO_UI, TRUE))
    	return;

    /* Open */
    cam_data = (CamData *) user_data;
    cam_info_main(cam_data);

    return;
}  


/* Callback - Set all camera values back to the default */

void OnCamDefault(GtkWidget *def_val_btn, gpointer user_data)
{  
    GtkWidget *window;
    CamData *cam_data;
    MainUi *m_ui;
    
    /* Close 'Other Control' window if open */
    close_ui(OTHER_CTRL_UI); 

    /* Get data */
    window = (GtkWidget *) user_data;
    cam_data = g_object_get_data (G_OBJECT(window), "cam_data");
    m_ui = g_object_get_data (G_OBJECT(window), "ui");

    cam_defaults(cam_data->cam, m_ui, cam_data->cam->ctl_head);
    cam_defaults(cam_data->cam, m_ui, cam_data->cam->pctl_head);

    return;
}  


/* Callback - Restart the video from the camera */

void OnCamRestart(GtkWidget *cam_detail, gpointer user_data)
{  
    CamData *cam_data;
    MainUi *m_ui;
    GtkWidget *window;

    /* Get data */
    window = (GtkWidget *) user_data;
    cam_data = g_object_get_data (G_OBJECT(window), "cam_data");
    m_ui = g_object_get_data (G_OBJECT(window), "ui");

    /* Wipe the pipeline (free all the resources) and restart */
    if (view_clear_pipeline(cam_data, m_ui) == FALSE)
        return;

    gst_view(cam_data, m_ui);

    return;
}  


/* Callback - Rescan for cammeras */

void OnCamScan(GtkWidget *cam_scan, gpointer user_data)
{  
    MainUi *m_ui;
    CamData *cam_data;
    GtkWidget *window;

    /* Get data */
    window = (GtkWidget *) user_data;
    cam_data = g_object_get_data (G_OBJECT(window), "cam_data");
    m_ui = g_object_get_data (G_OBJECT(window), "ui");

    /* Close the 'More Settings' ui if open */
    close_ui(OTHER_CTRL_UI); 

    /* Remove the current menu items */
    delete_menu_items(m_ui->cam_menu, "cam_");

    /* Wipe the pipeline (free all the resources) */
    if (cam_data->camlist != NULL)
    {
    	if (view_clear_pipeline(cam_data, m_ui) == FALSE)
	    return;

	clear_camera_list(cam_data);
	memset(cam_data, 0, sizeof (CamData));
    }

    /* Rebuild everything */
    add_camera_list(&(m_ui->cam_menu), m_ui, cam_data);

    if (cam_data->cam == NULL)
    	return;

    reset_cntl_panel(m_ui, cam_data);

    /* Rebuild the pipeline */
    gst_view(cam_data, m_ui);

    return;
}  


/* Callback - Open video capture screen */

void OnStartCapUi(GtkWidget *capture_btn, gpointer user_data)
{  
    GtkWidget *window;

    /* Get data */
    window = (GtkWidget *) user_data;

    /* Check if already open */
    if (is_ui_reg(CAPTURE_UI, TRUE))
    	return;

    /* Open */
    capture_main(window);

    return;
}  


/* Callback - Start video capture */

void OnStartCap(GtkWidget *capture_btn, gpointer user_data)
{  
    CamData *cam_data;
    MainUi *m_ui;
    GtkWidget *window;
    gchar *dur_str;
    int duration;

    /* Get data */
    window = (GtkWidget *) user_data;
    cam_data = g_object_get_data (G_OBJECT(window), "cam_data");
    m_ui = g_object_get_data (G_OBJECT(window), "ui");

    dur_str = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (m_ui->cbox_dur));

    if (val_str2numb(dur_str, &duration, "Duration", window) == FALSE)
    {
	g_free(dur_str);
	return;
    }

    g_free(dur_str);

    /* Initiate capture */
    gst_capture(cam_data, m_ui, duration, 0);

    return;
}  


/* Callback -  Stop video capture */

void OnStopCap(GtkWidget *capture_btn, gpointer user_data)
{  
    MainUi *m_ui;
    CamData *cam_data;
    GtkWidget *window;

    /* Get data */
    window = (GtkWidget *) user_data;
    m_ui = g_object_get_data (G_OBJECT(window), "ui");
    cam_data = g_object_get_data (G_OBJECT(window), "cam_data");

    /* Force End-Of-Stream onto the pipeline (bus watch will handle terminate and restart) */
    if (cam_data->mode == CAM_MODE_CAPT)
	set_eos(m_ui);
    else
    	cancel_snapshot(m_ui);

    return;
}  


/* Callback -  Pause video capture */

void OnPauseCap(GtkWidget *capture_btn, gpointer user_data)
{  
    char s[100];
    CamData *cam_data;
    MainUi *m_ui;
    GtkWidget *window;

    /* Get data */
    window = (GtkWidget *) user_data;
    cam_data = g_object_get_data (G_OBJECT(window), "cam_data");
    m_ui = g_object_get_data (G_OBJECT(window), "ui");

    /* If camera is playing, pause it, otherwise un-pause it and continue */
    if (cam_data->state == GST_STATE_PAUSED)
    {
	cam_set_state(cam_data, GST_STATE_PLAYING, window);
	sprintf(s, "Camera %s (%s) capturing resumed ", cam_data->current_cam, cam_data->current_dev);
	gtk_label_set_text (GTK_LABEL (m_ui->status_info), s);
    }
    else
    {
	cam_set_state(cam_data, GST_STATE_PAUSED, window);
	sprintf(s, "Camera %s (%s) capturing paused ", cam_data->current_cam, cam_data->current_dev);
    }

    gtk_label_set_text (GTK_LABEL (m_ui->status_info), s);

    return;
}  


/* Callback - Open snapshot screen */

void OnSnapUi(GtkWidget *snap_btn, gpointer user_data)
{  
    GtkWidget *window;

    /* Get data */
    window = (GtkWidget *) user_data;

    /* Check if already open */
    if (is_ui_reg(SNAP_UI, TRUE))
    	return;

    /* Open */
    snap_ui_main(window);

    return;
}  


/* Callback -  Image snapshot */

void OnSnapShot(GtkWidget *capture_btn, gpointer user_data)
{  
    GtkWidget *window;
    CamData *cam_data;
    MainUi *m_ui;
    int idx;

    /* Get data */
    window = (GtkWidget *) user_data;
    cam_data = (CamData *) g_object_get_data (G_OBJECT(window), "cam_data");
    m_ui = (MainUi *) g_object_get_data (G_OBJECT(window), "ui");

    /* Check number of snapshots requested */
    idx = gtk_combo_box_get_active(GTK_COMBO_BOX (m_ui->cbox_seq));

    if (idx == 0 || idx == -1)
    	idx = 1;
    else
	idx *= 5;

    /* Snapshot */
    snap_control(cam_data, m_ui, idx, -1, 0);

    return;
}  


/* Callback - Set up preferences */

void OnPrefs(GtkWidget *menu_item, gpointer user_data)
{  
    GtkWidget *window;

    /* Get data */
    window = (GtkWidget *) user_data;

    /* Check if already open */
    if (is_ui_reg(USER_PREFS_UI, TRUE))
    	return;

    /* Open */
    user_prefs_main(window);

    return;
}  


/* Callback - Reset Night Vision (only applies when switched on) */

gboolean OnNvExpose(GtkWidget *widget, cairo_t *crx, gpointer user_data)
{  
    MainUi *m_ui;
    GtkAllocation allocation;

    /* Get data */
    m_ui = (MainUi *) user_data;

    /* Initial */
    g_signal_handler_block (m_ui->window, m_ui->nvexp_hndlr_id);
    GdkWindow *window = gtk_widget_get_window (m_ui->window);
    cairo_t *cr;

    gtk_widget_get_allocation (m_ui->window, &allocation);
    cr = gdk_cairo_create (window);

    /* Redraw to original */
    gtk_widget_draw (m_ui->window, cr);

    /* Restore night vision */
    cairo_set_source_rgba (cr, NIGHT.red, NIGHT.green, NIGHT.blue, NIGHT.alpha);
    cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_paint (cr);
    cairo_destroy (cr);

    /* Monitor for more exposures */
    g_signal_handler_unblock (m_ui->window, m_ui->nvexp_hndlr_id);

    return TRUE;
}


/* Callback - Set Night Vision by shading everything red */

void OnNightVision(GtkWidget *menu_item, gpointer user_data)
{  
    MainUi *m_ui;

    /* Get data */
    m_ui = (MainUi *) user_data;

    /* Toggle on or off */
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menu_item)) == TRUE)
    	set_night_view_on(m_ui);
    else
    	set_night_view_off(m_ui);

    return;
}  


/* Callback - Place a reticule over the camera display */

void OnReticule(GtkWidget *menu_item, gpointer user_data)
{  
    MainUi *m_ui;
    CamData *cam_data;

    /* Get data */
    m_ui = (MainUi *) user_data;
    cam_data = g_object_get_data (G_OBJECT(m_ui->window), "cam_data");

    /* Toggle on or off */
    set_reticule(m_ui, cam_data);

    return;
}  


/* Callback - Show About details */

void OnAbout(GtkWidget *menu_item, gpointer user_data)
{  
    MainUi *m_ui;

    /* Check if already open */
    if (is_ui_reg(ABOUT_UI, TRUE))
    	return;

    /* Get data */
    m_ui = (MainUi *) user_data;

    /* Open */
    about_main(m_ui->window);

    return;
}  


/* Callback - View Log File details */

void OnViewLog(GtkWidget *view_log, gpointer user_data)
{  
    GtkWidget *window;
    char *log_fn;

    /* Check if already open */
    if (is_ui_reg(VIEW_FILE_UI, TRUE))
    	return;

    /* Open */
    log_fn = log_name();
    window = (GtkWidget *) user_data;

    if (view_file_main(log_fn) == NULL)
    	log_msg("SYS9010", log_fn, "SYS9010", window);

    return;
}  


/* Callback - Load profile settings */

void OnSetProfile(GtkWidget *cbox, gpointer user_data)
{  
    GtkWidget *window;
    MainUi *m_ui;
    CamData *cam_data;
    gchar *cur_profile;

    /* Get data */
    window = (GtkWidget *) user_data;
    m_ui = (MainUi *) g_object_get_data (G_OBJECT(window), "ui");
    cam_data = g_object_get_data (G_OBJECT(window), "cam_data");
    cur_profile = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (m_ui->cbox_profile));

    /* Close 'Other Control' window if open */
    close_ui(OTHER_CTRL_UI); 

    /* No Profile */
    if (strcmp(cur_profile, PRF_NONE) == 0)
    {
	cam_ctrl_reset(cam_data, m_ui->cntl_grid, 'd', m_ui->window);
    	return;
    }

    /* Wipe the pipeline (free all the resources) */
    if (view_clear_pipeline(cam_data, m_ui) == FALSE)
        return;
   
    /* Load the preset profile and reset the window */
    load_profile(cur_profile);
    reset_cntl_panel(m_ui, cam_data);

    if ((strcmp(cur_profile, LAST_SESSION) == 0) || (strcmp(cur_profile, PRF_NONE) == 0))
    	gtk_widget_set_sensitive (m_ui->save_profile_btn, FALSE);
    else
    	gtk_widget_set_sensitive (m_ui->save_profile_btn, TRUE);

    g_free(cur_profile);

    /* Rebuild the pipeline */
    gst_view(cam_data, m_ui);

    return;
}  


/* Callback - Function to manage preset capturing profiles */

void OnManagePreset(GtkWidget *profile_btn, gpointer user_data)
{  
    GtkWidget *window;
    MainUi *m_ui;
    gchar *cur_profile;

    /* Check if already open */
    if (is_ui_reg(PROFILE_UI, TRUE))
    	return;

    /* Get data */
    window = (GtkWidget *) user_data;
    m_ui = (MainUi *) g_object_get_data (G_OBJECT(window), "ui");
    cur_profile = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (m_ui->cbox_profile));

    /* Open */
    profile_main(window, cur_profile);

    return;
}  


/* Callback - Function to save current profile */

void OnSavePreset(GtkWidget *save_profile_btn, gpointer user_data)
{  
    MainUi *m_ui;
    gchar *nm;

    /* Get data */
    m_ui = (MainUi *) user_data;

    nm = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (m_ui->cbox_profile));

    /* Shouldn't happen, but ... */
    if ((strcmp(nm, LAST_SESSION) != 0) && (strcmp(nm, PRF_NONE) != 0))
	save_profile((char *) nm);

    g_free(nm);

    return;
}  


/* Callback - Set Colour and compression video format */

void OnSetClrFmt(GtkWidget *cbox, gpointer user_data)
{  
    MainUi *m_ui;
    CamData *cam_data;
    gchar *fourcc_sel;
    char *p;
    char fourcc_save[10];
    struct v4l2_fmtdesc *vfmt; 
    struct v4l2_format cfmt;

    /* Get Data */
    if (gtk_combo_box_get_active (GTK_COMBO_BOX (cbox)) == -1)
    	return;

    fourcc_sel = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cbox));
    m_ui = g_object_get_data (G_OBJECT(cbox), "ui");
    cam_data = (CamData *) user_data;

    /* Save the current format before setting new one in case update fails */
    get_session(CLRFMT, &p);
    strcpy(fourcc_save, p);
    set_session(CLRFMT, fourcc_sel);

    /* Retrieve and check the format */
    if (cam_fmt_read(cam_data, &cfmt, &vfmt, FALSE) == FALSE)
    {
	g_free(fourcc_sel); 
    	app_msg("CAM0013", NULL, m_ui->window);
    	return;
    }

    /* Set the format */
    if (vfmt != NULL)
	cfmt.fmt.pix.pixelformat = vfmt->pixelformat;
    else
	cfmt.fmt.pix.pixelformat = fourcc2pxl(fourcc_sel);

    /* Wipe the pipeline (free all the resources) */
    if (view_clear_pipeline(cam_data, m_ui) == FALSE)
        return;

    /* Reset the format and restart the pipeline */
    if (set_cam_fmt(cam_data->cam, &cfmt, m_ui->window) == TRUE)
	update_main_ui_res(m_ui, cam_data);
    else
	set_session(CLRFMT, fourcc_save);

    /* Rebuild the pipeline */
    gst_view(cam_data, m_ui);
    g_free(fourcc_sel); 

    return;
}  


/* Callback - Set the image resolution */

void OnSetRes(GtkWidget *cbox, gpointer user_data)
{  
    MainUi *m_ui;
    CamData *cam_data;
    gchar *res_sel;
    struct v4l2_format rfmt;
    struct v4l2_fmtdesc *vfmt;
    long width, height;

    /* Get Data */
    if (gtk_combo_box_get_active (GTK_COMBO_BOX (cbox)) == -1)
    	return;

    res_sel = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cbox));
    m_ui = g_object_get_data (G_OBJECT(cbox), "ui");
    cam_data = (CamData *) user_data;

    /* The signal may be in consequence of another action, if so nothing is required */
    if (! cam_data->pipeline)
    	return;

    /* Retrieve and check the format */
    if (cam_fmt_read(cam_data, &rfmt, &vfmt, FALSE) == FALSE)
    {
	g_free(res_sel);
    	app_msg("CAM0013", NULL, m_ui->window);
    	return;
    }

    /* Set the format */
    res_to_long(res_sel, &width, &height);
    rfmt.fmt.pix.width = width;
    rfmt.fmt.pix.height = height;

    /* Wipe the pipeline (free all the resources) */
    if (view_clear_pipeline(cam_data, m_ui) == FALSE)
        return;
    
    // Reset the format and restart the pipeline
    // May need to adjust video window area accordingly - don't go larger than standard
    // Need to build a new fps list
    if (set_cam_fmt(cam_data->cam, &rfmt, m_ui->window) == TRUE)
    {
	set_session(RESOLUTION, res_sel);
	update_main_ui_video(width, height, m_ui);
	update_main_ui_fps(m_ui, cam_data);
    }

    /* Rebuild the pipeline */
    gst_view(cam_data, m_ui);
    g_free(res_sel);

    return;
}  


/* Callback - Set the frame rate */

void OnSetFps(GtkWidget *cbox, gpointer user_data)
{  
    MainUi *m_ui;
    CamData *cam_data;
    char s[20];
    gchar *fps_sel;
    struct v4l2_fmtdesc *vfmt; 
    struct v4l2_frmsizeenum *vfrm;
    struct v4l2_frmivalenum *vfrmival;
    struct v4l2_list *v_node;
    struct v4l2_streamparm s_parm;
    char *p;

    /* Get Data */
    if (gtk_combo_box_get_active (GTK_COMBO_BOX (cbox)) == -1)
    	return;

    fps_sel = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cbox));
    m_ui = g_object_get_data (G_OBJECT(cbox), "ui");
    cam_data = (CamData *) user_data;

    /* The signal may be in consequence of another action, if so nothing is required */
    if (! cam_data->pipeline)
    	return;

    /* Retrieve and check the format */
    if (cam_fps_read(cam_data, fps_sel, &s_parm, &vfrmival, FALSE) == FALSE)
    {
	g_free(fps_sel);
    	app_msg("CAM0015", NULL, m_ui->window);
    	return;
    }

    /* Set new values */
    if (vfrmival != NULL)
    {
    	s_parm.parm.capture.timeperframe.numerator = vfrmival->discrete.numerator;	// Stepwise ?
	s_parm.parm.capture.timeperframe.denominator = vfrmival->discrete.denominator;
    }
    else
    {
	s_parm.parm.capture.timeperframe.numerator = 1;
	s_parm.parm.capture.timeperframe.denominator = atol(fps_sel);
    }

    /* Wipe the pipeline (free all the resources) */
    if (view_clear_pipeline(cam_data, m_ui) == FALSE)
        return;
    
    /* Reset the format and restart the pipeline */
    if (set_cam_streamparm(cam_data->cam, &s_parm, m_ui->window) == TRUE)
	set_session(FPS, fps_sel);

    /* Rebuild the pipeline */
    gst_view(cam_data, m_ui);
    g_free(fps_sel);

    return;
}  


/* Callback - Set a control value */

void OnSetCtrl(GtkRange *slider, GtkScrollType scroll, gdouble val, gpointer user_data)
{  
    GtkWidget *window;
    CamData *cam_data;
    struct v4l2_queryctrl *qctrl;
    long ival;
    char ctl_key[10];

    /* Update the slider and get required objects */
    ival = round(val);
    gtk_range_set_value(GTK_RANGE (slider), (gdouble) ival);

    qctrl = g_object_get_data (G_OBJECT(slider), "control");
    window = g_object_get_data (G_OBJECT(slider), "ui_window");
    cam_data = (CamData *) user_data;

    /* Set the control */
    sprintf(ctl_key, "ctl-%d", qctrl->id - V4L2_CID_BASE);
    save_ctrl(qctrl, ctl_key, ival, cam_data, window);

    return;
}  


/* Callback - Set a menu control (list) value */

void OnSetCtrlCbox(GtkWidget *cbox, gpointer user_data)
{  
    GtkWidget *window;
    CamData *cam_data;
    struct v4l2_queryctrl *qctrl;
    long ival;
    char ctl_key[10];

    /* Get data */
    cam_data = (CamData *) user_data;
    qctrl = g_object_get_data (G_OBJECT(cbox), "control");
    window = g_object_get_data (G_OBJECT(cbox), "ui_window");
    ival = gtk_combo_box_get_active (GTK_COMBO_BOX (cbox));

    /* Set the control */
    sprintf(ctl_key, "ctl-%d", qctrl->id - V4L2_CID_BASE);
    save_ctrl(qctrl, ctl_key, ival, cam_data, window);

    return;
}  


/* Callback - Set a menu control (radio) value */

void OnSetCtrlRadio(GtkToggleButton *radio_btn, gpointer user_data)
{  
    GtkWidget *window;
    CamData *cam_data;
    struct v4l2_queryctrl *qctrl;
    long ival;
    char ctl_key[10];

    /* Get data */
    cam_data = (CamData *) user_data;
    qctrl = g_object_get_data (G_OBJECT(radio_btn), "control");
    window = g_object_get_data (G_OBJECT(radio_btn), "ui_window");

    /* Ignore if not active */
    if (! gtk_toggle_button_get_active(radio_btn))
	return;

    /* Set the control and check for impact on other controls */
    ival = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(radio_btn), "index"));
    sprintf(ctl_key, "ctl-%d", qctrl->id - V4L2_CID_BASE);
    save_ctrl(qctrl, ctl_key, ival, cam_data, window);
    cam_auto_reset(GTK_WIDGET (radio_btn), qctrl, ival, cam_data->cam, window);

    /*
    GSList * grp_list = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio_btn));
    GtkToggleButton *tmp = NULL;

    while(grp_list)
    {
	tmp = grp_list->data;

	if (gtk_toggle_button_get_active(tmp))
	    break;

	grp_list = grp_list->next;
        tmp = NULL;
    }

    /* Set the control and check for impact on other controls **
    if (tmp != NULL)
    {
	ival = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(tmp), "index"));
	sprintf(ctl_key, "ctl-%d", qctrl->id - V4L2_CID_BASE);
	save_ctrl(qctrl, ctl_key, ival, cam_data, window);
	cam_auto_reset(GTK_WIDGET (tmp), qctrl, ival, cam_data->cam, window);
    }
    */

    return;
}  


/* Callback - Set all control values back to the default */

void OnCtrlDefVal(GtkWidget *def_val_btn, gpointer user_data)
{  
    CamData *cam_data;
    MainUi *m_ui;

    /* Get data */
    cam_data = (CamData *) user_data;
    m_ui = g_object_get_data (G_OBJECT(def_val_btn), "ui");

    cam_ctrl_reset(cam_data, m_ui->cntl_grid, 'd', m_ui->window);

    return;
}  


/* Callback - Set all control values back to the original */

void OnCtrlReset(GtkWidget *reset_btn, gpointer user_data)
{  
    CamData *cam_data;
    MainUi *m_ui;

    /* Get data */
    cam_data = (CamData *) user_data;
    m_ui = g_object_get_data (G_OBJECT(reset_btn), "ui");

    cam_ctrl_reset(cam_data, m_ui->cntl_grid, 'l', m_ui->window);
    gtk_widget_set_sensitive (m_ui->save_profile_btn, FALSE);

    return;
}  


/* Callback - Call ui to set other control values */

void OnOtherCtrl(GtkWidget *oth_ctrl_btn, gpointer user_data)
{  
    CamData *cam_data;
    MainUi *m_ui;

    /* Get data */
    cam_data = (CamData *) user_data;
    m_ui = g_object_get_data (G_OBJECT(oth_ctrl_btn), "ui");

    other_ctrl_main(m_ui->window, cam_data);

    return;
}  


/* Callback - Gst probe for inserting a cairo object for reticule */

GstPadProbeReturn OnPadProbe (GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    MainUi *m_ui;
    CamData *cam_data;

    /* Get data */
    m_ui = (MainUi *) user_data;
    cam_data = g_object_get_data (G_OBJECT(m_ui->window), "cam_data");

    /* Prepare the pipeline for a reticule for insertion or removal */
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (m_ui->opt_ret)) == TRUE)
	prepare_reticule(m_ui, cam_data);
    else
	remove_reticule(m_ui, cam_data);

    /* Remove the probe */
    //gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

    return GST_PAD_PROBE_REMOVE;
}


/* Store the information from the caps that we are interested in */

void OnPrepReticule (GstElement *overlay, GstCaps *caps, gpointer user_data)
{
    MainUi *m_ui;
    CamData *cam_data;
    CairoOverlayState *state;

    /* Get data */
    m_ui = (MainUi *) user_data;
    cam_data = g_object_get_data (G_OBJECT(m_ui->window), "cam_data");
    state = cam_data->gst_objs.overlay_state;

    state->valid = gst_video_info_from_caps (&state->vinfo, caps);
}


/* Draw the overlay - a reticule */ 

void OnDrawReticule (GstElement *overlay, cairo_t *cr, guint64 timestamp, guint64 duration, gpointer user_data)
{
    MainUi *m_ui;
    CamData *cam_data;
    CairoOverlayState *state;
    int width, height;
    double xc, yc;

    /* Get data */
    m_ui = (MainUi *) user_data;
    cam_data = g_object_get_data (G_OBJECT(m_ui->window), "cam_data");
    state = cam_data->gst_objs.overlay_state;

    if (!state->valid)
	return;

    /* Draw the reticule */
    width = GST_VIDEO_INFO_WIDTH (&state->vinfo);
    height = GST_VIDEO_INFO_HEIGHT (&state->vinfo);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba (cr, 1, 0.2, 0.2, 0.6);
    cairo_set_line_width (cr, 2);

    xc = width / 2;
    yc = height / 2;

    cairo_move_to (cr, xc, 0);
    cairo_line_to (cr, xc, height);
    cairo_move_to (cr, 0, yc);
    cairo_line_to (cr, width, yc);
    cairo_arc (cr, xc, yc, 20, 0.0, 6.28); 	// NB. 6.28 = 360.0 * (3.14/180.0)
    cairo_arc (cr, xc, yc, 60, 0.0, 6.28); 	// NB. 6.28 = 360.0 * (3.14/180.0)
    cairo_stroke (cr);

    return;
}


// Callback - On create of physical window that will hold the video.
// At this point we can retrieve its handler (for X windowing system only)

void OnRealise(GtkWidget *widget, CamData *cam_data)
{
    GdkWindow *window = gtk_widget_get_window (widget);

    if (!gdk_window_ensure_native (window))
        g_error ("Couldn't create native window needed for GstVideoOverlay!");

  /* Retrieve window handler from GDK */
#if defined (GDK_WINDOWING_X11)
{
    gulong window_handle = GDK_WINDOW_XID (window);
    video_window_handle = window_handle;
}
#else
    g_error("Error: - This is purely for linux (X) based systems.\n");
#endif
}


// This function is called everytime the video window needs to be redrawn (due to damage/exposure,
// rescaling, etc). 

gboolean OnExpose (GtkWidget *draw_area, cairo_t *cr, gpointer user_data)
{
    MainUi *m_ui;
    CamData *cam_data;
    GtkAllocation allocation;
    GdkWindow *window;

    /* Get data */
    m_ui = (MainUi *) user_data;
    cam_data = g_object_get_data (G_OBJECT(m_ui->window), "cam_data");
    window = gtk_widget_get_window (draw_area);
    gtk_widget_get_allocation (draw_area, &allocation);

    if (cam_data->mode == CAM_MODE_SNAP)
    {
	if (snap_mutex_lock() == 0)
	{
	    if (cam_data->pixbuf != NULL)
	    {
		if (GDK_IS_PIXBUF (cam_data->pixbuf))
		{
		    gdk_cairo_set_source_pixbuf (cr, cam_data->pixbuf, 0, 0);
		    cairo_paint (cr);
		    g_object_unref(G_OBJECT(cam_data->pixbuf));
		    cam_data->pixbuf = NULL;
		}
	    }

	    snap_mutex_unlock();
	}
    }
    else if (cam_data->mode == CAM_MODE_NONE)
    {
	return FALSE;
    }
    else if (cam_data->state < GST_STATE_PAUSED)
    {
        //cairo_t *cr;

        /* Cairo is a 2D graphics library which we use here to clean the video window.
         * It is used by GStreamer for other reasons, so it will always be available to us. */
        //cr = gdk_cairo_create (window);
        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
        cairo_fill (cr);
        //cairo_destroy (cr);
    }

    return FALSE;
}


/* Callback - Quit */

void OnQuit(GtkWidget *window, gpointer user_data)
{  
    CamData *cam_data;

    /* Get window and current camera details */
    cam_data = g_object_get_data (G_OBJECT(window), "cam_data");

    if (cam_data->camlist != NULL)
    {
	/* Free resources */
	if (cam_data->pipeline)
	{
	    cam_set_state(cam_data, GST_STATE_NULL, window);
	    gst_object_unref (cam_data->pipeline);
	}

	/* Write the cam_info file */
	if (cam_data->cam != NULL)
	{
	    if (cam_data->cam->video_dev != cam_data->info_file)
		camera_info_file(cam_data->cam);
	}

	/* Check for unset session items that need to be saved */
	check_session_save(cam_data->cam);

	/* Clean up memory allocations on camera menu items */
	clear_camera_list(cam_data);
    }

    /* Close any open windows */
    close_open_ui();
    free_window_reg();

    /* Main quit */
    gtk_main_quit();

    return;
}  


/* CALLBACK other functions */

