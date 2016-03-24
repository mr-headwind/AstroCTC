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
** Description: Capture a snapshot from a camera
**
** Author:	Anthony Buckley
**
** History
**	15-Jul-2014	Initial code
**
*/


/* Includes */

#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <jpeglib.h>
#include <png.h>
#include <fitsio.h>
#include <cairo/cairo.h>
#include <pthread.h>
#include <main.h>
#include <cam.h>
#include <preferences.h>
#include <defs.h>
#include <session.h>




/* Defines */

/* Structures and Typedefs required */

struct buffer
{
    void *start;
    size_t length;
};


typedef struct ImgCapture
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
} capture_t;


typedef struct SnapArgs
{
    CamData *cam_data; 
    MainUi *m_ui; 
    int snap_count; 
    int delay; 
    int delay_grp;
    const gchar *obj_title;
} snap_args_t;

enum { SN_FAIL, SN_SUCCESS, SN_CANCEL, SN_IN_PROGRESS, SN_DONE };


/* Prototypes */
int snap_control(CamData *, MainUi *, int, int, int);
void snap_status(CamData *, MainUi *);
gboolean snap_main_loop_fn(gpointer);
void cancel_snapshot(MainUi *);
void * snap_main(void *);
int snap_init(capture_t *, snap_args_t *, CamData *, MainUi *);
static void load_prefs(capture_t *);
int snap_image(capture_t *, CamData *, MainUi *);
int streaming_io(capture_t *, CamData *, MainUi *);
int mmap_io(capture_t *, CamData *, MainUi *);
int usrptr_io(capture_t *, CamData *, MainUi *);
int read_io(capture_t *, CamData *, MainUi *);
int start_capture(capture_t *, CamData *, MainUi *);
int stop_capture(CamData *, MainUi *);
int image_capture(capture_t *, CamData *, MainUi *);
int image_output(int, char *, capture_t *, MainUi *);
int std_format(char *, capture_t *, MainUi *);
int fits_file(char *, capture_t *, MainUi *);
void jpeg_file(FILE *, capture_t *);
void bmp_file(FILE *, capture_t *);
int png_file(FILE *, capture_t *, MainUi *);
void ppm_file(FILE *, capture_t *);
char * bmp_header(capture_t *);
char * dib_header(capture_t *);
unsigned char * img_start_sz(int *, capture_t *);
void img_row(unsigned char *, unsigned char *, int);
void snap_final(capture_t *, CamData *, MainUi *);
void show_buffer(int, capture_t *, MainUi *, CamData *);
int check_cancel(int *, CamData *, MainUi *);
GdkPixbufDestroyNotify destroy_px (guchar *, gpointer);
int write_24_to_32_bpp(fitsfile *, long, long, capture_t *, MainUi *);
int write_24_to_16_bpp(fitsfile *, long, long, capture_t *, MainUi *);
int snap_mutex_lock();	
int snap_mutex_trylock();
int snap_mutex_unlock();	

extern int gst_view(CamData *, MainUi *);
extern int cam_open(char *, int, GtkWidget *);
extern void xv4l2_close(camera_t *);
extern int xioctl(int, int, void *);
extern void log_msg(char*, char*, char*, GtkWidget*);
extern void dttm_stamp(char *, size_t);
extern int get_user_pref(char *, char **);
extern void get_file_name(char *, int, char *, char *, char, char, char);
extern int64_t msec_time();
extern void set_capture_btns(MainUi *, int, int);
extern void printBits(size_t const, void const * const);
extern int view_clear_pipeline(CamData *, MainUi *);
extern void get_session(char*, char**);
extern void res_to_long(char *, long *, long *);
extern void pxl2fourcc(pixelfmt, char *);
extern int check_dir(char *);


/* Globals */

static const char *debug_hdr = "DEBUG-snapshot.c ";
static const int hdr_sz = 14;
static const int dib_sz = 40;
unsigned char *bgr_data;
static int ret_snap;
static pthread_t snap_tid;
static int cancel_indi;
static pthread_mutex_t snap_mutex = PTHREAD_MUTEX_INITIALIZER;	


// Control taking snapshots. Need to attach a timer function to the main (gtk) loop
// as GTK calls from threads are not thread safe or have been deprecated.
// Set up the snapshot basics, set the timer function and start the thread

int snap_control(CamData *cam_data, MainUi *m_ui, int snap_count, int delay, int delay_grp)
{
    snap_args_t *snap_args;
    int p_err;
    guint id;

    /* Wipe the current pipeline (free all the resources) */
    if (view_clear_pipeline(cam_data, m_ui) == FALSE)
        return FALSE;

    /* Start snapshot capture */
    cam_data->cam_count = 0;
    cam_data->cam_max = snap_count;

    snap_args = malloc(sizeof(snap_args_t));
    snap_args->cam_data = cam_data;
    snap_args->m_ui = m_ui;

    snap_args->snap_count = snap_count;
    snap_args->delay = delay;
    snap_args->delay_grp = delay_grp;
    snap_args->obj_title = gtk_entry_get_text( GTK_ENTRY (m_ui->obj_title));

    if ((p_err = pthread_create(&snap_tid, NULL, &snap_main, (void *) snap_args)) != 0)
    {
	sprintf(app_msg_extra, "Error: %s", strerror(p_err));
	log_msg("SYS9016", NULL, "SYS9016", m_ui->window);
	free(snap_args);
    	cam_data->status = SN_FAIL;
	snap_status(cam_data, m_ui);
	gst_view(cam_data, m_ui);
	return p_err;
    }

    /* Initiate a timer function on the main loop */
    id = g_timeout_add (100, snap_main_loop_fn, m_ui);

    /* Enable or disable screen buttons as appropriate */
    set_capture_btns(m_ui, FALSE, TRUE);
    gtk_widget_set_sensitive (m_ui->cap_pause, FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (m_ui->cap_pause_tb), FALSE);

    return p_err;
}


