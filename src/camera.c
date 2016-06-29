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
** Description:	Camera lookup and update functions.
**
** Author:	Anthony Buckley
**
** History
**	15-Dec-2013	Initial code
**
*/


/* Includes */

#include <gst/gst.h>
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
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <session.h>
#include <main.h>
#include <cam.h>
#include <defs.h>


/* Defines */
#define MAX_STD_CTRLS 10


/* Prototypes */

//GList* gst_camera_devices(gchar*);
struct camlistNode* dev_camera_devices(GtkWidget*);
struct camlistNode* new_listNode();
void add_listNode(struct camlistNode*, struct camlistNode**);
int camera_setup(camera_t*, GtkWidget*);
int camera_caps(camera_t*, GtkWidget*);
int camera_ctrls(camera_t*, GtkWidget*);
int camera_ctrl_menu(camera_t *, struct v4l2_queryctrl *, struct v4l2_list *, GtkWidget *);
int camera_formats(camera_t*, GtkWidget*);
int camera_res(camera_t*, struct v4l2_fmtdesc*, struct v4l2_list *, GtkWidget*);
int camera_frmival(camera_t *, struct v4l2_frmsizeenum *, struct v4l2_list *, GtkWidget *);
struct v4l2_list *new_v4l2Node(int);
int std_controls(camera_t *); 
struct v4l2_queryctrl * get_next_ctrl(int); 
struct v4l2_list * get_next_oth_ctrl(struct v4l2_list *, CamData *); 
int find_ctl(camera_t *, char *); 
int get_cam_ctrl(long, struct v4l2_queryctrl *, camera_t *, GtkWidget *);
int set_cam_ctrl(camera_t *, struct v4l2_queryctrl *, long, GtkWidget *);
int cam_defaults(camera_t *, MainUi *, struct v4l2_list *); 
int cam_ctrl_reset(CamData *, GtkWidget *, char, GtkWidget *); 
void cam_reset_range(GtkWidget *, CamData *, char, GtkWidget *); 
void cam_reset_cbox(GtkWidget *, CamData *, char, GtkWidget *); 
void cam_reset_radio(GtkWidget *, CamData *, char, GtkWidget *); 
int get_reset_val(const gchar *, struct v4l2_queryctrl *, char, long *);
void cam_auto_reset(GtkWidget *, struct v4l2_queryctrl *, long, camera_t *, GtkWidget *);
int cam_fmt_update(CamData *, char *);
int cam_fmt_read(CamData *, struct v4l2_format *, struct v4l2_fmtdesc **, int);
int get_cam_fmt(camera_t *, struct v4l2_format *, GtkWidget *, int);
int set_cam_fmt(camera_t *, struct v4l2_format *, GtkWidget *);
void camera_res_sort(struct v4l2_list *, struct v4l2_list **, struct v4l2_list **);
int cam_fps_update(CamData *, char *);
int cam_fps_read(CamData *, char *, struct v4l2_streamparm *, struct v4l2_frmivalenum **, int);
int get_cam_streamparm(camera_t *, struct v4l2_streamparm *, GtkWidget *, int);
int set_cam_streamparm(camera_t *, struct v4l2_streamparm *, GtkWidget *);
struct v4l2_list * find_fmt(CamData *, struct v4l2_fmtdesc **, char *);
struct v4l2_list * find_frm(CamData *, struct v4l2_list *, struct v4l2_frmsizeenum **, char *);
struct v4l2_list * find_frmival(CamData *, struct v4l2_list *, struct v4l2_frmivalenum **, char *);
int get_fps(CamData *, char *);
int xioctl(int, int, void *);
void xv4l2_close(camera_t *);
int cam_open(char *, int, GtkWidget *);
void session_ctrl_val(struct v4l2_queryctrl *, char *, long *);
void save_ctrl(struct v4l2_queryctrl *, char *, long, CamData *, GtkWidget *);
void clear_camera_list(CamData *);
void free_cam_data(struct v4l2_list *);


extern void log_msg(char*, char*, char*, GtkWidget*);
extern void pxl2fourcc(pixelfmt, char *);
extern void set_scale_val(GtkWidget *, char *, long);
extern int set_session(char *, char *);
extern void get_session_reset(char*, char**);
extern void get_session(char*, char**);
//extern GtkWidget * get_next_widget(GtkWidget *, int, GtkWidget *);
extern GList * ctrl_widget_list(GtkWidget *, GtkWidget *);
extern GtkWidget * find_parent(GtkWidget *);
extern pixelfmt fourcc2pxl(char *);
extern void res_to_long(char *, long *, long *);
extern int calc_fps(pixelfmt, pixelfmt);


/* Globals */

static const char *debug_hdr = "DEBUG-camera.c ";
static const char *v4l2_err = "Possible causes are:\n"
			      "The driver may not support this function, driver error "
			      "or some other error. See the description for more details.";
static const char *v4l2_warn = "Function returned 0 results (may not be supported). "
			       "Some standard ones will be used but may not work.";
struct camlistNode *head = NULL;
static struct v4l2_queryctrl *p_std_ctrls[MAX_STD_CTRLS];
static int current_idx;


/* Use GST Probe to return a list of WebCams */

/*
GList* gst_camera_devices(gchar *device_name)
{
    GstElement *device; 
    GstPropertyProbe *probe;
    GValueArray *va, *va2; 
    GList *list = NULL; 
    guint i = 0; 

    device = gst_element_factory_make (device_name, "source");
    gst_element_set_state(device, GST_STATE_READY);
    gst_element_get_state(device, NULL, NULL, 5 * GST_SECOND);

    if (device && GST_IS_PROPERTY_PROBE(device))
    {
	probe = GST_PROPERTY_PROBE (device);
	va = gst_property_probe_get_values_name (probe, "device-name"); 
	va2 = gst_property_probe_get_values_name (probe, "device"); 

	if ((va) && (va2))
	{
	    for(i = 0; i < va->n_values; ++i)
	    {
		GValue* cam_nm = g_value_array_get_nth(va, i);
		GValue* cam_dev = g_value_array_get_nth(va2, i);
		set_list(&list, &cam_nm, &cam_dev);
	    }

	    g_value_array_free(va);
	    g_value_array_free(va2);
	}
    }

    gst_element_set_state (device, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT (device));

    return list;
}
*/


/* Use V4L2 ioctl to get WebCam details and return a list (probably more efficient) */

