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
** Description:
**  	Create the Camera Information user interface.
**  	Write the camera information to the 'cam_info' file if required.
**  	Display the contents of the 'cam_info' file.
**
** Author:	Anthony Buckley
**
** History
**	24-Apr-2014	Initial code
**      20-Nov-2020     Changes to move to css
**
*/



/* Defines */


/* Includes */
#include <stdlib.h>  
#include <string.h>  
#include <libgen.h>  
#include <gtk/gtk.h>  
#include <linux/videodev2.h>
#include <stdio.h>
#include <cam.h>
#include <defs.h>


/* Prototypes */

GtkWidget* cam_info_main(CamData *);
GtkWidget* cam_info_ui(CamData*);
int cam_info_init(CamData *);
GtkWidget* cam_info_ui_menu(GtkWidget *);
void OnCamInfoClose(GtkWidget*, gpointer);
char *camera_info_file(camera_t *);
int write_cam_caps(camera_t *);
int write_cam_controls(camera_t *);
int write_cam_menu_items(struct v4l2_list *);
int write_cam_formats(camera_t *);
int write_cam_frame_sizes(struct v4l2_list *);
int write_cam_frame_ints(struct v4l2_list *);
void write_cam_ctrl(struct v4l2_queryctrl *, int, int);
void write_cam_menu(struct v4l2_querymenu *);
void write_cam_fmt(struct v4l2_fmtdesc *, int);
void write_cam_res(struct v4l2_frmsizeenum *);
void write_cam_frmival(struct v4l2_frmivalenum *, struct v4l2_frmsizeenum *);
int read_cam_info(char *, int);
void pxl2fourcc(pixelfmt, char *);
pixelfmt fourcc2pxl(char *);
void res_to_long(char *, long *, long *);
int calc_fps(pixelfmt, pixelfmt);

extern char *camfile_name();
extern void register_window(GtkWidget *);
extern void deregister_window(GtkWidget *);


/* Globals */

static const char *debug_hdr = "DEBUG-camera_info_ui.c ";
static char *camfile;
static FILE *cf = NULL;


/* Camera Information display control */

GtkWidget* cam_info_main(CamData *cam_data)
{
    GtkWidget *cam_info_window;  

    /* Write the cam_info file if it has not been already done */
    if (! cam_info_init(cam_data))
    	return NULL;

    /* Create the interface */
    cam_info_window = cam_info_ui(cam_data);
    gtk_widget_show_all(cam_info_window);

    /* Register the window */
    register_window(cam_info_window);

    return cam_info_window;
}


/* Initial processing */

int cam_info_init(CamData *cam_data)
{
    /* Write the cam_info file if it has not been already done */
    if (cam_data->cam->video_dev == cam_data->info_file)
    	return TRUE;
    	 
    if ((camfile = camera_info_file(cam_data->cam)) == NULL)
	return FALSE;

    cam_data->info_file = cam_data->cam->video_dev;

    return TRUE;
}


/* Create the user interface and set the CallBacks */