/* Timeout function on main loop update the display */

gboolean snap_main_loop_fn(gpointer user_data)
{
    CamData *cam_data;
    MainUi *m_ui;

    /* Get data */
    m_ui = (MainUi *) user_data;
    cam_data = (CamData *) g_object_get_data (G_OBJECT(m_ui->window), "cam_data");

    /* Check status */
    if (cam_data->status == SN_IN_PROGRESS)
    {
	snap_status(cam_data, m_ui);
	gtk_widget_queue_draw (m_ui->video_window);
	return TRUE;
    }
    else if (cam_data->status == SN_DONE)
    {
	return TRUE;
    }
    else
    {
	snap_status(cam_data, m_ui);
	snap_mutex_lock();
	gst_view(cam_data, m_ui);
	snap_mutex_unlock();
	return FALSE;
    }
}


/* Update the status information */

void snap_status(CamData *cam_data, MainUi *m_ui)
{
    char s[100];

    switch (cam_data->status)
    {
    	case SN_IN_PROGRESS:
	    if (cam_data->cam_count < cam_data->cam_max)
	    {
		sprintf(s, "Snapshot %d of %d done (successful)", (cam_data->cam_count + 1), cam_data->cam_max);
		gtk_label_set_text (GTK_LABEL (m_ui->status_info), s);
	    }

	    break;

    	case SN_SUCCESS:
	    gtk_label_set_text (GTK_LABEL (m_ui->status_info), "Snapshot successful");
	    break;

    	case SN_CANCEL:
	    gtk_label_set_text (GTK_LABEL (m_ui->status_info), "Snapshot cancelled");
	    break;

    	case SN_FAIL:
	    gtk_label_set_text (GTK_LABEL (m_ui->status_info), "Snapshot failed");
	    break;

    	case SN_DONE:
	    break;

	default:
	    gtk_label_set_text (GTK_LABEL (m_ui->status_info), "Status unkownn (error)");
    }

    return;
}


/* User has cancelled taking snapshot(s) */

void cancel_snapshot(MainUi *m_ui)
{
    cancel_indi = TRUE;

    return;
}


/* Snapshot main (thread) processing */

void * snap_main(void *arg)
{
    snap_args_t *args;
    capture_t capt;

    args = (snap_args_t *) arg;
    CamData *cam_data = args->cam_data; 
    MainUi *m_ui = args->m_ui; 

    /* Snapshot initial setup */
    if (snap_init(&capt, args, cam_data, m_ui) == FALSE)
    {
	free(args);
    	cam_data->status = SN_FAIL;
	snap_final(&capt, cam_data, m_ui);
	pthread_exit(&ret_snap);
    }

    free(args);

    /* Capture an image */
    if (! snap_image(&capt, cam_data, m_ui))
    	cam_data->status = SN_FAIL;

    /* Resume normal viewing */
    snap_final(&capt, cam_data, m_ui);

    pthread_exit(&ret_snap);
}


/* Close current pipeline, open the device and initialise it for snapshot */