struct camlistNode* dev_camera_devices(GtkWidget *window)
{
    DIR *dp = NULL;
    struct dirent *ep;
    struct stat fileStat;
    int fd, err, sz_dev, sz_fs;
    char video_dev[100];
    const char *sysfsclass = V4L_SYS_CLASS;
    struct camlistNode *v_node;

    /* Open video directory */
    if((dp = opendir(sysfsclass)) == NULL)
    {
	log_msg("CAM0001", (char *) sysfsclass, "SYS9000", window);
        return NULL;
    }

    sz_fs = strlen(sysfsclass);
    sz_dev = sizeof(video_dev);

    /* Iterate thru the video devices */
    while (ep = readdir(dp))
    {
    	if (strncmp(ep->d_name, "video", 5) != 0)
	    continue;

	if ((strlen(ep->d_name) + sz_fs) > sz_dev)
	{
	    log_msg("SYS9006", NULL, "SYS9000", window);
	    return NULL;
	}

	sprintf(video_dev, "%s/%s", sysfsclass, ep->d_name);

	if ((err = lstat(video_dev, &fileStat)) < 0)
	{
	    sprintf(app_msg_extra, "File: %s Error: %s", ep->d_name, strerror(errno)); 
	    log_msg("CAM0002", NULL, "SYS9000", window);
	    continue;
	}

	if (! S_ISLNK(fileStat.st_mode))
	    continue;

	sprintf(video_dev, "%s/%s", DEV_DIR, ep->d_name);

	/* Open the camera */
	if ((fd = cam_open(video_dev, O_RDONLY, window)) == -1)
	    return NULL;

	/* Connect to the camera and get caps */
	v_node = new_listNode();

	v_node->cam->fd = fd;
	strcpy(v_node->cam->video_dev, video_dev);

	if (! camera_caps(v_node->cam, window))
	    return NULL;

	add_listNode(v_node, &head);
	
	v4l2_close(fd);
    }

    closedir(dp);

    return head;
}


/* Set up a new list node for a camera */

struct camlistNode *new_listNode()
{
    struct camlistNode *n = (struct camlistNode *) malloc(sizeof(struct camlistNode));
    n->cam = (camera_t *) malloc(sizeof(camera_t) + 1);
    n->next = NULL;
    memset(n->cam, 0, sizeof(camera_t));

    return n;
}


/* Set the camera structure into a list */

void add_listNode(struct camlistNode *vn, struct camlistNode **headNode)
{
    struct camlistNode *tmp;

    if (*headNode == NULL)
    {
        *headNode = vn;
        return;
    }

    tmp = *headNode;

    while(tmp->next != NULL)
    {
    	tmp = tmp->next;
    }

    tmp->next = vn;

    return;
}


/* Get the camera details and capabilities */

int camera_caps(camera_t *cam, GtkWidget *window)
{
    if (xioctl(cam->fd, VIDIOC_QUERYCAP, &cam->vcaps) == -1)
    {
	sprintf(app_msg_extra, "%s Error: %s", v4l2_err, strerror(errno));
	log_msg("CAM0004", "VIDIOC_QUERYCAP", "SYS9000", window);
	return FALSE;
    }

    return TRUE;
}


/* Find camera details - controls, menus, formats */

int camera_setup(camera_t *cam, GtkWidget *window)
{
    /* Return if the details are already populated (may need to rebuild controls though) */
    if (cam->ctl_head)
    {
	std_controls(cam);
	return TRUE;
    }

    /* Open the camera */
    if ((cam->fd = cam_open(cam->video_dev, O_RDWR, window)) == -1)
	return FALSE;

    if (! camera_ctrls(cam, window))
	return FALSE;

    if (! camera_formats(cam, window))
	return FALSE;

    xv4l2_close(cam);

    return TRUE;
}


/* Get the camera controls and menus that are required */