GtkWidget* cam_info_ui(CamData *cam_data)
{  
    GtkWidget *cam_info_window;  
    GtkWidget *scrollwin;
    GtkWidget *mbox, *bbox, *lbox;  
    GtkWidget *label_t, *label_f;  
    GtkWidget *close_btn;  
    GtkWidget *txt_view;  
    GtkTextBuffer *txt_buffer;  
    GtkTextIter iter;
    GtkWidget *menu_bar;
    char buffer[500];
    int rc;
    int close_hndlr_id;

    /* Set up the UI window */
    cam_info_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);  
    gtk_window_set_title(GTK_WINDOW(cam_info_window), CAM_INFO_UI);
    gtk_window_set_position(GTK_WINDOW(cam_info_window), GTK_WIN_POS_NONE);
    gtk_window_set_default_size(GTK_WINDOW(cam_info_window), 400, 450);
    gtk_container_set_border_width(GTK_CONTAINER(cam_info_window), 10);

    /* Main view */
    mbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    /* Label for file name */
    label_t = gtk_label_new("Filename:");
    gtk_widget_set_name(label_t, "lbl_6");

    label_f = gtk_label_new(camfile);
    gtk_widget_set_name(label_f, "lbl_2");

    lbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start (GTK_BOX (lbox), label_t, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (lbox), label_f, FALSE, FALSE, 0);
    
    /* Scrolled window for TextView */
    scrollwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrollwin),
    				   GTK_POLICY_AUTOMATIC,
    				   GTK_POLICY_AUTOMATIC);

    /* Text area for camera information */
    txt_view = gtk_text_view_new();
    gtk_widget_set_name(txt_view, "txtview");
    gtk_container_add(GTK_CONTAINER(scrollwin), txt_view);
    txt_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (txt_view));
    gtk_widget_set_size_request (scrollwin, 500, 400);
    gtk_text_buffer_set_text (txt_buffer, "", -1);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (txt_view), FALSE);

    /* Populate the text area */
    rc = TRUE;

    while(rc != EOF)
    {
	rc = read_cam_info(buffer, sizeof(buffer));
	gtk_text_buffer_get_end_iter (txt_buffer, &iter);
	gtk_text_buffer_insert (txt_buffer, &iter, buffer, -1);
	gtk_text_iter_forward_to_end (&iter);
    }
    
    /* Close button */
    close_btn = gtk_button_new_with_label("  Close  ");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(OnCamInfoClose), cam_info_window);
    bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_end (GTK_BOX (bbox), close_btn, FALSE, FALSE, 0);
    gtk_widget_set_halign(GTK_WIDGET (bbox), GTK_ALIGN_CENTER);

    /* Menu */
    menu_bar = cam_info_ui_menu(cam_info_window);

    /* Combine everything onto the window */
    gtk_box_pack_start (GTK_BOX (mbox), menu_bar, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (mbox), lbox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (mbox), scrollwin, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (mbox), bbox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(cam_info_window), mbox);  

    /* Exit when window closed */
    close_hndlr_id = g_signal_connect(cam_info_window, "destroy", G_CALLBACK(OnCamInfoClose), cam_info_window);  
    g_object_set_data (G_OBJECT (cam_info_window), "close_hndlr_id", GINT_TO_POINTER (close_hndlr_id));

    return cam_info_window;
}


/*
** Menu function for camera information window
**
**  File
**   - Close
*/

GtkWidget* cam_info_ui_menu(GtkWidget *window)
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
    g_signal_connect_swapped (file_close, "activate", G_CALLBACK (OnCamInfoClose), window); 

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

void OnCamInfoClose(GtkWidget *window, gpointer user_data)
{ 
    int close_hndlr_id;

    close_hndlr_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (window), "close_hndlr_id"));
    g_signal_handler_block (window, close_hndlr_id);
    deregister_window(window);
    gtk_window_close(GTK_WINDOW(window));

    return;
}


/* Write camera information to file */

char *camera_info_file(camera_t *cam)
{
    /* First time set up */
    if (camfile == NULL)
	camfile = camfile_name();

    if ((cf = fopen(camfile, "w")) == (FILE *) NULL)
    {
	return NULL;
    }

    /* Camera capabilities */
    if (! write_cam_caps(cam))
	return FALSE;

    /* Camera controls and menus */
    if (! write_cam_controls(cam))
	return NULL;

    /* Camera formats, resolutions and frame ratess */
    if (! write_cam_formats(cam))
	return NULL;

    /* Close */
    fclose(cf);
    cf = NULL;

    return camfile;
}


/* Write camera information to file */