int snap_init(capture_t *capt, snap_args_t *args, CamData *cam_data, MainUi *m_ui)
{
    char *res_str;
    camera_t *cam;
    struct v4l2_format *fmt;
    char fourcc[5];
    unsigned int min;

    /* Set up */
    if (args->snap_count <= 0)
    	cam_data->cam_max = 1;
    else
	cam_data->cam_max = args->snap_count;

    cam = cam_data->cam;
    fmt = &(capt->fmt);
    cam_data->status = SN_FAIL;
    cancel_indi = FALSE;
    cam_data->pixbuf = NULL;

    /* Preferences */
    load_prefs(capt);

    if (check_dir(capt->locn) == FALSE)
    {
	log_msg("APP0006", capt->locn, "APP0006", m_ui->window);
    	return FALSE;
    }

    if (args->delay != -1)
    	capt->delay = args->delay;

    if (capt->delay == 0)
	capt->delay_grp = 0;
    else
	capt->delay_grp = args->delay_grp;

    /* Object title for image file name(s) */
    capt->obj_title = args->obj_title;

    /* Make sure camera is capable of capture */
    if (!(cam->vcaps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
	log_msg("CAM0007", NULL, "CAM0007", m_ui->window);
    	return FALSE;
    }

    if (!(cam->vcaps.capabilities & V4L2_CAP_READWRITE) &&
    	!(cam->vcaps.capabilities & V4L2_CAP_STREAMING))
    {
	log_msg("CAM0007", NULL, "CAM0007", m_ui->window);
    	return FALSE;
    }

    /* Open camera */
    cam->fd = v4l2_open(cam->video_dev, O_RDWR | O_NONBLOCK, 0);

    if (cam->fd < 0)
    {
	sprintf(app_msg_extra, "Error: %d, %s", errno, strerror(errno));
	log_msg("CAM0003", cam->video_dev, "SYS9000", m_ui->window);
	return FALSE;
    }

    /* Set format to RGB24 */
    memset(fmt, 0, sizeof(*fmt));

    get_session(RESOLUTION, &res_str);
    res_to_long(res_str, &(capt->width), &(capt->height));
    capt->img_sz_bytes = capt->width * capt->height * 3;

    fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt->fmt.pix.width = capt->width;
    fmt->fmt.pix.height = capt->height;
    fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    fmt->fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (xioctl(cam->fd, VIDIOC_S_FMT, fmt) == -1)
    {
	sprintf(app_msg_extra, "Error: %d, %s", errno, strerror(errno));
	log_msg("CAM0014", NULL, "CAM0014", m_ui->window);
	return FALSE;
    }

    if (fmt->fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24)
    {
	pxl2fourcc(fmt->fmt.pix.pixelformat, fourcc);
	sprintf(app_msg_extra, "Format is: %s", fourcc);
	log_msg("CAM0017", "Failed to set RGB24 format", "CAM0017", m_ui->window);
	return FALSE;
    }

    if ((fmt->fmt.pix.width != capt->width) || (fmt->fmt.pix.height != capt->height))
    {
	sprintf(app_msg_extra, "Error: Image dimensions being forced to %d x %d (found %ld x %ld)\n",
			       fmt->fmt.pix.width, fmt->fmt.pix.height, capt->width, capt->height);
	log_msg("SYS9013", NULL, "SYS9013",  m_ui->window);
	return FALSE;
    }

    /* Buggy driver paranoia */
    min = fmt->fmt.pix.width * 2;

    if (fmt->fmt.pix.bytesperline < min)
	fmt->fmt.pix.bytesperline = min;

    min = fmt->fmt.pix.bytesperline * fmt->fmt.pix.height;

    if (fmt->fmt.pix.sizeimage < min)
	fmt->fmt.pix.sizeimage = min;

    cam_data->status = SN_IN_PROGRESS;
    cam_data->mode = CAM_MODE_SNAP;

    return TRUE;
}


/* Load user preferences for snapshot and filenames */

static void load_prefs(capture_t *capt)
{
    char *p;

    get_user_pref(IMAGE_TYPE, &p);
    capt->codec = p;

    get_user_pref(JPEG_QUALITY, &p);
    capt->jpeg_quality = atoi(p);

    get_user_pref(FITS_BITS, &p);
    capt->fits_bits = atoi(p);

    get_user_pref(SNAPSHOT_DELAY, &p);
    capt->delay = atoi(p);

    get_user_pref(CAPTURE_LOCATION, &p);
    capt->locn = p;

    get_user_pref(FN_ID, &p);
    capt->id = *p;

    get_user_pref(FN_TITLE, &p);
    capt->tt = *p;

    get_user_pref(FN_TIMESTAMP, &p);
    capt->ts = *p;

    return;
}


/* Release the device buffers, Close off, Rebuild the pipline and restart */

void snap_final(capture_t *capt, CamData *cam_data, MainUi *m_ui)
{
    int i;

    /* Information status */
    cam_data->mode = CAM_MODE_NONE;

    if (cam_data->status == SN_FAIL)
    {
	if (cam_data->pipeline)
	    return;
    }
    else
    {
	cam_data->status = SN_SUCCESS;
    }

    /* Clean up */
    snap_mutex_lock();

    if (capt->io_method == 'M')		// Memory mapping
    {
	for(i = 0; i < capt->n_buffers; i++)
	    v4l2_munmap(capt->buffers[i].start, capt->buffers[i].length);

	free(capt->buffers);
    }
    else if (capt->io_method == 'U')	// User pointer
    {
	for(i = 0; i < capt->n_buffers; i++)
	    free(capt->buffers[i].start);

	free(capt->buffers);
    }
    else if (capt->io_method == 'R')	// Read
    {
	free(capt->buffers[0].start);
	free(capt->buffers);
    }

    free(capt->tmp_img_buf);
    xv4l2_close(cam_data->cam);
    snap_mutex_unlock();

    return;
}


/* Take a snapshot */

int snap_image(capture_t *capt, CamData *cam_data, MainUi *m_ui)
{
    camera_t *cam;

    cam = cam_data->cam;

    /* Determine the input/output method */
    if (cam->vcaps.capabilities & V4L2_CAP_STREAMING)
    {
    	if (! streaming_io(capt, cam_data, m_ui))
	    return FALSE;

	if (! start_capture(capt, cam_data, m_ui))
	    return FALSE;

	if (! image_capture(capt, cam_data, m_ui))
	    return FALSE;

	if (! stop_capture(cam_data, m_ui))
	    return FALSE;
    }
    else
    {
    	if (! read_io(capt, cam_data, m_ui))
	    return FALSE;

	if (! image_capture(capt, cam_data, m_ui))
	    return FALSE;
    }

    return TRUE;
}


/* Use Memory Map for preference, otherwise User Pointer to request buffers */

int streaming_io(capture_t *capt, CamData *cam_data, MainUi *m_ui)
{
    camera_t *cam;

    cam = cam_data->cam;

    /* Try mmap method */
    memset(&(capt->req), 0, sizeof(capt->req));

    capt->req.count = 2;
    capt->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    capt->req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(cam->fd, VIDIOC_REQBUFS, &(capt->req)) == 0)
    {
    	capt->io_method = 'M';

	if (! mmap_io(capt, cam_data, m_ui))
	    return FALSE;
	else
	    return TRUE;
    }
    
    /* Try user pointer method */
    memset(&(capt->req), 0, sizeof(capt->req));

    capt->req.count = 2;
    capt->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    capt->req.memory = V4L2_MEMORY_USERPTR;

    if (xioctl(cam->fd, VIDIOC_REQBUFS, &(capt->req)) == 0)
    {
	capt->io_method = 'U';

	if (! usrptr_io(capt, cam_data, m_ui))
	    return FALSE;
	else
	    return TRUE;
    }
    	
    /* Should not get to this point ! */
    sprintf(app_msg_extra,
	    "Camera does not support memory mapping or user pointer IO.\n%s\n",
	    strerror(errno));
    log_msg("CAM0017", "Request buffers", "CAM0017", m_ui->window);

    return FALSE;
}


/* Memory mapping io */