int camera_ctrls(camera_t *cam, GtkWidget *window)
{
    struct v4l2_queryctrl qctrl; 
    struct v4l2_list *v_node;

    memset(&qctrl, 0, sizeof(qctrl));

    /* Enumerate the base controls */
    for(qctrl.id = V4L2_CID_BASE; qctrl.id < V4L2_CID_LASTP1; qctrl.id++)
    {
	if (xioctl(cam->fd, VIDIOC_QUERYCTRL, &qctrl) != 0)
	{
	    if ((errno == EINVAL) || (qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
	    	continue;
	    else
	    {
		sprintf(app_msg_extra, "%s Error: %s", v4l2_err, strerror(errno));
		log_msg("CAM0005", "VIDIOC_QUERYCTRL", "SYS9000", window);
		return FALSE;
	    }
	}

	if (! (qctrl.flags == 0 || qctrl.flags & V4L2_CTRL_FLAG_SLIDER || qctrl.flags & V4L2_CTRL_FLAG_INACTIVE))
	    continue;

	v_node = new_v4l2Node(sizeof(qctrl));
	memcpy(v_node->v4l2_data, &qctrl, sizeof(qctrl));

	if (! cam->ctl_head)
	    cam->ctl_head = v_node;
	else
	    cam->ctl_last->next = v_node;

	cam->ctl_last = v_node;

	/* If the control is a menu, enumerate it */
	if (qctrl.type == V4L2_CTRL_TYPE_MENU)
	{
	    if (! camera_ctrl_menu(cam, &qctrl, v_node, window))
		return FALSE;
	}
    }

    /* Enumerate the private controls */
    memset(&qctrl, 0, sizeof(qctrl));

    for(qctrl.id = V4L2_CID_PRIVATE_BASE; errno != EINVAL; qctrl.id++)
    {
	if (xioctl(cam->fd, VIDIOC_QUERYCTRL, &qctrl) != 0)
	{
	    if ((errno == EINVAL) || (qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
	    	break;
	    else
	    {
		sprintf(app_msg_extra, "%s Error: %s", v4l2_err, strerror(errno));
		log_msg("CAM0005", "VIDIOC_QUERYCTRL", "SYS9000", window);
		return FALSE;
	    }
	}

	v_node = new_v4l2Node(sizeof(qctrl));
	memcpy(v_node->v4l2_data, &qctrl, sizeof(qctrl));

	if (! cam->pctl_head)
	    cam->pctl_head = v_node;
	else
	    cam->pctl_last->next = v_node;

	cam->pctl_last = v_node;
    }

    return TRUE;
}


/* Enumerate the camera menu for a control */

int camera_ctrl_menu(camera_t *cam, 
		     struct v4l2_queryctrl *qctrl,
		     struct v4l2_list *ctlNode, 
		     GtkWidget *window)
{
    struct v4l2_querymenu qmenu;
    struct v4l2_list *v_node;

    memset(&qmenu, 0, sizeof(qmenu));
    qmenu.id = qctrl->id;

    for(qmenu.index = qctrl->minimum; qmenu.index <= qctrl->maximum; qmenu.index++)
    {
	if (xioctl(cam->fd, VIDIOC_QUERYMENU, &qmenu) != 0)
	{
	    if (errno == EINVAL)
		continue;
	    else
	    {
		sprintf(app_msg_extra, "%s Error: %s", v4l2_err, strerror(errno));
		log_msg("CAM0005", "VIDIOC_QUERYMENU", "SYS9000", window);
		return FALSE;
	    }
	}

	v_node = new_v4l2Node(sizeof(qmenu));
	memcpy(v_node->v4l2_data, &qmenu, sizeof(qmenu));

	if (! ctlNode->sub_list_head)
	    ctlNode->sub_list_head = v_node;
	else
	    ctlNode->sub_list_last->next = v_node;

	ctlNode->sub_list_last = v_node;
    }

    return TRUE;
}


/* Get the camera supported (pixel) formats */

int camera_formats(camera_t *cam, GtkWidget *window)
{
    struct v4l2_fmtdesc vfmt; 
    struct v4l2_list *v_node;

    int i;
    const int max_vfmts = 10;

    memset(&vfmt, 0, sizeof(vfmt));
    vfmt.index = 0;
    i = 0;

    for(vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; vfmt.type <= V4L2_BUF_TYPE_VIDEO_OVERLAY; vfmt.type++)
    {
	while(xioctl(cam->fd, VIDIOC_ENUM_FMT, &vfmt) >= 0)
        {
	    /*
	    if (errno == EINVAL)
	    	break;
	    */

	    v_node = new_v4l2Node(sizeof(vfmt));
	    memcpy(v_node->v4l2_data, &vfmt, sizeof(vfmt));

	    if (cam->fmt_head == NULL)
		cam->fmt_head = v_node;
	    else
		cam->fmt_last->next = v_node;

	    cam->fmt_last = v_node;

	    /* Each format has 1 or more frame sizes it supports */
	    if (! camera_res(cam, &vfmt, v_node, window))
	    	return FALSE;

	    vfmt.index++;
	}

	vfmt.index = 0;
    }

    /* If none were found, either the function is not supported or its the driver */
    if (cam->fmt_head == NULL)
    {
	sprintf(app_msg_extra, "VIDIOC_ENUM_FMT %s", v4l2_warn);
	log_msg("CAM0006", "video formats (VIDIOC_ENUM_FMT)", NULL, NULL);
    }

    return TRUE;
}


/* Enumerate the camera supported frame sizes (resolution) for a given (pixel) format */

int camera_res(camera_t *cam, 
	       struct v4l2_fmtdesc *vfmt,
	       struct v4l2_list *fmtNode,
	       GtkWidget *window)
{
    struct v4l2_frmsizeenum vfrm; 
    struct v4l2_list *v_node;

    memset(&vfrm, 0, sizeof(vfrm));
    vfrm.index = 0;
    vfrm.pixel_format = vfmt->pixelformat;

    while(xioctl(cam->fd, VIDIOC_ENUM_FRAMESIZES, &vfrm) >= 0)
    {
	v_node = new_v4l2Node(sizeof(vfrm));
	memcpy(v_node->v4l2_data, &vfrm, sizeof(vfrm));

	/* Resolutions should be ordered */
	camera_res_sort(v_node, &(fmtNode->sub_list_head), &(fmtNode->sub_list_last));

	/* Each frame size has 1 or more frame rates it supports */
	if (! camera_frmival(cam, &vfrm, v_node, window))
	    return FALSE;

	if (vfrm.index == 0)
	{
	    if (vfrm.type != V4L2_FRMSIZE_TYPE_DISCRETE)
	    	break;
	}
	 
	vfrm.index++;
    }

    /* If none were found, either the function is not supported or its the driver */
    if (fmtNode->sub_list_head == NULL)
    {
	sprintf(app_msg_extra, "VIDIOC_ENUM_FRAMESIZES %s", v4l2_warn);
	log_msg("CAM0006", "video frame sizes (VIDIOC_ENUM_FRAMESIZES)", NULL, NULL);
    }

    return TRUE;
}


/* Sort the camera resolutions based on the width */

void camera_res_sort(struct v4l2_list *vn, struct v4l2_list **headNode, struct v4l2_list **endNode)
{
    struct v4l2_frmsizeenum *vn_frm; 
    struct v4l2_frmsizeenum *tmp_frm; 
    struct v4l2_list *prev;
    struct v4l2_list *tmp;

    if (*headNode == NULL)
    {
    	*headNode = vn;
    	*endNode = vn;
    	return;
    }

    tmp = *headNode;
    prev = NULL;

    while(TRUE)
    {
	vn_frm = (struct v4l2_frmsizeenum *) vn->v4l2_data;
	tmp_frm = (struct v4l2_frmsizeenum *) tmp->v4l2_data;

	if (vn_frm->discrete.width < tmp_frm->discrete.width)
	{
	    // add in to list
	    if (prev == NULL)
	    	*headNode = vn;
	    else
	    	prev->next = vn;

	    vn->next = tmp;
	    break;
	}
    	else
    	{
	    if (tmp->next == NULL)
	    {
		// add to end of list
		tmp->next = vn;
		*endNode = vn;
		break;
	    }
    	}

	// check next in list
	prev = tmp;
	tmp = prev->next;
    }

    return;
}


// Enumerate the camera supported frame intervals (rates) for a given (pixel) format and size
// For Stepwise and Continuous only enumerate the min size values and not the steps

int camera_frmival(camera_t *cam,
		   struct v4l2_frmsizeenum *vfrm,
		   struct v4l2_list *frmNode,
		   GtkWidget *window)
{
    struct v4l2_frmivalenum vfrmival; 
    struct v4l2_list *v_node;

    memset(&vfrmival, 0, sizeof(vfrmival));
    vfrmival.index = 0;
    vfrmival.pixel_format = vfrm->pixel_format;

    if (vfrm->type == V4L2_FRMSIZE_TYPE_DISCRETE)
    {
	vfrmival.width = vfrm->discrete.width;
	vfrmival.height = vfrm->discrete.height;
    }
    else
    {
	vfrmival.width = vfrm->stepwise.min_width;
	vfrmival.height = vfrm->stepwise.min_height;
    }

    while(xioctl(cam->fd, VIDIOC_ENUM_FRAMEINTERVALS, &vfrmival) >= 0)
    {
	v_node = new_v4l2Node(sizeof(vfrmival));
	memcpy(v_node->v4l2_data, &vfrmival, sizeof(vfrmival));

	if (! frmNode->sub_list_head)
	    frmNode->sub_list_head = v_node;
	else
	    frmNode->sub_list_last->next = v_node;

	frmNode->sub_list_last = v_node;

	if (vfrmival.index == 0)
	{
	    if (vfrmival.type != V4L2_FRMIVAL_TYPE_DISCRETE)
	    	break;
	}
	 
	vfrmival.index++;
    }

    /* If none were found, either the function is not supported or its the driver */
    if (frmNode->sub_list_head == NULL)
    {
	sprintf(app_msg_extra, "VIDIOC_ENUM_FRAMEINTERVALS %s", v4l2_warn);
	log_msg("CAM0006", "video frame intervals (VIDIOC_ENUM_FRAMEINTERVALS)", NULL, NULL);
    }

    return TRUE;
}


/* Set up a new list node for a v4l2 item (structure) */

struct v4l2_list *new_v4l2Node(int data_sz)
{
    struct v4l2_list *n = (struct v4l2_list *) malloc(sizeof(struct v4l2_list));
    n->v4l2_data = (void *) malloc(data_sz);
    n->next = NULL;
    n->sub_list_head = NULL;
    n->sub_list_last = NULL;

    return n;
}


/* Populate pointers to the standard controls */

int std_controls(camera_t *cam) 
{
    // These controls are the standard ones for adjustment in the order to be
    // set up in the control panel. The remaining ones are available via 'More Options'.
    const int std_ctrls[] =
    		   { 
		    V4L2_CID_BASE + 19,		// Gain
		    V4L2_CID_BASE + 17,		// Exposure
		    V4L2_CID_BASE + 13,		// White Balance
		    V4L2_CID_BASE + 28,		// Backlight Compensation
		    V4L2_CID_BASE + 16,		// Gamma
		    V4L2_CID_BASE + 27,		// Sharpness
		    V4L2_CID_BASE + 2,		// Saturation
		    V4L2_CID_BASE + 3,		// Hue
		    V4L2_CID_BASE + 1,		// Contrast
		    V4L2_CID_BASE + 0 		// Brightness
		   };

    int i;
    struct v4l2_queryctrl *qctrl; 
    struct v4l2_list *p; 

    p = cam->ctl_head;
    memset(p_std_ctrls, 0, sizeof(p_std_ctrls));

    while(p != NULL)
    {
	qctrl = (struct v4l2_queryctrl *) p->v4l2_data;

	for(i = 0; i < MAX_STD_CTRLS; i++)
	{
	    if (qctrl->id == std_ctrls[i])
	    {
	    	p_std_ctrls[i] = qctrl;
	    	break;
	    }
	}

	p = p->next;
    }

    return TRUE;
}


/* Return a pointer to a requested standard control */

struct v4l2_queryctrl * get_next_ctrl(int init) 
{
    struct v4l2_queryctrl *p;

    if (init == TRUE)
    	current_idx = 0;

    p = NULL;

    for(; current_idx < MAX_STD_CTRLS; current_idx++)
    {
    	if (p_std_ctrls[current_idx] != NULL)
    	{
	    p = p_std_ctrls[current_idx];
	    current_idx++;
	    break;
	}
    }

    return p;
}


/* Return the next non-standard control */

struct v4l2_list * get_next_oth_ctrl(struct v4l2_list *last, CamData *cam_data) 
{
    struct v4l2_list *p;
    struct v4l2_queryctrl *qctrl, *tmp;
    int init;

    if (last == NULL)
    	p = cam_data->cam->ctl_head;
    else
    	p = last->next;

    while(p != NULL)
    {
	qctrl = (struct v4l2_queryctrl *) p->v4l2_data;
	init = TRUE;

	while((tmp = get_next_ctrl(init)) != NULL)
	{
	    init = FALSE;
	    
	    if (qctrl->id == tmp->id)			// Its a standard control
	    {
		p = p->next;
		break;
	    }
	}
	
	if (tmp == NULL)				// Its not a standard one
	    break;
    }

    return p;
}


/* Find a control in the camera lists */

int find_ctl(camera_t *cam, char *key) 
{
    struct v4l2_list *p; 
    struct v4l2_queryctrl *qctrl; 
    int key_id;

    key_id = atoi(key + 4);
    p = cam->ctl_head;

    while(p != NULL)
    {
	qctrl = (struct v4l2_queryctrl *) p->v4l2_data;

	if ((qctrl->id - V4L2_CID_BASE) == key_id)
	    return TRUE;

	p = p->next;
    }

    p = cam->pctl_head;

    while(p != NULL)
    {
	qctrl = (struct v4l2_queryctrl *) p->v4l2_data;

	if ((qctrl->id - V4L2_CID_PRIVATE_BASE) == key_id)
	    return TRUE;

	p = p->next;
    }

    return FALSE;
}


/* Get the query control for a given id */

int get_cam_ctrl(long id, struct v4l2_queryctrl *qctrl, camera_t *cam, GtkWidget *window)
{
    int r = TRUE;

    if ((cam->fd = cam_open(cam->video_dev, O_RDWR, window)) == -1)
	return FALSE;

    memset(qctrl, 0, sizeof(struct v4l2_queryctrl));
    qctrl->id = id;

    if (xioctl(cam->fd, VIDIOC_QUERYCTRL, qctrl) != 0)
    {
	sprintf(app_msg_extra, "%s Error: %s", v4l2_err, strerror(errno));
	log_msg("CAM0005", "VIDIOC_QUERYCTRL", "CAM0005", window);
	r = FALSE;
    }

    xv4l2_close(cam);

    return r;
}


/* Set a camera control value */

int set_cam_ctrl(camera_t *cam, 
		 struct v4l2_queryctrl *qctrl, 
		 long val, 
		 GtkWidget *window)
{
    struct v4l2_control ctrl; 
    int r = FALSE;

    /* Open the camera */
    if (! cam->cam_is_open)
    {
	if ((cam->fd = cam_open(cam->video_dev, O_RDWR, window)) == -1)
	    return r;
    }

    /* Get the current value */
    memset (&ctrl, 0, sizeof (ctrl));
    ctrl.id = qctrl->id;

    if (xioctl(cam->fd, VIDIOC_G_CTRL, &ctrl) != 0)
    {
	sprintf(app_msg_extra, "Control %s, Error: (%d) %s", qctrl->name, 
							     errno, 
							     strerror(errno)); 
	log_msg("CAM0011", qctrl->name, "SYS9009", window);
	xv4l2_close(cam);
	return r;
    }

    /* Set the new value (if req). The driver may clamp the value or return ERANGE, ignored here */
    if (ctrl.value != val)
    {
	ctrl.value = val;

	if (xioctl(cam->fd, VIDIOC_S_CTRL, &ctrl) == -1)
	{
	    sprintf(app_msg_extra, "New Value (%ld), Current Value (%d), Error: (%d) %s", 
				   val, ctrl.value, errno, strerror(errno)); 
	    log_msg("CAM0012", qctrl->name, "SYS9009", window);
	    xv4l2_close(cam);
	    return r;
	}

	r = TRUE;
    }
    else
    {
	r = -1;
    }

    /* Leave open if it was already open */
    if (! cam->cam_is_open)
	xv4l2_close(cam);

    return r;

    /* Debug
    xioctl(cam->fd, VIDIOC_G_CTRL, ctrl);
    printf("%s Control Before - Id %d Value %d\n", debug_hdr, ctrl->id, ctrl->value);
    printf("%s Control After - Id %d Value %d\n", debug_hdr, ctrl->id, ctrl->value);
    */
}


/* Set all the camera controls to their default value */

int cam_defaults(camera_t *cam, MainUi *m_ui, struct v4l2_list *head_node) 
{
    struct v4l2_queryctrl *qctrl; 
    struct v4l2_list *v_node;
    char s[10];
    int r;

    /* Open the camera */
    cam->cam_is_open = FALSE;

    if ((cam->fd = cam_open(cam->video_dev, O_RDWR, m_ui->window)) == -1)
	return FALSE;

    /* Loop through each control and reset to default if necessary */
    cam->cam_is_open = TRUE;
    v_node = head_node;

    while(v_node != NULL)
    {
    	qctrl = (struct v4l2_queryctrl *) v_node->v4l2_data;

	/* Check current setting and change if necessary */
    	r = set_cam_ctrl(cam, qctrl, qctrl->default_value, m_ui->window);

    	if (r == TRUE)
    	{
	    /* If a related widget exists, set its value */
	    sprintf(s, "ctl-%d", qctrl->id - V4L2_CID_BASE);
	    set_scale_val(m_ui->cntl_grid, s, qctrl->default_value);
	}
	else if (r == FALSE)
	{
	    return FALSE;
	}

    	v_node = v_node->next;
    }

    xv4l2_close(cam);

    return TRUE;
}


/* Set all the camera controls within a container to the last saved or default value */

int cam_ctrl_reset(CamData *cam_data, GtkWidget *contr, char action, GtkWidget *window) 
{
    GtkWidget *ctl_widget;
    GList *ctl_list = NULL;
    int reset_flg;
    camera_t *cam;

    /* Iterate all the relevant widgets */
    ctl_list = ctrl_widget_list(contr, window);
    ctl_list = g_list_first(ctl_list);

    /* Open the camera */
    cam = cam_data->cam;
    cam->cam_is_open = FALSE;

    if ((cam->fd = cam_open(cam->video_dev, O_RDWR, window)) == -1)
	return FALSE;

    cam->cam_is_open = TRUE;

    while(ctl_list != NULL)
    {
	ctl_widget = ctl_list->data;
	reset_flg = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (ctl_widget), "resetable"));

	if (reset_flg)
	{
	    if (GTK_IS_RANGE(ctl_widget))
	    {
		cam_reset_range(ctl_widget, cam_data, action, window);
	    }
	    else if (GTK_IS_COMBO_BOX_TEXT(ctl_widget))
	    {
		cam_reset_cbox(ctl_widget, cam_data, action, window);
	    }
	    else if (GTK_IS_RADIO_BUTTON(ctl_widget))
	    {
		cam_reset_radio(ctl_widget, cam_data, action, window);
	    }
	}

	ctl_list = g_list_next(ctl_list);
    }

    g_list_free(ctl_list);
    xv4l2_close(cam);

    return TRUE;
}


/* Set a camera control range value (action: d - default, l - last saved) */

void cam_reset_range(GtkWidget *ctl_widget, CamData *cam_data, char action, GtkWidget *window) 
{
    struct v4l2_queryctrl *qctrl; 
    gdouble curr_val;
    const gchar *ctl_key;
    long reset_val;

    /* Retrieve the current and reset values */
    qctrl = g_object_get_data(G_OBJECT (ctl_widget), "control");
    curr_val = gtk_range_get_value(GTK_RANGE (ctl_widget));

    ctl_key = gtk_widget_get_name(ctl_widget);

    if (get_reset_val(ctl_key, qctrl, action, &reset_val) == FALSE)
    	return;

    /* Reset the control if necessary */
    if (curr_val != (gdouble) reset_val)
    {
	gtk_range_set_value(GTK_RANGE (ctl_widget), (gdouble) reset_val);
	save_ctrl(qctrl, (char *) ctl_key, reset_val, cam_data, window);
    }

    return;
}


/* Set a camera control combobox (menu) value (action: d - default, l - last saved) */

void cam_reset_cbox(GtkWidget *ctl_widget, CamData *cam_data, char action, GtkWidget *window) 
{
    struct v4l2_queryctrl *qctrl; 
    int curr_idx; 
    long reset_idx;
    const gchar *ctl_key;

    /* Retrieve the current and reset values */
    qctrl = g_object_get_data(G_OBJECT (ctl_widget), "control");
    curr_idx = gtk_combo_box_get_active(GTK_COMBO_BOX (ctl_widget));

    ctl_key = gtk_widget_get_name(ctl_widget);

    if (get_reset_val(ctl_key, qctrl, action, &reset_idx) == FALSE)
    	return;

    /* Reset the control if necessary */
    if (curr_idx != reset_idx)
	gtk_combo_box_set_active(GTK_COMBO_BOX (ctl_widget), reset_idx);	// Trigger callback

    return;
}


/* Set a camera control radio (menu, boolean) value (action: d - default, l - last saved) */

void cam_reset_radio(GtkWidget *ctl_widget, CamData *cam_data, char action, GtkWidget *window) 
{
    struct v4l2_queryctrl *qctrl; 
    int curr_idx; 
    long reset_idx;
    gboolean is_active;
    const gchar *ctl_key;

    /* Retrieve the current and reset values */
    qctrl = g_object_get_data(G_OBJECT (ctl_widget), "control");
    curr_idx = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(ctl_widget), "index"));
    is_active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (ctl_widget));

    ctl_key = gtk_widget_get_name(ctl_widget);

    if (get_reset_val(ctl_key, qctrl, action, &reset_idx) == FALSE)
    	return;

    /* Reset the control if necessary */
    if (curr_idx == reset_idx && is_active == FALSE)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctl_widget), TRUE);	// Trigger callback

    return;
}