int write_cam_caps(camera_t *cam)
{
    fprintf(cf, "\n\n*** Camera Information ***\n    ------------------\n\n");
    fprintf(cf, "  Camera Name: %s\n", cam->vcaps.card);
    fprintf(cf, "  Device: %s\n", cam->video_dev);
    fprintf(cf, "  Driver: %s\n", cam->vcaps.driver);
    fprintf(cf, "  Version: %2d\n", cam->vcaps.version);
    fprintf(cf, "  Bus Info: %s\n\n", cam->vcaps.bus_info);

    fprintf(cf, "  CAPABILITIES\n");

    if (cam->vcaps.capabilities & V4L2_CAP_VIDEO_CAPTURE)
    	fprintf(cf, "    Video Capture: Yes\n");
    else
    	fprintf(cf, "    Video Capture: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_VIDEO_OUTPUT)
    	fprintf(cf, "    Video Output: Yes\n");
    else
    	fprintf(cf, "    Video Output: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_VIDEO_OVERLAY)
    	fprintf(cf, "    Video Overlay: Yes\n");
    else
    	fprintf(cf, "    Video Overlay: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_VBI_CAPTURE)
    	fprintf(cf, "    Raw VBI Capture: Yes\n");
    else
    	fprintf(cf, "    Raw VBI Capture: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_SLICED_VBI_CAPTURE)
    	fprintf(cf, "    Sliced VBI Capture: Yes\n");
    else
    	fprintf(cf, "    Sliced VBI Capture: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_SLICED_VBI_OUTPUT)
    	fprintf(cf, "    Sliced VBI Output: Yes\n");
    else
    	fprintf(cf, "    Sliced VBI Output: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_RDS_CAPTURE)
    	fprintf(cf, "    RDS Data Capture: Yes\n");
    else
    	fprintf(cf, "    RDS Data Capture: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_VIDEO_OUTPUT_OVERLAY)
    	fprintf(cf, "    Video Output Overlay: Yes\n");
    else
    	fprintf(cf, "    Video Output Overlay: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_HW_FREQ_SEEK)
    	fprintf(cf, "    Hardware Frequency Seek: Yes\n");
    else
    	fprintf(cf, "    Hardware Frequency Seek: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_RDS_OUTPUT)
    	fprintf(cf, "    RDS Encoder: Yes\n");
    else
    	fprintf(cf, "    RDS Encoder: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
    	fprintf(cf, "    Capture Multiplanar formats: Yes\n");
    else
    	fprintf(cf, "    Capture Multiplanar formats: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE)
    	fprintf(cf, "    Output Multiplanar formats: Yes\n");
    else
    	fprintf(cf, "    Output Multiplanar formats: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_TUNER)
    	fprintf(cf, "    Device has a tuner: Yes\n");
    else
    	fprintf(cf, "    Device has a tuner: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_AUDIO)
    	fprintf(cf, "    Audio support: Yes\n");
    else
    	fprintf(cf, "    Audio support: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_RADIO)
    	fprintf(cf, "    Radio support: Yes\n");
    else
    	fprintf(cf, "    Radio support: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_MODULATOR)
    	fprintf(cf, "    Device has a modulator: Yes\n");
    else
    	fprintf(cf, "    Device has a modulator: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_READWRITE)
    	fprintf(cf, "    Read / Write system calls: Yes\n");
    else
    	fprintf(cf, "    Read / Write system calls: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_ASYNCIO)
    	fprintf(cf, "    Async I/O support: Yes\n");
    else
    	fprintf(cf, "    Async I/O support: No\n");

    if (cam->vcaps.capabilities & V4L2_CAP_STREAMING)
    	fprintf(cf, "    Streaming support: Yes\n");
    else
    	fprintf(cf, "    Streaming support: No\n");

    fflush(cf);

    return TRUE;
}


/* Enumerate camera controls */

int write_cam_controls(camera_t *cam)
{
    struct v4l2_queryctrl *qctrl;
    struct v4l2_list *tmp;

    /* Base controls */
    tmp = cam->ctl_head;

    while(tmp != NULL)
    {
	qctrl = (struct v4l2_queryctrl *) tmp->v4l2_data;

	if (qctrl->id == V4L2_CID_BASE)	
	    write_cam_ctrl(qctrl, TRUE, FALSE);	 
	else
	    write_cam_ctrl(qctrl, FALSE, FALSE);

	/* If the control is a menu, enumerate it */
	if (qctrl->type == V4L2_CTRL_TYPE_MENU)
	{
	    if (! write_cam_menu_items(tmp))
		return FALSE;
	}

	tmp = tmp->next;
    }

    /* Private controls */
    if (cam->pctl_head == NULL)
    	return TRUE;

    tmp = cam->pctl_head;

    while(tmp != NULL)
    {
	qctrl = (struct v4l2_queryctrl *) tmp->v4l2_data;
	write_cam_ctrl(qctrl, FALSE, TRUE);
	tmp = tmp->next;
    }

    return TRUE;
}


/* Write the camera controls to the .cam_file */

void write_cam_ctrl(struct v4l2_queryctrl *qctrl, int hdr, int priv)
{
    if (hdr == TRUE)
	fprintf(cf, "\n  CONTROLS AND MENUS\n");

    fprintf(cf, "\n    VIDIOC_QUERYCTRL(V4L2_CID_BASE + %d)\n", qctrl->id - V4L2_CID_BASE);

    switch (qctrl->type)
    {
	case V4L2_CTRL_TYPE_INTEGER:
	    fprintf(cf, "    Type: INTEGER\n");
	    break;

	case V4L2_CTRL_TYPE_BOOLEAN:
	    fprintf(cf, "    Type: BOOLEAN\n");
	    break;

	case V4L2_CTRL_TYPE_MENU:
	    fprintf(cf, "    Type: MENU\n");
	    break;

	case V4L2_CTRL_TYPE_BUTTON:
	    fprintf(cf, "   Type: BUTTON\n");
	    break;
    }

    if (priv == FALSE)
	fprintf(cf,"    Name: %s\n", qctrl->name);
    else
	fprintf(cf,"    Name: %s (Private)\n", qctrl->name);

    fprintf(cf,"    Minimum: %d\n", qctrl->minimum);
    fprintf(cf,"    Maximum: %d\n", qctrl->maximum);
    fprintf(cf,"    Step: %d\n", qctrl->step);
    fprintf(cf,"    Default_value: %d\n", qctrl->default_value);
    fprintf(cf,"    Flags: %d\n\n", qctrl->flags);
    fflush(cf);

    return;
}


/* Enumerate camera control menu */

int write_cam_menu_items(struct v4l2_list *ctlNode)
{
    struct v4l2_querymenu *qmenu;
    struct v4l2_list *tmp;

    tmp = ctlNode->sub_list_head;

    while(tmp != NULL)
    {
	qmenu = (struct v4l2_querymenu *) tmp->v4l2_data;
	write_cam_menu(qmenu);
	tmp = tmp->next;
    }

    return TRUE;
}


/* Write the camera menu items to the .cam_file */

void write_cam_menu(struct v4l2_querymenu *qmenu)
{
    fprintf(cf, "    Menu Id: %d\n", qmenu->index);
    fprintf(cf, "    Menu Name: %s\n", qmenu->name);
    fflush(cf);

    return;
}


/* Enumerate camera formats */

int write_cam_formats(camera_t *cam)
{
    struct v4l2_fmtdesc *vfmt;
    struct v4l2_list *tmp;

    tmp = cam->fmt_head;

    while(tmp != NULL)
    {
	vfmt = (struct v4l2_fmtdesc *) tmp->v4l2_data;

	if (tmp == cam->fmt_head)
	    write_cam_fmt(vfmt, TRUE);	 
	else
	    write_cam_fmt(vfmt, FALSE);

	/* Each format has 1 or more frame sizes it supports */
	if (! write_cam_frame_sizes(tmp))
	    return FALSE;

	tmp = tmp->next;
    }

    return TRUE;
}


/* Write the camera formats to the .cam_file */

void write_cam_fmt(struct v4l2_fmtdesc *vfmt, int hdr)
{
    char buf_type[30];
    char fmt_flag[30];
    char fourcc[5];

    if (hdr == TRUE)
	fprintf(cf, "\n  SUPPORTED VIDEO FORMATS\n");

    switch(vfmt->type)
    {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	    strcpy(buf_type, "VIDEO CAPTURE");
	    break;

	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	    strcpy(buf_type, "VIDEO OUTPUT");
	    break;

	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	    strcpy(buf_type, "VIDEO OVERLAY");
	    break;

	case V4L2_BUF_TYPE_VBI_CAPTURE:
	    strcpy(buf_type, "VBI VIDEO CAPTURE");
	    break;

	case V4L2_BUF_TYPE_VBI_OUTPUT:
	    strcpy(buf_type, "VBI VIDEO OUTPUT");
	    break;

	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	    strcpy(buf_type, "VBI VIDEO OVERLAY");
	    break;

	default:
	    sprintf(buf_type, "OTHER BUFFER TYPE - %d", vfmt->type);
    }

    if (vfmt->flags)
	strcpy(fmt_flag, "Compressed");
    else
	strcpy(fmt_flag, "Uncompressed");

    fprintf(cf, "    VIDIOC_ENUM_FMT(%d, %d)\n", vfmt->index, vfmt->type);
    fprintf(cf, "      index        :%d\n", vfmt->index);
    fprintf(cf, "      type         :%s\n", buf_type);
    fprintf(cf, "      flags        :%s\n", fmt_flag);
    fprintf(cf, "      description  :%s\n", vfmt->description);

    /* Convert the pixelformat attributes from FourCC into 'human readable' format */
    pxl2fourcc(vfmt->pixelformat, fourcc);
    fprintf(cf, "      pixelformat  :%s\n", fourcc);
    fflush(cf);

    return;
}


/* Enumerate camera frame sizes (resolution) */

int write_cam_frame_sizes(struct v4l2_list *fmtNode)
{
    struct v4l2_frmsizeenum *vfrm;
    struct v4l2_list *tmp;

    tmp = fmtNode->sub_list_head;

    while(tmp != NULL)
    {
	vfrm = (struct v4l2_frmsizeenum *) tmp->v4l2_data;
	write_cam_res(vfrm);

	/* Each frame size has 1 or more frame intervals it supports */
	if (! write_cam_frame_ints(tmp))
	    return FALSE;

	tmp = tmp->next;
    }

    return TRUE;
}


/* Write a frame size (resolution) to the cam_file */

void write_cam_res(struct v4l2_frmsizeenum *vfrm)
{
    const char *type_desc[] = { "DISCRETE", "STEPWISE", "CONT" };

    if (vfrm->index == 0)
	fprintf(cf, "        Type %d (%s) Frame Sizes\n", vfrm->type, type_desc[vfrm->type - 1]);

    switch(vfrm->type)
    {
	case V4L2_FRMSIZE_TYPE_DISCRETE:
	    fprintf(cf, "\n          %d x %d\n", vfrm->discrete.width, vfrm->discrete.height);
	    break;

	case V4L2_FRMSIZE_TYPE_CONTINUOUS:
	case V4L2_FRMSIZE_TYPE_STEPWISE:
	    fprintf(cf, "\n          %d x %d to %d x %d  Width step: %d Height step: %d\n",
	    						vfrm->stepwise.min_width,
	    						vfrm->stepwise.min_height,
	    						vfrm->stepwise.max_width,
	    						vfrm->stepwise.max_height,
	    						vfrm->stepwise.step_width,
	    						vfrm->stepwise.step_height);
	    break;
    }

    fflush(cf);

    return;
}


/* Enumerate camera frame intervals (fps) */

int write_cam_frame_ints(struct v4l2_list *fmtNode)
{
    struct v4l2_frmivalenum *vfrm;
    struct v4l2_list *tmp;

    tmp = fmtNode->sub_list_head;

    while(tmp != NULL)
    {
	vfrm = (struct v4l2_frmivalenum *) tmp->v4l2_data;
	write_cam_frmival(vfrm, (struct v4l2_frmsizeenum *) fmtNode->v4l2_data);
	tmp = tmp->next;
    }

    return TRUE;
}


/* Write a frame interval(s) (rates) to the .cam_file */

void write_cam_frmival(struct v4l2_frmivalenum *vfrmival, struct v4l2_frmsizeenum *vfrm)
{
    const char *type_desc[] = { "DISCRETE", "STEPWISE", "CONT" };
    int fps;

    if (vfrmival->index == 0)
    {
	fprintf(cf, "          Type %d (%s) Frame Intervals\n", vfrmival->type, 
								type_desc[vfrmival->type - 1]);

	if (vfrm->type != V4L2_FRMSIZE_TYPE_DISCRETE)
	    fprintf(cf, "          NOTE: Only Interval details for the minimun "
	    		"(stepwise or continuous) frame size are shown1\n");
    }

    switch(vfrmival->type)
    {
	case V4L2_FRMSIZE_TYPE_DISCRETE:
	    fps = (int) ((float) vfrmival->discrete.denominator / (float) vfrmival->discrete.numerator);
	    fprintf(cf, "            %d / %d   (%d fps)\n", vfrmival->discrete.numerator, 
							    vfrmival->discrete.denominator,
							    fps);
	    break;

	case V4L2_FRMSIZE_TYPE_CONTINUOUS:
	case V4L2_FRMSIZE_TYPE_STEPWISE:
	    fprintf(cf, "            %d / %d to %d / %d   Step %d / %d\n",
	    						vfrmival->stepwise.min.numerator, 
						        vfrmival->stepwise.min.denominator,
						        vfrmival->stepwise.max.numerator,
						        vfrmival->stepwise.max.denominator,
							vfrmival->stepwise.step.numerator,
	    						vfrmival->stepwise.step.denominator);
	    break;
    }

    fflush(cf);

    return;
}


/* Read the camera information file */

int read_cam_info(char *buf, int sz_len)
{
    int i, max;
    char c;

    if (cf == NULL)
    {
	if ((cf = fopen(camfile, "r")) == (FILE *) NULL)
	{
	    return FALSE;
	}
    }

    i = 0;
    max = sz_len - 1;
    buf[0] = '\0';
    
    while((c = fgetc(cf)) != EOF)
    {
    	buf[i++] = c;

    	if (i >= max)
	    break;
    }

    buf[i] = '\0';

    if (c == EOF)
    {
    	fclose(cf);
    	cf = NULL;
    }

    return (int) c;
}


/* Convert a pixel format into its fourcc code */

void pxl2fourcc(pixelfmt pixelformat, char *s)
{
    sprintf(s, "%c%c%c%c", pixelformat & 0xFF, 
			   (pixelformat >> 8) & 0xFF,
			   (pixelformat >> 16) & 0xFF,
			   (pixelformat >> 24) & 0xFF);           

    return;
}


/* Convert a fourcc code to pixel format */

pixelfmt fourcc2pxl(char *s)
{
    if (strlen(s) != 4)
    	return 0;		//Unknown or unspecified format

    return ((pixelfmt) (((*(s+3))<<24) | ((*(s+2))<<16) | ((*(s+1))<<8) | (*s)));
}


/* Extract the width and height from a resolution string */

void res_to_long(char *res, long *width, long *height)
{
    char *p;
    char s[10];

    p = strstr(res, " x ");

    if (p == NULL)
    {
	*width = 0;
	*height = 0;
	return;
    }

    *p = '\0';
    strncpy(s, res, p - res);
    *width = atol(s);
    strcpy(s, p + 3);
    *height = atol(s);
    *p = ' ';

    return;
}


/* Calculate a frame rate */

int calc_fps(pixelfmt denominator, pixelfmt numerator)
{
    return (int) ((float) denominator / (float) numerator);
}