int mmap_io(capture_t *capt, CamData *cam_data, MainUi *m_ui)
{
    camera_t *cam;

    /* Initial */
    if (capt->req.count < 2)
    {
	sprintf(app_msg_extra, "Insufficient buffer memory.\n%s\n", strerror(errno));
	log_msg("CAM0017", "Request buffers", "CAM0017", m_ui->window);
    	return FALSE;
    }

    cam = cam_data->cam;
    capt->buffers = calloc(capt->req.count, sizeof(*(capt->buffers)));

    /* Query the buffer status and map device memory into app. address space */
    for(capt->n_buffers = 0; capt->n_buffers < capt->req.count; ++capt->n_buffers)
    {
	memset(&(capt->buf), 0, sizeof(capt->buf));

	capt->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	capt->buf.memory = V4L2_MEMORY_MMAP;
	capt->buf.index = capt->n_buffers;

	if (xioctl(cam->fd, VIDIOC_QUERYBUF, &(capt->buf)) == -1)
	{
	    sprintf(app_msg_extra, "Problem with buffer status: %s\n", strerror(errno));
	    log_msg("CAM0017", "Query buffer status", "CAM0017", m_ui->window);
	    return FALSE;
	}

	capt->buffers[capt->n_buffers].length = capt->buf.length;
	capt->buffers[capt->n_buffers].start = v4l2_mmap(NULL, capt->buf.length,
							 PROT_READ | PROT_WRITE,
							 MAP_SHARED,
							 cam->fd,
							 capt->buf.m.offset);

	if (MAP_FAILED == capt->buffers[capt->n_buffers].start)
	{
	    sprintf(app_msg_extra, "Problem with memory map: %s\n", strerror(errno));
	    log_msg("CAM0017", "Map device memory", "CAM0017", m_ui->window);
	    return FALSE;
	}
    }

    return TRUE;
}


/* User Pointer io */

int usrptr_io(capture_t *capt, CamData *cam_data, MainUi *m_ui)
{
    camera_t *cam;
    unsigned int page_size;
    unsigned int buffer_size;

    /* Initial */
    if (capt->req.count < 2)
    {
	sprintf(app_msg_extra, "Insufficient buffer memory.\n%s\n", strerror(errno));
	log_msg("CAM0017", "Request buffers", "CAM0017", m_ui->window);
    	return FALSE;
    }

    cam = cam_data->cam;
    capt->buffers = calloc(capt->req.count, sizeof(*(capt->buffers)));

    page_size = getpagesize();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

    /* Align allocate the buffers */
    for(capt->n_buffers = 0; capt->n_buffers < capt->req.count; ++capt->n_buffers)
    {
	capt->buffers[capt->n_buffers].length = buffer_size;
	capt->buffers[capt->n_buffers].start = memalign(page_size, buffer_size);

	if (! capt->buffers[capt->n_buffers].start)
	{
	    sprintf(app_msg_extra, "User pointer memory error: %s\n", strerror(errno));
	    log_msg("CAM0017", "Map device memory", "CAM0017", m_ui->window);
	    return FALSE;
	}
    }

    return TRUE;
}


/* Read / Write io method */

int read_io(capture_t *capt, CamData *cam_data, MainUi *m_ui)
{
    capt->io_method = 'R';
    capt->buffers = calloc(1, sizeof(*(capt->buffers)));

    capt->buffers[0].length = capt->fmt.fmt.pix.sizeimage;
    capt->buffers[0].start = malloc(capt->fmt.fmt.pix.sizeimage);

    if (! capt->buffers[0].start)
    {
	sprintf(app_msg_extra, "Read IO memory error: %s\n", strerror(errno));
	log_msg("CAM0017", "No memory", "CAM0017", m_ui->window);
	return FALSE;
    }

    return TRUE;
}


/* Start streaming */

int start_capture(capture_t *capt, CamData *cam_data, MainUi *m_ui)
{
    unsigned int i;
    camera_t *cam;
    enum v4l2_buf_type type;

    cam = cam_data->cam;

    /* Buffer exchange with driver (enqueue an empty buffer) */
    for(i = 0; i < capt->n_buffers; ++i)
    {
	memset(&(capt->buf), 0, sizeof(capt->buf));

	capt->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	capt->buf.index = i;

	if (capt->io_method == 'M')
	{
	    capt->buf.memory = V4L2_MEMORY_MMAP;
	}
	else
	{
	    capt->buf.memory = V4L2_MEMORY_USERPTR;
	    capt->buf.m.userptr = (unsigned long) capt->buffers[i].start;
	    capt->buf.length = capt->buffers[i].length;
	}

	if (xioctl(cam->fd, VIDIOC_QBUF, &(capt->buf)) == -1)
	{
	    sprintf(app_msg_extra, "Problem with enqueue: %s\n", strerror(errno));
	    log_msg("CAM0017", "Enqueue a buffer", "CAM0017", m_ui->window);
	    return FALSE;
	}
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(cam->fd, VIDIOC_STREAMON, &type) == -1)
    {
	sprintf(app_msg_extra, "Problem setting Streaming on: %s\n", strerror(errno));
	log_msg("CAM0017", "Streaming on", "CAM0017", m_ui->window);
	return FALSE;
    }

    return TRUE;
}


/* Image capture */