/* Determine the reset value (action: d - default, l - last saved) */

int get_reset_val(const gchar *ctl_key, struct v4l2_queryctrl *qctrl, char action, long *val)
{
    char *p;

    switch (action)
    {
    	case 'd':
	    *val = qctrl->default_value;
	    break;

    	case 'l':
	    get_session_reset((char *) ctl_key, &p);

	    if (p != NULL)
		*val = atol(p);
	    else
		*val = qctrl->default_value;

	    break;

    	default:
	    return FALSE;
    }

    return TRUE;
}


// Setting an 'auto' control may have an impact on the availability of the manual equivalent
// Eg. White Balance Temp, Auto and White Balance Temp - the latter is only enabled when auto is off

void cam_auto_reset(GtkWidget *radio_btn, 
		    struct v4l2_queryctrl *qctrl, 
		    long idx, 
		    camera_t *cam, GtkWidget *window)
{
    struct v4l2_queryctrl *curr_qctrl; 
    struct v4l2_queryctrl tmp_qctrl; 
    const gchar *nm;
    GtkWidget *parent, *tmp, *ctl_widget;
    GList *ctl_list = NULL;
    int reset_flg, sens;
    
    /* Only applies to booleans */
    if (qctrl->type != V4L2_CTRL_TYPE_BOOLEAN)
    	return;

    /* Find the main parent widget */
    tmp = radio_btn;

    while(tmp != NULL)
    {
    	parent = find_parent(tmp);
    	nm = gtk_widget_get_name(parent);

    	if (strncmp(nm, "ctrl_cntr", 9) == 0)
	    break;
	else
	    tmp = parent; 
    }

    /* Iterate all the relevant widgets */
    ctl_list = ctrl_widget_list(parent, window);
    ctl_list = g_list_first(ctl_list);

    while(ctl_list != NULL)
    {
	ctl_widget = ctl_list->data;

	if (ctl_widget == radio_btn)		// Ignore the current widget
	{
	    ctl_list = g_list_next(ctl_list);
	    continue;
	}

	/* If widget is resetable, get the current sensitivity and reload the query control */
	reset_flg = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (ctl_widget), "resetable"));

	if (reset_flg)
	{
	    sens = gtk_widget_get_sensitive(ctl_widget);
	    curr_qctrl = g_object_get_data (G_OBJECT(ctl_widget), "control");

	    if (get_cam_ctrl(curr_qctrl->id, &tmp_qctrl, cam, window) == FALSE)
		break;

	    if (tmp_qctrl.flags == 0 || tmp_qctrl.flags & V4L2_CTRL_FLAG_SLIDER) 	    // Possible enable
	    {
		if ((curr_qctrl->flags & V4L2_CTRL_FLAG_INACTIVE) && (sens == FALSE))
		{
		    gtk_widget_set_sensitive(ctl_widget, TRUE);
		    curr_qctrl->flags = tmp_qctrl.flags;
		}
	    }
	    else if (curr_qctrl->flags == 0 || curr_qctrl->flags & V4L2_CTRL_FLAG_SLIDER)   // Possible disable
	    {
		if ((tmp_qctrl.flags & V4L2_CTRL_FLAG_INACTIVE) && (sens == TRUE))
		{
		    gtk_widget_set_sensitive(ctl_widget, FALSE);
		    curr_qctrl->flags = tmp_qctrl.flags;
		}
	    }
	}

	ctl_list = g_list_next(ctl_list);
    }

    return;
}


