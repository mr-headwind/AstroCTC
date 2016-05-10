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
** Description:	Camera items
**
** Author:	Anthony Buckley
**
** History
**	15-Jan-2014	Initial
**
*/


/* Defines */

#ifndef CAM_HDR
#define CAM_HDR
#endif

/* Includes */

#include <linux/videodev2.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#ifndef CODEC_HDR
#include <codec.h>
#endif


/* Enums */

enum {ENC_PIPELINE, CAPS_PIPELINE, VIEW_PIPELINE };
enum { CAM_MODE_NONE, CAM_MODE_VIEW, CAM_MODE_CAPT, CAM_MODE_SNAP, CAM_MODE_UNDEF };


/* Structure for generic v4l2 linked list */

struct v4l2_list
{
    void *v4l2_data;
    struct v4l2_list *next;
    struct v4l2_list *sub_list_head;
    struct v4l2_list *sub_list_last;
};


/* Structure to hold information about and capabilities of a camera */

typedef struct _camera
{
    int fd;                             /* File descriptor for /dev/videoN */
    int cam_is_open;			/* General purpose flag only used as required */
    char video_dev[100];                /* Device path, i.e. /dev/video0 */
    struct v4l2_capability vcaps;       /* Video capability bit field */
    struct v4l2_list *ctl_head;		/* Controls list head */
    struct v4l2_list *ctl_last;		/* Controls list end */
    struct v4l2_list *pctl_head;	/* Private controls list head */
    struct v4l2_list *pctl_last;	/* Private controls list end */
    struct v4l2_list *fmt_head;		/* Formats list head */
    struct v4l2_list *fmt_last;		/* Formats list end */
    struct v4l2_buffer vbuf;            /* Video buffer */
} camera_t;


/* Linked list of cameras */

struct camlistNode
    {
    camera_t *cam;
    struct camlistNode *next;
    };


/* Structure to share the state between prepare and render for gst cairo overlay */

typedef struct _cairo_overlay_state
{
    gboolean valid;
    GstVideoInfo vinfo;
    int draw_handler;
    int info_handler;
} CairoOverlayState;


/* Snapshot capture details */

typedef struct _ImgCapture
{
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    struct buffer *buffers;
    unsigned int n_buffers;
    char io_method;
    unsigned char *tmp_img_buf;
    long width;
    long height;
    long img_sz_bytes;
    const gchar *obj_title;
    char *codec;					// Preferences
    unsigned int jpeg_quality;				// Preferences
    int delay;						// Preferences
    int delay_grp;					// Preferences
    int fits_bits;					// Preferences
    char *locn;						// Preferences
    char id;						// Preferences
    char tt;						// Preferences
    char ts;						// Preferences
} snap_capt_t;


/* Video capture details */

typedef struct _VideoCapture
{
    char tm_stmp[50];
    char out_name[256];
    char fn[100];
    codec_t *codec_data;
    const gchar *obj_title;
    char cam_fcc[5];					// Preferences
    char *codec;					// Preferences
    char *locn;						// Preferences
    char id;						// Preferences
    char tt;						// Preferences
    char ts;						// Preferences
} video_capt_t;


/* Structure to group GST elements */

typedef struct _app_gst_objects
{
    GstElement *v4l2_src, *vid_rate, *v_filter, *v_convert, *v_sink;	// View only
    GstElement *tee, *video_queue, *capt_queue, *muxer, *file_sink;	// Fixed capture
    GstElement *c_convert;						// Fixed capture
    GstElement *encoder; 						// Encoder capture
    GstElement *c_filter;						// Caps capture
    GstElement *q1; 							// Reticule (insertion) related
    GstPad *tee_capt_pad, *tee_video_pad;
    GstCaps *v_caps, *c_caps;						
    GstElement *cairo_overlay, *cairo_convert;				// Cairo elements for reticule
    GstPad *blockpad;							// Reticule only
    gulong probe_id;							// Reticule only
    CairoOverlayState *overlay_state;					// Reticule only
} app_gst_objects; 


/* Structure to contain all our information, so we can pass it around */

typedef struct _CamData
{
    GstElement *pipeline;           	/* Our one and only pipeline */
    int pipeline_type;			/* View, capture */
    int mode;				/* Camera mode */
    app_gst_objects gst_objs;		/* GST objects for viewing & recording */
    GstState state;                 	/* Current state of the pipeline */
    char current_dev[256];	    	/* Device path for current camera */
    char current_cam[256];	    	/* Name of current camera */
    camera_t *cam;			/* Information about the current camera */
    char *info_file;			/* Points to device last written to .cam_info if any */
    struct camlistNode *camlist;	/* Pointer to head of camera list */
    GdkPixbuf *pixbuf;			/* Snapshot usage */
    int status;				/* General purpose */
    int cam_count;			/* General purpose */
    int cam_max;			/* General purpose */
    union
    {
	video_capt_t v_capt;		/* Video capture details */
	snap_capt_t s_capt;		/* Snapshot capture details */
    } u;
} CamData;


/* Provide a type for pixelformat to remove a level from v4l2 */

typedef __u32 pixelfmt;