int image_capture(capture_t *capt, CamData *cam_data, MainUi *m_ui)
{
    camera_t *cam;
    fd_set fds;
    struct timeval tv;
    int i, r, grp_cnt;
    char tm_stmp[50];
    int64_t cur_msecs, delay_msecs;

    /* Allow for a sequence of image captures */
    cam = cam_data->cam;
    dttm_stamp(tm_stmp, sizeof(tm_stmp));

    /* Set up a temporary image data area */
    capt->tmp_img_buf = (unsigned char *) malloc(capt->img_sz_bytes);
    memset(capt->tmp_img_buf, 0, capt->img_sz_bytes);

    /* Possible delay */
    if (capt->delay > 0)
	delay_msecs = msec_time() + INT64_C(capt->delay * 1000);
    else
    	delay_msecs = 0;

    if (capt->delay_grp > 0)
	grp_cnt = 0;
    else
	grp_cnt = (cam_data->cam_max + 1) * -1;

    i = 0;

    while(i < cam_data->cam_max)
    {
	/* Grab a snapshot */
	do
	{
	    FD_ZERO(&fds);
	    FD_SET(cam->fd, &fds);

	    /* Timeout */
	    tv.tv_sec = 2;
	    tv.tv_usec = 0;

	    /* Synchronous I/O */
	    r = select(cam->fd + 1, &fds, NULL, NULL, &tv);
	} while ((r == -1 && (errno = EINTR)));

	if (r == -1)
	{
	    sprintf(app_msg_extra, "Problem with Select: %s\n", strerror(errno));
	    log_msg("CAM0017", "Select sync I/O", "CAM0017", m_ui->window);
	    return FALSE;
	}

	if (capt->io_method != 'R')
	{
	    /* Buffer exchange with driver (dequeue a filled buffer, then enqueue) */
	    memset(&(capt->buf), 0, sizeof(capt->buf));

	    capt->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	    if (capt->io_method == 'M')
		capt->buf.memory = V4L2_MEMORY_MMAP;
	    else
		capt->buf.memory = V4L2_MEMORY_USERPTR;

	    if (xioctl(cam->fd, VIDIOC_DQBUF, &(capt->buf)) == -1)
	    {
		sprintf(app_msg_extra, "Problem with dequeue: %s\n", strerror(errno));
		log_msg("CAM0017", "Dequeue a buffer", "CAM0017", m_ui->window);
		return FALSE;
	    }

	    /* Write image file if no delay or time has passed */
	    cur_msecs = msec_time();

	    if (cur_msecs >= delay_msecs)
	    {
		if (! image_output(i, tm_stmp, capt, m_ui))
		    return FALSE;

	    	grp_cnt++;

	    	if (grp_cnt >= capt->delay_grp)
	    	{
		    delay_msecs = msec_time() + INT64_C(capt->delay * 1000);
		    grp_cnt = 0;
		}

	    	i++;
	    }

	    /* Push image out to screen */
	    show_buffer(i, capt, m_ui, cam_data);

	    if (xioctl(cam->fd, VIDIOC_QBUF, &(capt->buf)) == -1)
	    {
		sprintf(app_msg_extra, "Problem with enqueue: %s\n", strerror(errno));
		log_msg("CAM0017", "Enqueue a buffer", "CAM0017", m_ui->window);
		return FALSE;
	    }
	}
	else if (capt->io_method == 'R')
	{
	    /* Buffer exchange with driver (read from device) */
	    if (v4l2_read(cam->fd, capt->buffers[0].start, capt->buffers[0].length) == -1)
	    {
		sprintf(app_msg_extra, "Problem with device read: %s\n", strerror(errno));
		log_msg("CAM0017", "Read from camera", "CAM0017", m_ui->window);
		return FALSE;
	    }

	    /* Write image file if no delay or time has passed */
	    cur_msecs = msec_time();

	    if (cur_msecs >= delay_msecs)
	    {
		if (! image_output(i, tm_stmp, capt, m_ui))
		    return FALSE;

	    	grp_cnt++;

	    	if (grp_cnt >= capt->delay_grp)
	    	{
		    delay_msecs = msec_time() + INT64_C(capt->delay * 1000);
		    grp_cnt = 0;
		}

	    	i++;
	    }

	    /* Push image out to screen */
	    show_buffer(i, capt, m_ui, cam_data);
	}

	/* Check for cancellation */
	check_cancel(&i, cam_data, m_ui);
    }

    /* Clean up */
    cam_data->status = SN_DONE;

    return TRUE;
}


/* Stop streaming */

int stop_capture(CamData *cam_data, MainUi *m_ui)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(cam_data->cam->fd, VIDIOC_STREAMOFF, &type) == -1)
    {
	sprintf(app_msg_extra, "Problem setting Streaming off: %s\n", strerror(errno));
	log_msg("CAM0017", "Streaming off", "CAM0017", m_ui->window);
	return FALSE;
    }

    return TRUE;
}


/* Set up image output */

int image_output(int img_id, char *tm_stmp, capture_t *capt, MainUi *m_ui)
{
    char out_name[256];
    char fn[100];

    /* File name */
    get_file_name(fn, img_id, (char *) capt->obj_title, tm_stmp, capt->id, capt->tt, capt->ts);
    sprintf(out_name, "%s/%s.%s", capt->locn, fn, capt->codec);

    /* Main formats or FITS */
    if (strcmp(capt->codec, "fits") != 0)
    	return std_format(out_name, capt, m_ui);
    else
    	return fits_file(out_name, capt, m_ui);
}


/* Write image output file, doing conversion if necessary */

int std_format(char *out_name, capture_t *capt, MainUi *m_ui)
{
    FILE *f_out;

    /* Output */
    f_out = fopen(out_name, "w");

    if (! f_out)
    {
	sprintf(app_msg_extra, "Cannot open output file: %s\n", strerror(errno));
	log_msg("CAM0017", "Cannot open output file", "CAM0017", m_ui->window);
	return FALSE;
    }

    /* Write the image file in the user preferred format */
    if (strcmp(capt->codec, "jpg") == 0)
    	jpeg_file(f_out, capt);			// Jpeg

    else if (strcmp(capt->codec, "bmp") == 0)
    	bmp_file(f_out, capt);			// Bmp 

    else if (strcmp(capt->codec, "png") == 0)
    	png_file(f_out, capt, m_ui);		// Png 
    else
    	ppm_file(f_out, capt);			// Portable

    fclose(f_out);

    /* TEST setup if baseline is needed for debug
    sprintf(out_name, "test.ppm");
    f_out = fopen(out_name, "w");
    ppm_file(f_out, capt);
    fclose(f_out);
    */

    return TRUE;
}