/* Get and optionally update the current camera colour and frame size format */

int cam_fmt_update(CamData *cam_data, char *res_str)
{
    struct v4l2_fmtdesc *vfmt; 
    struct v4l2_format rfmt;
    //int width, height;
    long width, height;
    char *fourcc;
    pixelfmt pxlfmt;

    /* Load the format */
    if (cam_fmt_read(cam_data, &rfmt, &vfmt, FALSE) != TRUE)
    	return FALSE;

    /* Set the values */
    if (vfmt != NULL)
    {
	pxlfmt = vfmt->pixelformat;
    }
    else
    {
	get_session(CLRFMT, &fourcc);
	pxlfmt = fourcc2pxl(fourcc);
    }

    res_to_long(res_str, &width, &height);

    /* Check for changes */
    if (width == rfmt.fmt.pix.width && 
    	height == rfmt.fmt.pix.height &&
    	pxlfmt == rfmt.fmt.pix.pixelformat)
    {
	xv4l2_close(cam_data->cam);
    	return TRUE;
    }

    /* Set the new format values */
    rfmt.fmt.pix.pixelformat = pxlfmt;
    rfmt.fmt.pix.width = width;
    rfmt.fmt.pix.height = height;

    if (set_cam_fmt(cam_data->cam, &rfmt, NULL) != TRUE)
    	return FALSE;

    return TRUE;

    /* Debug
    char fcc[5];
    pxl2fourcc(rfmt.fmt.pix.pixelformat, fcc);
    printf("%s cam_fmt_update Format values - width %d  height %d  pixel %s\n",
    		debug_hdr, rfmt.fmt.pix.width, rfmt.fmt.pix.height, fcc);
    */
}