/* Write a jpeg image file */

void jpeg_file(FILE *f_out, capture_t *capt)
{
    unsigned char *img;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    /* Create jpeg data */
    cinfo.err = jpeg_std_error( &jerr );
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, f_out);

    /* Set image parameters */
    cinfo.image_width = capt->width;	
    cinfo.image_height = capt->height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    /* Set jpeg compression parameters to default and adjust quality setting */
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, capt->jpeg_quality, TRUE);

    /* Start compress */
    jpeg_start_compress(&cinfo, TRUE);

    /* Feed data */
    while (cinfo.next_scanline < cinfo.image_height)
    {
	if (capt->io_method == 'M')
	{
	    img = capt->buffers[capt->buf.index].start;
	    row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
	}
	else if (capt->io_method == 'U')
	{
	    img = (void *) capt->buf.m.userptr;
	    row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
	}
	else if (capt->io_method == 'R')
	{
	    img = capt->buffers[0].start;
	    row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
	}					       
	else
	{
	    break;
	}

	jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Finish compression */
    jpeg_finish_compress(&cinfo);

    /* Destroy jpeg data */
    jpeg_destroy_compress(&cinfo);

    return;
}


/* Write a portable pixmap file */

void ppm_file(FILE *f_out, capture_t *capt)
{
    fprintf(f_out, "P6\n%d %d 255\n", capt->fmt.fmt.pix.width, capt->fmt.fmt.pix.height);

    if (capt->io_method != 'R')
	fwrite(capt->buffers[capt->buf.index].start, capt->buf.bytesused, 1, f_out);
    else
	fwrite(capt->buffers[0].start, capt->buffers[0].length, 1, f_out);

    return;
}


/* Write a bmp image file - Keep it simple as could use netpbm for format conversion */

void bmp_file(FILE *f_out, capture_t *capt)
{
    char *bmp_hdr;
    char *dib_hdr;
    unsigned char *bgr_data;
    unsigned char *rgb_data;
    unsigned char *pad;
    int pad_bytes, row_sz;
    int i;
    int img_len;
    const unsigned int c = 0;

    /* Image headers */
    bmp_hdr = bmp_header(capt);
    fwrite(bmp_hdr, hdr_sz, 1, f_out);

    dib_hdr = dib_header(capt);
    fwrite(dib_hdr, dib_sz, 1, f_out);

    // Each row of data must be a multiple of 4 bytes (24bpp)
    // Padding bytes added to end of each row
    pad_bytes = (4 - (capt->width * 3) % 4) % 4; 

    if (pad_bytes > 0)
    {
	pad = malloc(pad_bytes * sizeof(unsigned char));
	memcpy(pad, &c, pad_bytes);
    }

    /* Image data, row at a time */
    row_sz = (capt->width * 3 * sizeof(unsigned char));
    bgr_data = malloc(sizeof(unsigned char) * row_sz);

    rgb_data = img_start_sz(&img_len, capt);

    for(i = 0; i < capt->height; i++)
    {
	img_row(rgb_data, bgr_data, row_sz);
	fwrite(bgr_data, row_sz, 1, f_out);

	if (pad_bytes > 0)
	    fwrite(pad, pad_bytes, 1, f_out);

	rgb_data += row_sz;
    }

    /* Clean up */
    free(bmp_hdr);
    free(dib_hdr);
    free(bgr_data);

    if (pad_bytes > 0)
    	free(pad);

    return;
}


/* Build BMP image header */

char * bmp_header(capture_t *capt)
{
    int i;
    char *hdr;

    hdr = malloc(sizeof(char) * hdr_sz);

    /* Windows style first 2 bytes are 'BM' */
    *hdr = 'B';			
    *(hdr + 1) = 'M';

    /* File size (4 bytes) = header size + dib size + (w * h * bpp) */
    i = hdr_sz + dib_sz + (3 * capt->width * capt->height);
    memcpy((hdr + 2), &i, 4);

    /* Set next 4 bytes to 0 */
    i = 0;
    memcpy((hdr + 6), &i, 4);

    /* Image data offset (4 bytes ) */
    i = hdr_sz + dib_sz;
    memcpy((hdr + 10), &i, 4);

    return hdr;
}


/* Build DIB (information header) */

char * dib_header(capture_t *capt)
{
    int i;
    short j;
    char *hdr;

    hdr = malloc(sizeof(char) * dib_sz);

    /* Dib size (4 bytes) */
    memcpy(hdr, &dib_sz, 4);

    /* Bitmap width in pixels (4 bytes) */
    i = (int) capt->width;
    memcpy((hdr + 4), &i, 4);

    /* Bitmap height in pixels (4 bytes), negative indicates top to bottom order */
    i = -((int) capt->height);
    memcpy((hdr + 8), &i, 4);

    /* Single colour plane (2 bytes ) */
    j = 1;
    memcpy((hdr + 12), &j, 2);

    /* Bits per pixel (2 bytes ) */
    j = 24;
    memcpy((hdr + 14), &j, 2);

    /* No compression (4 bytes ) */
    i = 0;
    memcpy((hdr + 16), &i, 4);

    /* Size of raw bitmap data (4 bytes ) */
    i = 0;
    memcpy((hdr + 20), &i, 4);

    /* Pixels per metre - horizontal resolution (4 bytes ) */
    i = 0;
    memcpy((hdr + 24), &i, 4);

    /* Pixels per metre - vertical resolution (4 bytes ) */
    memcpy((hdr + 28), &i, 4);

    /* Number of colours in colour table - not used if >= 16bpp (4 bytes ) */
    memcpy((hdr + 32), &i, 4);

    /* Number of important colours - 0 for every colour (4 bytes ) */
    memcpy((hdr + 36), &i, 4);

    return hdr;
}


/* Return the start of the image and its size */

unsigned char * img_start_sz(int *sz, capture_t *capt)
{
    unsigned char* img;

    if (capt->io_method == 'M')
    {
	img = capt->buffers[capt->buf.index].start;
	*sz = capt->buf.bytesused;
    }
    else if (capt->io_method == 'U')
    {
	img = (void *) capt->buf.m.userptr;
	*sz = capt->buf.bytesused;
    }

    else if (capt->io_method == 'R')
    {
	img = capt->buffers[0].start;
	*sz = capt->buffers[0].length;
    }
    else
    {
    	*sz = 0;
    	return NULL;
    }

    return img;
}


/* Grab a row of image data and swap from rgb to bgr */

void img_row(unsigned char *rgb_data, unsigned char *bgr_data, int row_sz)
{
    int i;

    i = 0;

    while(i < row_sz)
    {
    	*(bgr_data + i) = *(rgb_data + i + 2);
    	*(bgr_data + i + 1) = *(rgb_data + i + 1);
    	*(bgr_data + i + 2) = *(rgb_data + i);

    	i += 3;
    }

    return;
}


/* Write a portable network graphics file */

int png_file(FILE *f_out, capture_t *capt, MainUi *m_ui)
{
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_byte **row_pointers = NULL;
    unsigned char *rgb_data;
    int img_len, row_sz, incr;

    /* Setup */
    int pixel_size = 3;	
    int depth = 8;

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (! png_ptr)
    {
	log_msg("CAM0017", "png_create_write_struct failed", "CAM0017", m_ui->window);
    	return FALSE;
    }
    
    info_ptr = png_create_info_struct (png_ptr);

    if (! info_ptr)
    {
	log_msg("CAM0017", "png_create_info_struct failed", "CAM0017", m_ui->window);
	png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
    	return FALSE;
    }
    
    /* Error handling */
    if (setjmp (png_jmpbuf (png_ptr)))
    {
	log_msg("CAM0017", "PNG error found", "CAM0017", m_ui->window);
	png_destroy_write_struct (&png_ptr, &info_ptr);
    	return FALSE;
    }

    /* Image attributes */
    png_set_IHDR (png_ptr,
                  info_ptr,
                  capt->width,
                  capt->height,
                  depth,
                  PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);

    /* Initialize rows of PNG */
    row_pointers = png_malloc (png_ptr, capt->height * sizeof(png_byte *));
    rgb_data = img_start_sz(&img_len, capt);
    row_sz = (capt->width * 3 * sizeof(unsigned char));

    for(y = 0; y < capt->height; y++)
    {
        png_byte *row = png_malloc (png_ptr, sizeof(uint8_t) * capt->width * pixel_size);
        row_pointers[y] = row;
        incr = y * row_sz;

        /* Row pixels */
        for(x = 0; x < capt->width; x++)
        {
            *row++ = *(rgb_data + incr + (x * 3));		// Red
            *row++ = *(rgb_data + incr + (x * 3) + 1);		// Green
            *row++ = *(rgb_data + incr + (x * 3) + 2);		// Blue
        }
    }
    
    /* Output */
    png_init_io(png_ptr, f_out);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    for(y = 0; y < capt->height; y++)
    {
        png_free (png_ptr, row_pointers[y]);
    }

    png_free (png_ptr, row_pointers);

    return TRUE;
}


/* Write a FITS format image file */

int fits_file(char *out_name, capture_t *capt, MainUi *m_ui)
{
    fitsfile *f_out;
    int status;
    long  fpixel, no_elements;
    char s[100];
    int bitpix; 

    /* Initialize FITS image parameters */
    if (capt->fits_bits == 32)
    {
	bitpix = ULONG_IMG; 				/* 32-bit int pixel values */
    }
    else if (capt->fits_bits == 16)
    {
	bitpix = USHORT_IMG; 				/* 16-bit unsigned short pixel values */
    }
    else
    {
    	return FALSE;
    }

    long no_axis = 2;					/* 2-dimensional image */    
    long n_axes[2] = { capt->width, capt->height };	/* image dimensions */

    /* Initial setup */
    status = 0;
    fpixel = 1;                               		/* first pixel to write */
    no_elements = capt->width * capt->height;        	/* number of pixels to write */

    /* Create new FITS file */
    if (fits_create_file(&f_out, out_name, &status)) 
    {
	sprintf(s, "fits_create_file failed - status %d", status);
	log_msg("CAM0017", s, "CAM0017", m_ui->window);
    	return FALSE;
    }

    /* Write the required keywords for the primary array image */
    if (fits_create_img(f_out, bitpix, no_axis, n_axes, &status))
    {
	sprintf(s, "fits_create_img failed - status %d", status);
	log_msg("CAM0017", s, "CAM0017", m_ui->window);
    	return FALSE;
    }

    /* Set the image data into the array */
    if (bitpix == ULONG_IMG)
    {
    	if (write_24_to_32_bpp(f_out, fpixel, no_elements, capt, m_ui) == FALSE)
	    return FALSE;
    }
    else if (bitpix == USHORT_IMG)
    {
    	if (write_24_to_16_bpp(f_out, fpixel, no_elements, capt, m_ui) == FALSE)
	    return FALSE;
    }
    else
    {
    	return FALSE;
    }

    /* Clean up */
    if (fits_close_file(f_out, &status)) 
    {
	sprintf(s, "fits_close_file failed - status %d", status);
	log_msg("CAM0017", s, "CAM0017", m_ui->window);
    	return FALSE;
    }

    return TRUE;

/* Debug
printf("r %u, g %hhu, b %u\n", r, g, b);
print_bits(sizeof(r), &r);
print_bits(sizeof(g), &g);
print_bits(sizeof(b), &b);
print_bits(sizeof(uint32_t), pixel);
*/
}