/* Read the current camera colour and frame size format */

int cam_fmt_read(CamData *cam_data, 
		 struct v4l2_format *rfmt, 
		 struct v4l2_fmtdesc **vfmt, 
		 int close_cam)
{
    struct v4l2_list *v_node;
    struct v4l2_fmtdesc *p; 
    char *fourcc;

    /* Match the format */
    get_session(CLRFMT, &fourcc);
    v_node = find_fmt(cam_data, &p, fourcc);

    /* Get the current format */
    if (v_node != NULL)
    {
	*vfmt = p;
	rfmt->type = (*vfmt)->type;
    }
    else
    {
	rfmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	*vfmt = NULL;
    }

    if (get_cam_fmt(cam_data->cam, rfmt, NULL, close_cam) != TRUE)
	return FALSE;

    return TRUE;
}


/* Get the current camera colour and frame size format */

int get_cam_fmt(camera_t *cam, struct v4l2_format *fmt, GtkWidget *window, int close_cam)
{
    /* Open the camera */
    if ((cam->fd = cam_open(cam->video_dev, O_RDWR, window)) == -1)
	return FALSE;

    /* Get the current settings */
    if (xioctl(cam->fd, VIDIOC_G_FMT, fmt) != 0)
    {
	sprintf(app_msg_extra, "Camera Format Error: (%d) %s", errno, strerror(errno)); 
	log_msg("CAM0013", NULL, "SYS9009", window);
	xv4l2_close(cam);
	return FALSE;
    }

    if (close_cam == TRUE)
	xv4l2_close(cam);

    return TRUE;

    /* Debug
    char fcc[5];
    pxl2fourcc(fmt->fmt.pix.pixelformat, fcc);
    printf("%s Format values (read) - width %d  height %d  pixel %s\n",
    		debug_hdr, fmt->fmt.pix.width, fmt->fmt.pix.height, fcc);
    */
}