/* Convert 24bpp in image data to 32bpp for FITS format */

int write_24_to_32_bpp(fitsfile *f_out, long fpixel, long no_elements, capture_t *capt, MainUi *m_ui)
{
    int status, i, j;
    unsigned char *rgb_data;
    int img_len;
    uint32_t *pixel;
    unsigned long *array[capt->width];
    char s[100];

    /* Allocate memory for the whole image */ 
    array[0] = (unsigned long *) malloc(capt->width * capt->height * sizeof(unsigned long));
    memset(array[0], 0, capt->width * capt->height);

    /* Initialize pointers to the start of each row of the image */
    for(i = 1; i < capt->height; i++ )
	array[i] = array[i - 1] + capt->width;

    /* Set the image data into the array - as fits has no 24bpp need to convert to 32bpp */
    rgb_data = img_start_sz(&img_len, capt);

    for(j = 0; j < capt->height; j++)
    {   
    	for(i = 0; i < capt->width; i++)
        {
            pixel = (uint32_t *) &(array[j][i]); 
            unsigned char r = *rgb_data++;
            unsigned char g = *rgb_data++;
            unsigned char b = *rgb_data++;

            *pixel |= (r << 24);
            *pixel |= (g << 16);
            *pixel |= (b << 8);
        }
    }

    /* Write the array of long integers to the FITS file */
    if (fits_write_img(f_out, TULONG, fpixel, no_elements, array[0], &status))
    {
	sprintf(s, "fits_write_img failed - status %d", status);
	log_msg("CAM0017", s, "CAM0017", m_ui->window);
    	return FALSE;
    }
      
    /* Clean up */
    free(array[0]);  

    return TRUE;
}


/* Convert 24bpp in image data to 16bpp for FITS format */

int write_24_to_16_bpp(fitsfile *f_out, long fpixel, long no_elements, capture_t *capt, MainUi *m_ui)
{
    int status, i, j;
    unsigned char *rgb_data;
    int img_len;
    uint16_t *pixel;
    unsigned short *array[capt->width];
    char s[100];

    /* Allocate memory for the whole image */ 
    array[0] = (unsigned short *) malloc(capt->width * capt->height * sizeof(unsigned short));
    memset(array[0], 0, capt->width * capt->height);

    /* Initialize pointers to the start of each row of the image */
    for(i = 1; i < capt->height; i++ )
	array[i] = array[i - 1] + capt->width;

    /* Set the image data into the array - as fits has no 24bpp need to convert to 16bpp */
    rgb_data = img_start_sz(&img_len, capt);

    for(j = 0; j < capt->height; j++)
    {   
    	for(i = 0; i < capt->width; i++)
        {
            pixel = (uint16_t *) &(array[j][i]); 
            unsigned char r = *rgb_data++;
            unsigned char g = *rgb_data++;
            unsigned char b = *rgb_data++;

            *pixel |= ((r & 0xF8) << 8);
            *pixel |= ((g & 0xFC) << 3);
            *pixel |= (b >> 3);
        }
    }

    /* Write the array of unsigned integers to the FITS file */
    if (fits_write_img(f_out, TUSHORT, fpixel, no_elements, array[0], &status))
    {
	sprintf(s, "fits_write_img failed - status %d", status);
	log_msg("CAM0017", s, "CAM0017", m_ui->window);
    	return FALSE;
    }
      
    /* Clean up */
    free(array[0]);  

    return TRUE;
}


/* Push image out to be picked up by main loop (thread) for viewing */

void show_buffer(int i, capture_t *capt, MainUi *m_ui, CamData *cam_data)
{
    unsigned char *img;
    GtkAllocation allocation;

    //cam_data->cam_count = i;
    //if (snap_mutex_lock() != 0)

    if (snap_mutex_trylock() != 0)	
    	return;

    cam_data->cam_count = i;

    if (capt->io_method == 'M')
    {
	img = capt->buffers[capt->buf.index].start;
    }
    else if (capt->io_method == 'U')
    {
	img = (void *) capt->buf.m.userptr;
    }
    else if (capt->io_method == 'R')
    {
	img = capt->buffers[0].start;
    }
    else
    {
    	snap_mutex_unlock();
    	return;
    }

    if (cam_data->pixbuf != NULL)
	if (GDK_IS_PIXBUF (cam_data->pixbuf))
	    g_object_unref(G_OBJECT(cam_data->pixbuf));

    memcpy(capt->tmp_img_buf, img, capt->img_sz_bytes);
    gtk_widget_get_allocation (m_ui->video_window, &allocation);
    cam_data->pixbuf = gdk_pixbuf_new_from_data (capt->tmp_img_buf, GDK_COLORSPACE_RGB, FALSE, 8, 
					         allocation.width, allocation.height, allocation.width * 3,
					         NULL, cam_data);
    snap_mutex_unlock();
    
    return;
}


/* Lock the snapshot mutex */

int snap_mutex_lock()
{
    return pthread_mutex_lock(&snap_mutex);
}


/* Lock the snapshot mutex */	

int snap_mutex_trylock()
{
    return pthread_mutex_trylock(&snap_mutex);
}


/* Unock the snapshot mutex */	

int snap_mutex_unlock()
{
    return pthread_mutex_unlock(&snap_mutex);
}


/* Check if user has cancelled */

int check_cancel(int *i, CamData *cam_data, MainUi *m_ui)
{
    if (cancel_indi == FALSE)
    	return FALSE;

    *i = cam_data->cam_max;

    return TRUE;
}