/* Set the current camera data format - assumes camera device has been opened */

int set_cam_fmt(camera_t *cam, struct v4l2_format *fmt, GtkWidget *window)
{
    if (xioctl(cam->fd, VIDIOC_S_FMT, fmt) != 0)
    {
	sprintf(app_msg_extra, "Set Format Error: (%d) %s", errno, strerror(errno)); 
	log_msg("CAM0014", NULL, "SYS9009", window);
	xv4l2_close(cam);
	return FALSE;
    }

    xv4l2_close(cam);

    /* Debug
    char fcc[5];
    pxl2fourcc(fmt->fmt.pix.pixelformat, fcc);
    printf("%s Format values (write) - width %d  height %d  pixel %s\n",
    		debug_hdr, fmt->fmt.pix.width, fmt->fmt.pix.height, fcc);
    */

    return TRUE;
}


/* Get and optionally update the current frame interval */

int cam_fps_update(CamData *cam_data, char *fps_str)
{
    struct v4l2_frmivalenum *vfrmival;
    struct v4l2_streamparm s_parm;

    /* Load the frame interval */
    if (cam_fps_read(cam_data, fps_str, &s_parm, &vfrmival, FALSE) != TRUE)
    	return FALSE;

    /* Check for changes and set new values if required */
    if (vfrmival != NULL)
    {
    	if (s_parm.parm.capture.timeperframe.numerator == vfrmival->discrete.numerator &&
	    s_parm.parm.capture.timeperframe.denominator == vfrmival->discrete.denominator)
	{
	    xv4l2_close(cam_data->cam);
	    return TRUE;
    	}

    	s_parm.parm.capture.timeperframe.numerator = vfrmival->discrete.numerator;	// Stepwise ?
	s_parm.parm.capture.timeperframe.denominator = vfrmival->discrete.denominator;
    }
    else
    {
	s_parm.parm.capture.timeperframe.numerator = 1;
	s_parm.parm.capture.timeperframe.denominator = atol(fps_str);
    }

    if (set_cam_streamparm(cam_data->cam, &s_parm, NULL) != TRUE)
    	return FALSE;

    /* Debug
    printf("%s Interval values (upd) - num %d  denom %d\n",
    		debug_hdr, vfrmival->discrete.numerator, vfrmival->discrete.denominator);
    printf("%s Interval values (old) num %d  denom %d\n", debug_hdr,
	        s_parm.parm.capture.timeperframe.numerator, s_parm.parm.capture.timeperframe.denominator);
    */

    return TRUE;
}


/* Read the current camera frame interval */

int cam_fps_read(CamData *cam_data, 
		 char *fps_str,
		 struct v4l2_streamparm *s_parm, 
		 struct v4l2_frmivalenum **vfrmival,
		 int close_cam)
{
    struct v4l2_list *v_node;
    struct v4l2_fmtdesc *vfmt; 
    struct v4l2_frmsizeenum *vfrm;
    struct v4l2_frmivalenum *p;
    char *fourcc;
    char *res_str;

    /* Match the format */
    get_session(CLRFMT, &fourcc);
    v_node = find_fmt(cam_data, &vfmt, fourcc);

    if (v_node != NULL)
	s_parm->type = vfmt->type;
    else
	s_parm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (get_cam_streamparm(cam_data->cam, s_parm, NULL, close_cam) != TRUE)
	return FALSE;

    if (v_node == NULL)
	return TRUE;

    /* Match the resolution */
    get_session(RESOLUTION, &res_str);
    v_node = find_frm(cam_data, v_node, &vfrm, res_str);

    /* Match the frame interval */
    if (v_node != NULL)
    {
	v_node = find_frmival(cam_data, v_node, &p, fps_str);
	*vfrmival = p;
    }
    else
    {
    	*vfrmival = NULL;
    }

    return TRUE;
}


/* Get the current camera streaming parameters */

int get_cam_streamparm(camera_t *cam, struct v4l2_streamparm *s_parm, GtkWidget *window, int close_cam)
{
    /* Open the camera */
    if ((cam->fd = cam_open(cam->video_dev, O_RDWR, window)) == -1)
	return FALSE;

    /* Get the current settings */
    if (xioctl(cam->fd, VIDIOC_G_PARM, s_parm) != 0)
    {
	sprintf(app_msg_extra, "Streaming Parameter Error: (%d) %s", errno, strerror(errno)); 
	log_msg("CAM0015", NULL, "SYS9009", window);
	xv4l2_close(cam);
	return FALSE;
    }

    if (close_cam == TRUE)
	xv4l2_close(cam);

    /* Debug
    printf("%s s_parm values - capability %d, numerator %d  denominator %d\n", debug_hdr,
    		s_parm->parm.capture.capability,
    		s_parm->parm.capture.timeperframe.numerator,
    		s_parm->parm.capture.timeperframe.denominator);
    */

    return TRUE;
}


/* Set the camera streaming parameters - assumes camera device has been opened */

int set_cam_streamparm(camera_t *cam, struct v4l2_streamparm *s_parm, GtkWidget *window)
{
    if (xioctl(cam->fd, VIDIOC_S_PARM, s_parm) != 0)
    {
	sprintf(app_msg_extra, "Set Streaming Parameters Error: (%d) %s", errno, strerror(errno)); 
	log_msg("CAM0016", NULL, "SYS9009", window);
	xv4l2_close(cam);
	return FALSE;
    }

    xv4l2_close(cam);

    return TRUE;
}


/* Match a selected colour format with the camera capabilities */

struct v4l2_list * find_fmt(CamData *cam_data,
			    struct v4l2_fmtdesc **vfmt,
			    char *cc_sel)
{
    struct v4l2_list *v_node;
    char fourcc[10];

    v_node = cam_data->cam->fmt_head;
    fourcc[0] = '\0';
    *vfmt = NULL;

    while(v_node != NULL)
    {
    	*vfmt = (struct v4l2_fmtdesc *) v_node->v4l2_data;
    	pxl2fourcc((*vfmt)->pixelformat, fourcc);

    	if (strcmp(fourcc, cc_sel) == 0)
	    break;

    	v_node = v_node->next;
    }

    return v_node;
}


/* Match a selected resolution if possible */

struct v4l2_list * find_frm(CamData *cam_data,
			    struct v4l2_list *v_node_start,
			    struct v4l2_frmsizeenum **vfrm,
			    char *s)
{
    struct v4l2_list *v_node;
    long width, height;

    v_node = v_node_start->sub_list_head;
    res_to_long(s, &width, &height);
    *vfrm = NULL;

    while(v_node != NULL)
    {
    	*vfrm = (struct v4l2_frmsizeenum *) v_node->v4l2_data;

    	if ((*vfrm)->discrete.width == width && (*vfrm)->discrete.height == height)
	    break;

    	v_node = v_node->next;
    }

    return v_node;
}


/* Match a selected frame rate if possible */

struct v4l2_list * find_frmival(CamData *cam_data,
				struct v4l2_list *v_node_start,
				struct v4l2_frmivalenum **vfrmival,
				char *s)
{
    struct v4l2_list *v_node;
    int fps;
    char fps_str[10];

    v_node = v_node_start->sub_list_head;
    *vfrmival = NULL;

    while(v_node != NULL)
    {
    	*vfrmival = (struct v4l2_frmivalenum *) v_node->v4l2_data;
    	fps = calc_fps((*vfrmival)->discrete.denominator, (*vfrmival)->discrete.numerator);
    	sprintf(fps_str, "%d", fps);

    	if (strcmp(s, fps_str) == 0)
	    break;

    	v_node = v_node->next;
    }

    return v_node;
}


/* Match a selected frame rate if possible */

int get_fps(CamData *cam_data, char *s)
{
    int fps;
    struct v4l2_fmtdesc *vfmt;
    struct v4l2_streamparm s_parm;
    struct v4l2_list *v_node;

    v_node = find_fmt(cam_data, &vfmt, s);
    fps = 0;

    if (v_node != NULL)
	s_parm.type = vfmt->type;
    else
	s_parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (get_cam_streamparm(cam_data->cam, &s_parm, NULL, TRUE) != TRUE)
	return fps;

    /* If the capability flag is not set, frame rate setting is not supported */
    if (s_parm.parm.capture.capability == V4L2_CAP_TIMEPERFRAME)
    {
	fps = calc_fps(s_parm.parm.capture.timeperframe.denominator, 
		       s_parm.parm.capture.timeperframe.numerator);
    }

    return fps;
}


/* Central call point for ioctl */

int xioctl(int fd, int request, void *arg)
{
    int r;

    do
    {
	r = v4l2_ioctl(fd, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    return r;
}


/* Open camera device */

int cam_open(char *cam_dev, int mode, GtkWidget *window)
{
    int fd;

    if ((fd = v4l2_open(cam_dev, mode, 0)) == -1)
    {	
    	sprintf(app_msg_extra, "Error: %d, %s", errno, strerror(errno));
	log_msg("CAM0003", cam_dev, "SYS9000", window);
	return -1;
    }

    return fd;
}


/* Close camera */

void xv4l2_close(camera_t *cam)
{
    v4l2_close(cam->fd);
    cam->cam_is_open = FALSE;

    return;
}


/* Get the last session value, if any, or the default for a control */

void session_ctrl_val(struct v4l2_queryctrl *qctrl, char *key, long *val)
{
    char *p;

    sprintf(key, "ctl-%d", qctrl->id - V4L2_CID_BASE);
    get_session(key, &p);

    if (p != NULL)
    {
	*val = atol(p);

	if (*val < qctrl->minimum || *val > qctrl->maximum)
	    *val = qctrl->default_value;
    }
    else
    {
	*val = qctrl->default_value;
    }

    return;
}


/* Set the camera for this control value and store for the session */

void save_ctrl(struct v4l2_queryctrl *qctrl, 
	       char *ctl_key, long ctl_val, 
	       CamData *cam_data, GtkWidget *window) 
{
    char s[10];

    if (set_cam_ctrl(cam_data->cam, qctrl, ctl_val, window) != FALSE)
    {
	sprintf(s, "%ld", ctl_val);
	set_session(ctl_key, s);
    }
	
    return;
}


/* Clear the resources for the camera list */

void clear_camera_list(CamData *cam_data)
{
    struct camlistNode *tmp;
    camera_t *cam;

    /* Clean up memory allocations on camera menu items */
    tmp = cam_data->camlist;

    while (tmp != NULL)
    {
	/* Free the controls, menus, formats, frame sizes and frame rates lists */
	cam = tmp->cam;
	free_cam_data(cam->ctl_head);
	free_cam_data(cam->pctl_head);
	free_cam_data(cam->fmt_head);

	/* Free the list entry */
	cam_data->camlist = tmp->next;
	free(tmp);
	tmp = cam_data->camlist;
    }

    head = NULL;

    return;
}


/* Free the camera controls, menus, formats, frame intervals et al list items */

void free_cam_data(struct v4l2_list *v4l2_node)
{
    struct v4l2_list *tmp;

    while(v4l2_node != NULL)
    {
    	if (v4l2_node->sub_list_head != NULL)
	    free_cam_data(v4l2_node->sub_list_head);

    	tmp = v4l2_node->next;
    	free(v4l2_node);
    	v4l2_node = tmp;
    }

    return;
}
