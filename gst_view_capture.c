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
** Description:	GST Camera viewing and capture
**
** Author:	Anthony Buckley
**
** History
**	24-Jan-2015	Initial code
*/

/*
    The possible pipelines are as follows:

 ** VIEW ** (note convenience 'blk' queue - see reticule)

  | Camera  |  | Video |  | Caps   |  | Queue |  | Video   |  | Video |
  | v4l2src |->| Rate  |->| Filter |->| (blk) |->| convert |->| sink  |-> Screen


 ** CAPTURE 1 (encoder based) ** (note 2 x 'Videoconvert' convenience)

                                                            | Video |  | Video   |  | Video |
                                                         /->| queue |->| convert |->| sink  |-> Screen
  | Camera  |  | Video |  | Caps   |  | Queue |  | Tee |/
  | v4l2src |->| Rate  |->| Filter |->| (blk) |->|     |\   | Capture |  | Video   |  | Encoder |  | Muxer |  | File |
                                                         \->| queue   |->| convert |->|         |->|       |->| sink |-> Video file
                                                                         
                                                                       
 ** CAPTURE 2 (requires a 2nd caps filter) ** 

                                                            | Video |  | Video   |  | Video |
                                                         /->| queue |->| convert |->| sink  |-> Screen
  | Camera  |  | Video |  | Caps   |  | Queue |  | Tee |/
  | v4l2src |->| Rate  |->| Filter |->| (blk) |->|     |\   | Capture |  | Video   |  | Caps   |  | Muxer |  | File |
                                                         \->| queue   |->| convert |->| filter |->|       |->| sink |-> Video file
                                                                         

 ** RETICULE ** (when selected is added to each video thread of the above)

      | Video   |  | Cairo   |  | Video   |  | Video |
  ... | convert |->| overlay |->| convert |->| sink  |-> Screen

                                                                         
 ** SNAPSHOT ** 
  NOTE snapshots do not use gstreamer and instead use the various Video4linux utilities directly.
  This is done for a number of (mostly good) reasons.

*/


/* Defines */

#define GST_VIEW_CAPT


/* Includes */

#include <stdlib.h>  
#include <string.h>  
#include <libgen.h>  
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <gtk/gtk.h>  
#include <gst/gst.h>  
#include <glib.h>  
#include <gst/video/videooverlay.h>
#include <gst/video/video-format.h>
#include <cairo/cairo.h>
#include <session.h>
#include <main.h>
#include <codec.h>
#include <cam.h>
#include <defs.h>
#include <preferences.h>


/* Prototypes */

int gst_view(CamData *, MainUi *);
int gst_view_elements(CamData *, MainUi *);
int link_view_pipeline(CamData *, MainUi *);
int start_view_pipeline(CamData *, MainUi *, int);
int gst_capture(CamData *, MainUi *, int, int);
int gst_capture_init(CamData *, MainUi *, int, int);
int gst_capture_elements(CamData *, MainUi *);
int link_enc_pipeline(CamData *, MainUi *);
int link_caps_pipeline(CamData *, MainUi *);
int start_capt_pipeline(CamData *, MainUi *);
void view_prepare_capt(CamData *, MainUi *);
void capt_prepare_view(CamData *, MainUi *);
int view_clear_pipeline(CamData *, MainUi *);
int cam_set_state(CamData *, GstState, GtkWidget *);
int create_element(GstElement **, char *, char *, CamData *, MainUi *);
void check_unref(GstElement **, char *, int);
void set_capture_btns(MainUi *, int, int);
void swap_fourcc(char *, char *);
void capture_limits(app_gst_objects *, MainUi *);
void set_encoder_props(video_capt_t *, GstElement **, MainUi *); 
static void load_prefs(video_capt_t *);
void set_reticule(MainUi *, CamData *);
int prepare_reticule(MainUi *, CamData *);
int remove_reticule(MainUi *, CamData *);
int init_thread(MainUi *, void *(*start_routine)(void*));
void * monitor_unltd(void *);
void * monitor_duration(void *);
void * monitor_frames(void *);
void * send_EOS(void *);
int set_eos(MainUi *);
void setup_meta(CamData *);
void capture_cleanup();
GstBusSyncReply bus_sync_handler (GstBus*, GstMessage*, gpointer);
gboolean bus_message_watch (GstBus *, GstMessage *, gpointer);
void debug_state(GstElement *, char *,  CamData *);

extern void log_msg(char*, char*, char*, GtkWidget*);
extern void res_to_long(char *, long *, long *);
extern void get_session(char*, char**);
extern void dttm_stamp(char *, size_t);
extern void get_file_name(char *, char *, char *, char *, char, char, char);
extern codec_t * get_codec(char *);
extern int get_user_pref(char *, char **);
extern int codec_property_type(char *, char *);
extern GstPadProbeReturn OnPadProbe (GstPad *, GstPadProbeInfo *, gpointer);
extern void OnPrepReticule (GstElement *, GstCaps *, gpointer);
extern void OnDrawReticule (GstElement *, cairo_t *, guint64, guint64, gpointer);
extern int check_dir(char *);
extern int write_meta_file(char, CamData *, char *);


/* Globals */

static const char *debug_hdr = "DEBUG-gst_view_capture.c ";
static int capt_seq_no = 0;
static int ret_mon, ret_eos;
static pthread_t mon_tid, eos_tid;
static pthread_mutex_t capt_lock_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t capt_eos_cv = PTHREAD_COND_INITIALIZER;

extern guintptr video_window_handle;


/* GST camera viewing */

int gst_view(CamData *cam_data, MainUi *m_ui)
{
    /* Initial */
    if (cam_data->camlist == NULL)
        return FALSE;

    /* GST setup */

    if (!gst_view_elements(cam_data, m_ui))
	return FALSE;

    /* Link all the viewing elements */
    if (link_view_pipeline(cam_data, m_ui) == FALSE)
	return FALSE;

    /* May need to include reticule */
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (m_ui->opt_ret)) == TRUE)
	prepare_reticule(m_ui, cam_data);

    /* Start viewing */
    if (start_view_pipeline(cam_data, m_ui, TRUE) == FALSE)
	return FALSE;

    return TRUE;
}


/* Create pipeline and viewing elements - these will remain fixed as they also apply to capture */

int gst_view_elements(CamData *cam_data, MainUi *m_ui)
{
    long width, height;
    int fps;
    char *p;
    char fourcc[5];

    /* Create the elements */
    /*
    cam_data->gst_objs.v4l2_src = gst_element_factory_make ("v4l2src", "v4l2");
    cam_data->gst_objs.v_convert = gst_element_factory_make ("videoconvert", "v_convert");
    cam_data->gst_objs.v_sink = gst_element_factory_make ("autovideosink", "v_sink");
    cam_data->gst_objs.v_filter = gst_element_factory_make ("capsfilter", "v_filter");
    cam_data->gst_objs.vid_rate = gst_element_factory_make ("videorate", "vid_rate");
    cam_data->gst_objs.q1 = gst_element_factory_make ("queue", "block");
    */

    memset(&(cam_data->gst_objs), 0, sizeof(app_gst_objects));

    if (! create_element(&(cam_data->gst_objs.v4l2_src), "v4l2src", "v4l2", cam_data, m_ui))
    	return FALSE;

    if (! create_element(&(cam_data->gst_objs.v_convert), "videoconvert", "v_convert", cam_data, m_ui))
    	return FALSE;

    //if (! create_element(&(cam_data->gst_objs.v_sink), "glimagesink", "v_sink", cam_data, m_ui))
    //if (! create_element(&(cam_data->gst_objs.v_sink), "xvimagesink", "v_sink", cam_data, m_ui))
    if (! create_element(&(cam_data->gst_objs.v_sink), "ximagesink", "v_sink", cam_data, m_ui))
    	return FALSE;

    if (cam_data->gst_objs.v_sink == NULL)
	if (! create_element(&(cam_data->gst_objs.v_sink), "autovideosink", "v_sink", cam_data, m_ui))
	    return FALSE;

    if (! create_element(&(cam_data->gst_objs.v_filter), "capsfilter", "v_filter", cam_data, m_ui))
    	return FALSE;
    
    if (! create_element(&(cam_data->gst_objs.vid_rate), "videorate", "vid_rate", cam_data, m_ui))
    	return FALSE;

    if (! create_element(&(cam_data->gst_objs.q1), "queue", "block", cam_data, m_ui))
    	return FALSE;

    /* Create the pipeline */
    cam_data->pipeline = gst_pipeline_new ("cam_video");

    if (!cam_data->pipeline)
    {
	log_msg("CAM0020", NULL, "CAM0020", m_ui->window);
        return FALSE;
    }

    /* Specify what kind of video is wanted from the camera */
    get_session(CLRFMT, &p);
    swap_fourcc(p, fourcc);
    get_session(RESOLUTION, &p);
    res_to_long(p, &width, &height);
    get_session(FPS, &p);
    fps = atoi(p);

    cam_data->gst_objs.v_caps = gst_caps_new_simple ("video/x-raw",
						     "format", G_TYPE_STRING, fourcc,
						     "framerate", GST_TYPE_FRACTION, fps, 1,
						     "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
						     "width", G_TYPE_INT, width,
						     "height", G_TYPE_INT, height,
						     NULL);

    /* Set the device source, caps filter and other object properties */
    g_object_set (cam_data->gst_objs.v4l2_src, "device", cam_data->current_dev, NULL);
    g_object_set (cam_data->gst_objs.v_sink, "sync", FALSE, NULL);
    g_object_set (cam_data->gst_objs.v_filter, "caps", cam_data->gst_objs.v_caps, NULL);

    cam_data->gst_objs.blockpad = gst_element_get_static_pad (cam_data->gst_objs.q1, "src");

    /* Build the pipeline - add all the elements */
    gst_bin_add_many (GST_BIN (cam_data->pipeline), 
    				cam_data->gst_objs.v4l2_src, 
    				cam_data->gst_objs.vid_rate, 
    				cam_data->gst_objs.v_sink, 
    				cam_data->gst_objs.v_convert, 
    				cam_data->gst_objs.v_filter, 
    				cam_data->gst_objs.q1, 
    				NULL);

    return TRUE;
}


/* Build the view only pipeline - link all the elements */

int link_view_pipeline(CamData *cam_data, MainUi *m_ui)
{
    app_gst_objects *gst_objs;

    /* Convenience pointer */
    gst_objs = &(cam_data->gst_objs);

    /* Link */
    if (gst_element_link_many (gst_objs->v4l2_src, 
			       gst_objs->vid_rate, 
			       gst_objs->v_filter,
			       gst_objs->q1,
			       gst_objs->v_convert,
			       gst_objs->v_sink,
			       NULL) != TRUE)
    {
	sprintf(app_msg_extra, " - v4l2_src:vid_rate:v_filter (vcaps):convert:sink");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
	return FALSE;
    }

    gst_caps_unref(gst_objs->v_caps);

    return TRUE;
}


/* Pipeline watch and start playing setup */

int start_view_pipeline(CamData *cam_data, MainUi *m_ui, int init)
{
    GstBus *bus;
    guint source_id;
    char s[100];

    if (init == TRUE)
    {
	/* Set up sync handler for setting the xid once the pipeline is started */
	bus = gst_pipeline_get_bus (GST_PIPELINE (cam_data->pipeline));
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler, NULL, NULL);
    }

    if (cam_set_state(cam_data, GST_STATE_PLAYING, m_ui->window) == FALSE)
        return FALSE;

    /* Enable or disable capture buttons as appropriate */
    cam_data->mode = CAM_MODE_VIEW;
    cam_data->pipeline_type = VIEW_PIPELINE;
    set_capture_btns(m_ui, TRUE, FALSE);

    if (init == TRUE)
    {
	/* Add a bus watch for messages */
	source_id = gst_bus_add_watch (bus, (GstBusFunc) bus_message_watch, m_ui);
	gst_object_unref (bus);
    }

    /* Inforamtion status line */
    sprintf(s, "Camera %s (%s) playing", cam_data->current_cam, cam_data->current_dev);
    gtk_label_set_text (GTK_LABEL (m_ui->status_info), s);

    return TRUE;
}


/* Gst camera view and capture video */

int gst_capture(CamData *cam_data, MainUi *m_ui, int duration, int no_frames)
{
    /* Initial */
    if (gst_capture_init(cam_data, m_ui, duration, no_frames) == FALSE)
    	return FALSE;

    view_prepare_capt(cam_data, m_ui);

    /* GST setup */
    if (gst_capture_elements(cam_data, m_ui) == FALSE)
    	return FALSE;

    /* Capture limits */
    capture_limits(&(cam_data->gst_objs), m_ui);

    /* Capture pipeline element links */
    if (cam_data->pipeline_type == ENC_PIPELINE)
    {
	if (link_enc_pipeline(cam_data, m_ui) == FALSE)
	    return FALSE;
    }
    else
    {
	if (link_caps_pipeline(cam_data, m_ui) == FALSE)
	    return FALSE;
    }

    /* Start view and capture */
    if (start_capt_pipeline(cam_data, m_ui) == FALSE)
	return FALSE;

    return TRUE;
}


/* Setup preferences and other video capture data */

int gst_capture_init(CamData *cam_data, MainUi *m_ui, int duration, int no_frames)
{
    video_capt_t *capt;
    char seq_no_s[10];

    /* Set up convenience pointer */
    memset(&(cam_data->u.v_capt), 0, sizeof(video_capt_t));
    capt = &(cam_data->u.v_capt);

    /* Preferences */
    load_prefs(capt);

    /* Capture limits */
    m_ui->duration = duration;	
    m_ui->no_of_frames = no_frames;

    /* Initial */
    m_ui->thread_init = FALSE;
    sprintf(seq_no_s, "%03d", capt_seq_no);

    /* Object title for file name */
    capt->obj_title = gtk_entry_get_text( GTK_ENTRY (m_ui->obj_title));

    /* Timestamp */
    dttm_stamp(capt->tm_stmp, sizeof(capt->tm_stmp));

    /* Capture location */
    if (check_dir(capt->locn) == FALSE)
    {
	log_msg("APP0006", capt->locn, "APP0006", m_ui->window);
    	return FALSE;
    }

    /* Filename */
    if ((capt->codec_data = get_codec(capt->codec)) == NULL)
    {
	log_msg("CAM0024", capt->codec, "CAM0024", m_ui->window);
    	return FALSE;
    }

    get_file_name(capt->fn, 
    		  seq_no_s, 
		  (char *) capt->obj_title,
		  capt->tm_stmp, capt->id, capt->tt, capt->ts);
    sprintf(capt->out_name, "%s/%s.%s", capt->locn, capt->fn, capt->codec_data->extn);

    if (*(capt->codec_data->encoder) == '\0')		
	cam_data->pipeline_type = CAPS_PIPELINE;		// Requires a 2nd caps filter
    else
	cam_data->pipeline_type = ENC_PIPELINE;

    /* Capture sequence is incremented each time a capture is performed */
    capt_seq_no++;

    return TRUE;
}


/* Create capture elements and add to pipeline */

int gst_capture_elements(CamData *cam_data, MainUi *m_ui)
{
    long width, height;
    int fps;
    char *p;
    video_capt_t *capt;

    /* Convenience pointer */
    capt = &(cam_data->u.v_capt);

    /* Fixed elements */
    if (! create_element(&(cam_data->gst_objs.tee), "tee", "split", NULL, m_ui))
    	return FALSE;

    if (! create_element(&(cam_data->gst_objs.video_queue), "queue", "v_queue", NULL, m_ui))
    	return FALSE;

    if (! create_element(&(cam_data->gst_objs.capt_queue), "queue", "c_queue", NULL, m_ui))
    	return FALSE;

    if (! create_element(&(cam_data->gst_objs.muxer), capt->codec_data->muxer, "muxer", NULL, m_ui))
    	return FALSE;

    if (! create_element(&(cam_data->gst_objs.file_sink), "filesink", "file_sink", NULL, m_ui))
    	return FALSE;
    
    if (! create_element(&(cam_data->gst_objs.c_convert), "videoconvert", "c_convert", NULL, m_ui))
    	return FALSE;
    
    /* Different elements will created or set depending on the output format */
    if (cam_data->pipeline_type == CAPS_PIPELINE)		// Requires a 2nd caps filter
    {
	cam_data->gst_objs.c_filter = gst_element_factory_make ("capsfilter", "c_filter");

    	if (!cam_data->gst_objs.c_filter)
    	{
	    log_msg("CAM0020", NULL, "CAM0020", m_ui->window);
	    return FALSE;
    	}
    }
    else						// Normal encoding
    {
	cam_data->gst_objs.encoder = gst_element_factory_make (capt->codec_data->encoder, "encoder");

    	if (!cam_data->gst_objs.encoder)
    	{
	    log_msg("CAM0020", NULL, "CAM0020", m_ui->window);
	    return FALSE;
    	}
    }

    /* Just a check, shouldn't be a problem but ... */
    if (!cam_data->pipeline ||
    	!cam_data->gst_objs.v4l2_src || !cam_data->gst_objs.v_filter || !cam_data->gst_objs.v_convert || 
    	!cam_data->gst_objs.v_sink || !cam_data->gst_objs.vid_rate)
    {
	log_msg("CAM0020", NULL, "CAM0020", m_ui->window);
        return FALSE;
    }

    /* The queue needs to be allowed to leak and the video sink should not sync */
    g_object_set (cam_data->gst_objs.video_queue, "leaky", TRUE, NULL);
    g_object_set (cam_data->gst_objs.v_sink, "sync", FALSE, NULL);

    /* Encoder properties */
    set_encoder_props(capt, &(cam_data->gst_objs.encoder), m_ui);

    /* Capture file */
    g_object_set (cam_data->gst_objs.file_sink, "location", capt->out_name, NULL);

    /* Build the pipeline - add all the elements (note some elements are already present from viewing) */
    gst_bin_add_many (GST_BIN (cam_data->pipeline),
		      cam_data->gst_objs.tee, cam_data->gst_objs.video_queue, cam_data->gst_objs.capt_queue,
    		      cam_data->gst_objs.muxer, cam_data->gst_objs.file_sink, cam_data->gst_objs.c_convert, NULL);

    if (cam_data->pipeline_type == ENC_PIPELINE)
    {
	gst_bin_add_many (GST_BIN (cam_data->pipeline), cam_data->gst_objs.encoder, NULL);
    }
    else
    {
	gst_bin_add_many (GST_BIN (cam_data->pipeline), cam_data->gst_objs.c_filter, NULL);

	/* Specify what kind of video is wanted from the camera */
	get_session(RESOLUTION, &p);
	res_to_long(p, &width, &height);
	get_session(FPS, &p);
	fps = atoi(p);

	/* Video (capture) caps filter */
	cam_data->gst_objs.c_caps = gst_caps_new_simple ("video/x-raw",
							   "format", G_TYPE_STRING, capt->codec_data->fourcc,
							   "framerate", GST_TYPE_FRACTION, fps, 1,
							   "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
							   "width", G_TYPE_INT, width,
							   "height", G_TYPE_INT, height,
							   NULL);
    }

    return TRUE;
}


/* Set the required properties for the encoder */

void set_encoder_props(video_capt_t *capt, GstElement **encoder, MainUi *m_ui) 
{
    int i, idx, pref_total, len, pr_type, i_val;
    double f_val;
    char key[PREF_KEY_SZ];
    char s[50];
    char *p, *nm, *nm_val;

    /* Total user default setting for the encoder */
    sprintf(key, "%sMAX", capt->codec_data->encoder);
    idx = get_user_pref(key, &p);

    if (p == NULL)
    	return;

    pref_total = atoi(p);
    
    /* Set the encoder object property for each setting */
    for(i = 0; i < pref_total; i++)
    {
    	/* Check preference setting */
	sprintf(key, "%s%02d", capt->codec_data->encoder, i);
    	get_user_pref(key, &p);

	if (p == NULL)
	{
	    sprintf(app_msg_extra, "The User Preferences file may be corrupt.");
	    sprintf(s, "Encoder Preference (%s)", key);
	    log_msg("CAM0006", s, "CAM0006", m_ui->window);
	    continue;
	}

    	if ((nm_val = strchr(p , '=')) == NULL)
	{
	    sprintf(app_msg_extra, "The User Preferences file may be corrupt.");
	    sprintf(s, "Encoder Property Value (%s)", key);
	    log_msg("CAM0006", s, "CAM0006", m_ui->window);
	    continue;
	}

	/* Get the property name and value */
    	nm_val++;
    	len = nm_val - p;
    	nm = (char *) malloc(len);
    	strncpy(nm, p, len - 1);
    	nm[len - 1] = '\0';

    	/* Convert value to correct type and set */
    	pr_type = codec_property_type(capt->codec_data->encoder, nm);

    	switch(pr_type)
    	{
	    case 0:						// Integer
		i_val = atoi(nm_val);
		g_object_set (G_OBJECT (*encoder), nm, i_val, NULL);
		break;

	    case 1:						// Float
		f_val = atof(nm_val);
		g_object_set (G_OBJECT (*encoder), nm, f_val, NULL);
		break;

	    case 2:						// Boolean
		if (*nm_val == 'T')
		    i_val = TRUE;
		else
		    i_val = FALSE;

		g_object_set (G_OBJECT (*encoder), nm, i_val, NULL);
		break;

	    case 3:						// Object
		if (*nm_val != '\0')
		    gst_util_set_object_arg (G_OBJECT (*encoder), nm, nm_val);

		break;

	    default:						// Unknown 
		log_msg("CAM0006", "Property data type", "CAM0006", m_ui->window);
    	};

    	free(nm);
    };

    return;
}


/* Link the capture elements for a pileine that uses an encoder */

int link_enc_pipeline(CamData *cam_data, MainUi *m_ui)
{
    GstPadTemplate *tee_src_pad_template;
    GstPad *queue_capt_pad, *queue_video_pad;
    app_gst_objects *gst_objs;

    /* Convenience pointer */
    gst_objs = &(cam_data->gst_objs);

    /* Video thread (note that some view elements are already linked) */
    if (gst_element_link_many (gst_objs->q1, gst_objs->tee, NULL) != TRUE)
    {
	sprintf(app_msg_extra, " - q1:tee");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
        return FALSE;
    }

    if (gst_element_link_many (gst_objs->video_queue, gst_objs->v_convert, NULL) != TRUE)
    {
	sprintf(app_msg_extra, " - video queue:video convert");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
	return FALSE;
    }

    /* Capture thread - (use an Encoder instead of second caps filter) */
    if (gst_element_link_many (gst_objs->capt_queue, gst_objs->c_convert, gst_objs->encoder, 
			       gst_objs->muxer, gst_objs->file_sink, NULL) != TRUE)
    {
	sprintf(app_msg_extra, " - capture queue:video convert:encoder:muxer:filesink");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
	return FALSE;
    }

    /* Build the pipeline - linking the elements with Request pads */
    tee_src_pad_template = gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (gst_objs->tee), "src_%u");

    gst_objs->tee_video_pad = gst_element_request_pad (gst_objs->tee, tee_src_pad_template, NULL, NULL);
    queue_video_pad = gst_element_get_static_pad (gst_objs->video_queue, "sink");

    gst_objs->tee_capt_pad = gst_element_request_pad (gst_objs->tee, tee_src_pad_template, NULL, NULL);
    queue_capt_pad = gst_element_get_static_pad (gst_objs->capt_queue, "sink");

    if (gst_pad_link (gst_objs->tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK)
    {
	sprintf(app_msg_extra, " - Cannot link Tee to Video pad");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
	gst_object_unref (cam_data->pipeline);
        return FALSE;
    }

    if (gst_pad_link (gst_objs->tee_capt_pad, queue_capt_pad) != GST_PAD_LINK_OK)
    {
	sprintf(app_msg_extra, " - Cannot link Tee to Capture pad");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
	gst_object_unref (cam_data->pipeline);
        return FALSE;
    }

    /* Free resources */
    gst_object_unref (queue_video_pad);
    gst_object_unref (queue_capt_pad);

    return TRUE;
}


// Link the capture elements for a pileine that requires a second caps filter */
// (Certain formats require a second caps filter for capture)

int link_caps_pipeline(CamData *cam_data, MainUi *m_ui)
{
    GstPadTemplate *tee_src_pad_template;
    GstPad *queue_capt_pad, *queue_video_pad;
    app_gst_objects *gst_objs;
    long width, height;
    int fps;
    char *p;

    /* Convenience pointer */
    gst_objs = &(cam_data->gst_objs);

    /* Specify what kind of video is wanted from the camera */
    get_session(RESOLUTION, &p);
    res_to_long(p, &width, &height);
    get_session(FPS, &p);
    fps = atoi(p);

    /* Video thread (note that some view elements are already linked) */
    if (gst_element_link_many (gst_objs->q1, gst_objs->tee, NULL) != TRUE)
    {
	sprintf(app_msg_extra, " - q1:tee");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
        return FALSE;
    }

    if (gst_element_link_many (gst_objs->video_queue, gst_objs->v_convert, NULL) != TRUE)
    {
	sprintf(app_msg_extra, " - video queue:video convert");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
        return FALSE;
    }

    /* Capture queue to capture convert */
    if (gst_element_link_many (gst_objs->capt_queue, gst_objs->c_convert, NULL) != TRUE)
    {
	sprintf(app_msg_extra, " - capture queue:video convert");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
	return FALSE;
    }

    /* Build the pipeline - linking the elements with Always pads */
    if (gst_element_link_filtered (gst_objs->c_convert, gst_objs->c_filter, gst_objs->c_caps) != TRUE)
    {
	sprintf(app_msg_extra, " - video convert:filter (c_caps)");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
	return FALSE;
    }

    if (gst_element_link_many (gst_objs->c_filter, gst_objs->muxer, gst_objs->file_sink, NULL) != TRUE)
    {
	sprintf(app_msg_extra, " - capture filter:muxer:filesink");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
	return FALSE;
    }

    /* Build the pipeline - linking the elements with Request pads */
    tee_src_pad_template = gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (gst_objs->tee), "src_%u");

    gst_objs->tee_video_pad = gst_element_request_pad (gst_objs->tee, tee_src_pad_template, NULL, NULL);
    queue_video_pad = gst_element_get_static_pad (gst_objs->video_queue, "sink");

    gst_objs->tee_capt_pad = gst_element_request_pad (gst_objs->tee, tee_src_pad_template, NULL, NULL);
    queue_capt_pad = gst_element_get_static_pad (gst_objs->capt_queue, "sink");

    if (gst_pad_link (gst_objs->tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK)
    {
	sprintf(app_msg_extra, " - Cannot link Tee to Video pad");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
	gst_object_unref (cam_data->pipeline);
        return FALSE;
    }

    if (gst_pad_link (gst_objs->tee_capt_pad, queue_capt_pad) != GST_PAD_LINK_OK)
    {
	sprintf(app_msg_extra, " - Cannot link Tee to Capture pad");
	log_msg("CAM0021", NULL, "CAM0021", m_ui->window);
	gst_object_unref (cam_data->pipeline);
        return FALSE;
    }

    /* Free resources */
    gst_object_unref (queue_video_pad);
    gst_object_unref (queue_capt_pad);
    gst_caps_unref(gst_objs->c_caps);

    return TRUE;
}


/* Pipeline watch and start playing setup */

int start_capt_pipeline(CamData *cam_data, MainUi *m_ui)
{
    GstBus *bus;
    guint source_id;
    char s[100];

    /* Set up sync handler for setting the xid once the pipeline is started */
    bus = gst_pipeline_get_bus (GST_PIPELINE (cam_data->pipeline));
    //gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler, NULL, NULL);	xxxx IS THIS NEEDED ?

    if (cam_set_state(cam_data, GST_STATE_PLAYING, m_ui->window) == FALSE)
        return FALSE;

    /* Enable or disable capture buttons as appropriate */
    cam_data->mode = CAM_MODE_CAPT;
    set_capture_btns(m_ui, FALSE, TRUE);

    /* Add a bus watch for messages */
    //source_id = gst_bus_add_watch (bus, (GstBusFunc) bus_message_watch, m_ui);	xxxx IS THIS NEEDED ?

    /* Inforamtion status line */
    sprintf(s, "Camera %s (%s) is capturing", cam_data->current_cam, cam_data->current_dev);

    if (m_ui->duration > 0)
	sprintf(s, "%s: %d secs", s, m_ui->duration);
    else if (m_ui->no_of_frames > 0)
	sprintf(s, "%s: %d frames", s, m_ui->no_of_frames);
    else
	sprintf(s, "%s: unlimited", s);

    gtk_label_set_text (GTK_LABEL (m_ui->status_info), s);
    gst_object_unref (bus);

    return TRUE;
}


/* Set pileline state */

int cam_set_state(CamData *cam_data, GstState state, GtkWidget *window)
{
    GstStateChangeReturn ret, ret2;
    char s[10];
    GstState chg_state;

    if (! GST_IS_ELEMENT(cam_data->pipeline))
    	return -1;

    ret = gst_element_set_state (cam_data->pipeline, state);

    switch(ret)
    {
	case GST_STATE_CHANGE_SUCCESS:
	case GST_STATE_CHANGE_NO_PREROLL:
	    ret2 = gst_element_get_state (cam_data->pipeline, &chg_state, NULL, GST_CLOCK_TIME_NONE);

	    if (chg_state != state)
	    {
		sprintf(app_msg_extra, "Current camera is %s", cam_data->current_cam);
		log_msg("CAM0022", s, "CAM0022", window);
		return FALSE;
	    }

	    break;

	case GST_STATE_CHANGE_ASYNC:
	    break;

	case GST_STATE_CHANGE_FAILURE:
	    switch (state)
	    {
		case GST_STATE_NULL:
		    strcpy(s, "NULL");
		    break;

		case GST_STATE_READY:
		    strcpy(s, "READY");
		    break;

		case GST_STATE_PAUSED:
		    strcpy(s, "PAUSED");
		    break;

		case GST_STATE_PLAYING:
		    strcpy(s, "PLAYING");
		    break;

		default:
		    strcpy(s, "Unknown");
	    }

	    sprintf(app_msg_extra, "Current camera is %s", cam_data->current_cam);
	    log_msg("CAM0022", s, "CAM0022", window);
	    return FALSE;

	default:
	    sprintf(app_msg_extra, "Unknown return - Current camera is %s", cam_data->current_cam);
	    log_msg("CAM0022", s, "CAM0022", window);
	    return FALSE;
    }

    cam_data->state = state;

    return TRUE;
}


/* Gstreamer doesn't recognise some format alternate values and some genuine ones */

void swap_fourcc(char *fcc_in, char *fcc_out)
{
    const char *match_fcc[][2] = {
				     { "YUYV", "YUY2" },
				     { "YU12", "I420" },
				     { "JPEG", "I420" },	// Just a guess
				     { "RGB3", "RGB" },		// RGB3 should work
				     { "BGR3", "BGR" }		// BGR3 should work
				 };
    const int fcc_max = 5;
    int i;

    /* Try to match */
    for(i = 0; i < fcc_max; i++)
    {
    	if (strcmp(fcc_in, match_fcc[i][0]) == 0)
    	{
	    strcpy(fcc_out, match_fcc[i][1]);
	    return;
	}
    }

    /* No match */
    strcpy(fcc_out, fcc_in);

    return;
}


/* Load user preferences for video capture and filenames */

static void load_prefs(video_capt_t *capt)
{
    char *p;

    get_session(CLRFMT, &p);
    swap_fourcc(p, capt->cam_fcc);

    get_user_pref(CAPTURE_FORMAT, &p);
    capt->codec = p;

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


/* Enable or disable capture buttons as appropriate */

void set_capture_btns (MainUi *m_ui, int start_sens, int stop_sens)
{
    int ctrl_sens;

    /* Start items on menu and toolbar */
    gtk_widget_set_sensitive (m_ui->cap_ui, start_sens);
    gtk_widget_set_sensitive (GTK_WIDGET (m_ui->cap_start_tb), start_sens);

    /* Snapshot items on menu and toolbar */
    gtk_widget_set_sensitive (m_ui->snap_ui, start_sens); 
    gtk_widget_set_sensitive (GTK_WIDGET (m_ui->snap_tb), start_sens);

    /* Stop items on menu and toolbar */
    gtk_widget_set_sensitive (m_ui->cap_stop, stop_sens);
    gtk_widget_set_sensitive (GTK_WIDGET (m_ui->cap_stop_tb), stop_sens);

    /* Pause is a special case */
    gtk_widget_set_sensitive (m_ui->cap_pause, stop_sens);
    gtk_widget_set_sensitive (GTK_WIDGET (m_ui->cap_pause_tb), stop_sens);

    /* Controls (always same as start) */
    gtk_widget_set_sensitive (m_ui->cbox_clrfmt, start_sens); 
    gtk_widget_set_sensitive (m_ui->cbox_res, start_sens); 
    gtk_widget_set_sensitive (m_ui->cbox_fps, start_sens); 
    gtk_widget_set_sensitive (m_ui->oth_ctrls_btn, start_sens); 
    gtk_widget_set_sensitive (m_ui->def_val_btn, start_sens); 

    /* Menu items, Toolbar items (always same as start) */
    gtk_widget_set_sensitive (m_ui->cam_hdr, start_sens); 
    gtk_widget_set_sensitive (m_ui->cbox_profile, start_sens); 

    return;
}


/* Set any capture limits */

void capture_limits(app_gst_objects *gst_objs, MainUi *m_ui)
{
    if (m_ui->no_of_frames > 0)
    {
	g_object_set (gst_objs->vid_rate, "drop-only", TRUE, NULL);
	g_object_set (gst_objs->v4l2_src, "num-buffers", m_ui->no_of_frames, NULL);
    }

    return;
}


// Prepare the pipeline for capture
// Unlink as appropriate and reset the caps filter if mpeg2 is to be captured

void view_prepare_capt(CamData *cam_data, MainUi *m_ui)
{
    char s[10];
    long width, height;
    int fps;
    char *p;
    char fourcc[5];


    if (cam_data->mode != CAM_MODE_VIEW)
    {
    	sprintf(s, "Expected View - found %d", cam_data->mode);
	log_msg("CAM0026", s, "CAM0020", m_ui->window);
    	return;
    }

    if (cam_set_state(cam_data, GST_STATE_NULL, m_ui->window) == FALSE)
    	return;

    gst_element_unlink (cam_data->gst_objs.q1, cam_data->gst_objs.v_convert);
    
    if (cam_data->pipeline_type == ENC_PIPELINE)
    {
	if (strcmp(cam_data->u.v_capt.codec, MPEG2) == 0)
	{
	    get_session(CLRFMT, &p);
	    swap_fourcc(p, fourcc);
	    get_session(RESOLUTION, &p);
	    res_to_long(p, &width, &height);
	    get_user_pref(MPG2_FRAMERATE, &p);
	    fps = atoi(p);

	    gst_caps_set_simple (cam_data->gst_objs.v_caps,
				 "format", G_TYPE_STRING, fourcc,
				 "framerate", GST_TYPE_FRACTION, fps, 1,
				 "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
				 "width", G_TYPE_INT, width,
				 "height", G_TYPE_INT, height,
				 NULL);

	    g_object_set (cam_data->gst_objs.v_filter, "caps", cam_data->gst_objs.v_caps, NULL);
	}
    }

    return;
}


/* Unlink and remove capture elements from the pipeline */

void capt_prepare_view(CamData *cam_data, MainUi *m_ui)
{
    app_gst_objects *gst_objs;
    char s[10];
    long width, height;
    int fps;
    char *p;
    char fourcc[5];
    GstStructure *gst_struct;
    GstCaps *tmp_caps;
    gint num, denom;

    /* Return pipeline to initial state */
    if (cam_data->mode != CAM_MODE_CAPT)
    {
    	sprintf(s, "Expected Capture - found %d", cam_data->mode);
	log_msg("CAM0026", s, "CAM0020", m_ui->window);
    	return;
    }

    if (cam_set_state(cam_data, GST_STATE_NULL, m_ui->window) == FALSE)
    	return;

    /* Convenience pointer */
    gst_objs = &(cam_data->gst_objs);

    /* Release the request pads from the Tee, and unref them */
    gst_element_release_request_pad (gst_objs->tee, gst_objs->tee_capt_pad);
    gst_element_release_request_pad (gst_objs->tee, gst_objs->tee_video_pad);
    gst_object_unref (gst_objs->tee_capt_pad);
    gst_object_unref (gst_objs->tee_video_pad);

    /* Remove or unlink capture elements */
    gst_bin_remove_many (GST_BIN (cam_data->pipeline), gst_objs->file_sink,
    						       gst_objs->muxer,
    						       gst_objs->tee,
    						       gst_objs->video_queue,
    						       gst_objs->capt_queue,
    						       gst_objs->c_convert,
    						       NULL);

    if (cam_data->pipeline_type == ENC_PIPELINE)
    {
	gst_bin_remove (GST_BIN (cam_data->pipeline), gst_objs->encoder);

	/* Check need to adjust fps (from mpeg2 capture) */
	g_object_get (gst_objs->v_filter, "caps", &tmp_caps, NULL);
	gst_struct = gst_caps_get_structure (tmp_caps, 0);

        if (!gst_structure_get_fraction (gst_struct, "framerate", &num, &denom)) 
        {
	    log_msg("CAM0027", NULL, "CAM0027", m_ui->window);
	    return;
        }
        else
        {
	    //printf("%s framerate found ! num %d denom %d\n", debug_hdr, num, denom);
        }

	gst_caps_unref(tmp_caps);
	get_session(FPS, &p);
	fps = atoi(p);

	if (num != fps)
	{
	    get_session(CLRFMT, &p);
	    swap_fourcc(p, fourcc);
	    get_session(RESOLUTION, &p);
	    res_to_long(p, &width, &height);

	    gst_caps_make_writable (gst_objs->v_caps);
	    gst_caps_set_simple (gst_objs->v_caps,
				 "format", G_TYPE_STRING, fourcc,
				 "framerate", GST_TYPE_FRACTION, fps, 1,
				 "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
				 "width", G_TYPE_INT, width,
				 "height", G_TYPE_INT, height,
				 NULL);

	    g_object_set (gst_objs->v_filter, "caps", gst_objs->v_caps, NULL);
	}
    }
    else
    {
	gst_bin_remove_many (GST_BIN (cam_data->pipeline), gst_objs->c_filter, NULL);
    }

    gst_element_link (gst_objs->q1, gst_objs->v_convert);

    /* Reset */
    gst_objs->file_sink = NULL;
    gst_objs->muxer = NULL;
    gst_objs->tee = NULL;
    gst_objs->video_queue = NULL;
    gst_objs->capt_queue = NULL;
    gst_objs->c_convert = NULL;
    gst_objs->c_filter = NULL;
    gst_objs->encoder = NULL;

    return;
}


/* Clear all elements from the pipeline */

int view_clear_pipeline(CamData *cam_data, MainUi *m_ui)
{
    char s[10];

    if (cam_data->mode != CAM_MODE_VIEW)
    {
    	sprintf(s, "Expected View - found %d", cam_data->mode);
	log_msg("CAM0026", s, "CAM0020", m_ui->window);
    	return FALSE;
    }

    if (cam_set_state(cam_data, GST_STATE_NULL, m_ui->window) == FALSE)
    	return FALSE;

    check_unref(&(cam_data->pipeline), "cam_video", TRUE);

    return TRUE;
}


/* Check the ref count of the element and set up if required */

int create_element(GstElement **element, char *factory_nm, char *nm, CamData *cam_data, MainUi *m_ui)
{
    int rc;

    if (GST_IS_ELEMENT(*element) == TRUE)
    {
	if ((rc = GST_OBJECT_REFCOUNT_VALUE (*element)) > 0)
	{
	    //printf("%s Element %s (%s) has a ref count of %d\n", debug_hdr, nm, factory_nm, rc);   // debug
	    return TRUE;
	}
    }

    *element = gst_element_factory_make ((const gchar *) factory_nm, (const gchar *) nm);

    if (! *(element))
    {
	log_msg("CAM0020", NULL, "CAM0020", m_ui->window);
        return FALSE;
    }

    return TRUE;
}


/* Check the ref count of the element and unref if specified */

void check_unref(GstElement **element, char *desc, int unref_indi)
{
    int rc;

    if (GST_IS_ELEMENT(*element) == TRUE)
    {
	rc = GST_OBJECT_REFCOUNT_VALUE (*element);
	//printf("%s:check_unref Element %s has a ref count of %d\n", debug_hdr, desc, rc);   // debug
    }
    else
    {
    	rc = 0;
    }

    if (unref_indi == TRUE)
    {
    	while (rc > 0)
    	{
	    gst_object_unref (*element);
	    rc--;
    	}

    	*element = NULL;
    }

    return;
}


/* Bus watch for the video window handle */

GstBusSyncReply bus_sync_handler (GstBus * bus, GstMessage * message, gpointer user_data)
{
    // Ignore anything but 'prepare-window-handle' element messages
    if (!gst_is_video_overlay_prepare_window_handle_message (message))
        return GST_BUS_PASS;

    if (video_window_handle != 0)
    {
        //g_print("%s sync reply\n", debug_hdr);
        GstVideoOverlay *overlay;

        // GST_MESSAGE_SRC (message) will be the video sink element
        overlay = GST_VIDEO_OVERLAY (GST_MESSAGE_SRC (message));
        gst_video_overlay_set_window_handle (overlay, video_window_handle);
    }
    else
    {
        g_warning ("Should have obtained video_window_handle by now!");
    }

    gst_message_unref (message);

    return GST_BUS_DROP;
}


/* Bus message watch */

gboolean bus_message_watch (GstBus *bus, GstMessage *msg, gpointer user_data)
{
    CamData *cam_data;
    MainUi *m_ui;
    GError *err;
    gchar *msg_str;

    /* Get data */
    m_ui = (MainUi *) user_data;
    cam_data = g_object_get_data (G_OBJECT(m_ui->window), "cam_data");

    /* Mainly interested in EOS, but need to be playing first */
    switch GST_MESSAGE_TYPE (msg)
    {
	case GST_MESSAGE_ERROR:
	    gst_message_parse_error (msg, &err, &msg_str);
	    sprintf(app_msg_extra, "Error received from element %s: %s\n", 
	    			   GST_OBJECT_NAME (msg->src), msg_str);

	    if (cam_data->mode != CAM_MODE_CAPT)
		log_msg("CAM0023", "View", "CAM0023", m_ui->window);
	    else
		log_msg("CAM0023", "Capture", "CAM0023", m_ui->window);

	    g_free (err);
	    g_free (msg_str);
	    break;

	case GST_MESSAGE_WARNING:
	    gst_message_parse_warning (msg, &err, &msg_str);
	    sprintf(app_msg_extra, "Warning received from element %s: %s\n", 
	    			   GST_OBJECT_NAME (msg->src), msg_str);

	    if (cam_data->mode != CAM_MODE_CAPT)
		log_msg("CAM0023", "View", "CAM0023", m_ui->window);
	    else
		log_msg("CAM0023", "Capture", "CAM0023", m_ui->window);

	    g_free (err);
	    g_free (msg_str);
	    break;

	case GST_MESSAGE_STATE_CHANGED:
	case GST_MESSAGE_ASYNC_DONE:
	    /* Action for capture only */
	    if (cam_data->mode != CAM_MODE_CAPT)
	    	break;

	    /* Only concerned with pipeline messages at present */
	    if (GST_MESSAGE_SRC (msg) != GST_OBJECT (cam_data->pipeline))
	    	break;

	    GstState curr_state, pend_state;
	    GstStateChangeReturn ret;
	    ret = gst_element_get_state (cam_data->pipeline, &curr_state, &pend_state, GST_CLOCK_TIME_NONE);

	    /* If not already started, start thread to monitor or time the capture */
	    if (m_ui->thread_init == FALSE)
	    {
		if (curr_state)
		{
		    if (ret == GST_STATE_CHANGE_SUCCESS && curr_state == GST_STATE_PLAYING)
		    {
		    	if (m_ui->duration > 0) 		// Timed capture
		    	{
			    if (init_thread(m_ui, &monitor_duration) == FALSE)
			    	break;
            		}
		    	else if (m_ui->no_of_frames > 0) 	// Number of buffers capture
		    	{
			    if (init_thread(m_ui, &monitor_frames) == FALSE)
			    	break;
            		}
            		else					// Unlimited
		    	{
			    if (init_thread(m_ui, &monitor_unltd) == FALSE)
			    	break;
            		}
		    }
		}
	    }

	    break;

	    /* Debug
	    if ((GST_MESSAGE_TYPE (msg)) == GST_MESSAGE_STATE_CHANGED)
		printf("%s capt 9a state change\n", debug_hdr);
	    else
		printf("%s capt 9a async done\n", debug_hdr);

	    if (! curr_state)
		printf("%s capt 9a curr state null\n", debug_hdr);
	    else
	    {
	    	if (curr_state == GST_STATE_PLAYING)
		    printf("%s capt 9a curr state playing\n", debug_hdr);
	    }

	    if (ret == GST_STATE_CHANGE_SUCCESS)
		printf("%s capt 9a state change success\n", debug_hdr);

	    else if (ret == GST_STATE_CHANGE_ASYNC)
		printf("%s capt 9a state change success\n", debug_hdr);
	    else
		printf("%s capt 9a state change failed\n", debug_hdr);

	    fflush(stdout);
	    break;
	    */

	case GST_MESSAGE_EOS:
	    /* Action for capture only */
	    if (cam_data->mode != CAM_MODE_CAPT)
	    	break;

	    /* Lock this section of code */
	    pthread_mutex_lock(&capt_lock_mutex);

	    /* Check the meta data file */
	    setup_meta(cam_data);

	    /* Prepare to restart normal viewing */
	    capt_prepare_view(cam_data, m_ui);

	    /* Release the mutex and set capture as done */
	    m_ui->thread_init = FALSE;
	    pthread_cond_signal(&capt_eos_cv);
	    pthread_mutex_unlock(&capt_lock_mutex);

	    /* Start viewing */
	    start_view_pipeline(cam_data, m_ui, FALSE);
	    break;

	    /* Debug
	    printf("%s capt 9 EOS\n", debug_hdr);
	    fflush(stdout);
	    */

	default:
	    /*
	    printf("%s Unknown message name %s type %d\n", debug_hdr, 
	    						   GST_MESSAGE_SRC_NAME(msg), 
	    						   GST_MESSAGE_TYPE(msg));
	    fflush(stdout);
	    */
	    break;
    }

    return TRUE;
}



/* Thread functions */


/* Start the nominated capture thread */

int init_thread(MainUi *m_ui, void *(*start_routine)(void*))
{
    int p_err;

    /* Start thread */
    if ((p_err = pthread_create(&mon_tid, NULL, start_routine, (void *) m_ui)) != 0)
    {
	sprintf(app_msg_extra, "Error: %s", strerror(p_err));
	log_msg("SYS9016", NULL, "SYS9016", m_ui->window);
	return FALSE;
    }

    m_ui->thread_init = TRUE;

    return TRUE;
}


/* Monitor and time the specified for capture duration */

void * monitor_duration(void *arg)
{
    MainUi *m_ui;
    CamData *cam_data;
    const gchar *s;
    char *info_txt;
    char new_status[150];
    time_t start_time;
    double tmp_pause, total_pause;
    
    /* Base information text */
    ret_mon = TRUE;
    m_ui = (MainUi *) arg;
    cam_data = g_object_get_data (G_OBJECT (m_ui->window), "cam_data");
    s = gtk_label_get_text (GTK_LABEL (m_ui->status_info));
    info_txt = (char *) malloc(strlen(s) + 1);
    strcpy(info_txt, (char *) s);
    cam_data->u.v_capt.capt_opt = 1;			// Capture number of seconds
    cam_data->u.v_capt.capt_reqd = m_ui->duration;

    /* Monitor the current time against the start time */
    cam_data->u.v_capt.capt_actl = 0;
    start_time = time(NULL);
    tmp_pause = 0;
    total_pause = 0;
    
    while(cam_data->u.v_capt.capt_actl <= m_ui->duration)
    {
    	usleep(500000);

    	/* Test if capture has been ended manually */
	if (! G_IS_OBJECT(cam_data->gst_objs.tee_capt_pad))
	    break;

	/* If the pipeline has been paused, maintain how long for, otherwise continue */
	if (cam_data->state == GST_STATE_PAUSED)
	{
	    tmp_pause = difftime(time(NULL), start_time) - total_pause - (double) cam_data->u.v_capt.capt_actl;
	    sprintf(new_status, "Capture paused at %ld of %d seconds\n", cam_data->u.v_capt.capt_actl, m_ui->duration);
	}
	else
	{
	    if (tmp_pause != 0)
	    {
	    	total_pause += tmp_pause;
	    	tmp_pause = 0;
	    }

	    cam_data->u.v_capt.capt_actl = (long) (difftime(time(NULL), start_time) - total_pause);
	    sprintf(new_status, "%s    (%ld of %d)\n", info_txt, cam_data->u.v_capt.capt_actl, m_ui->duration);
	}

	gtk_label_set_text (GTK_LABEL (m_ui->status_info), new_status);
    };

    free(info_txt);

    /* Time is up - stop capture and resume normal playback */
    set_eos(m_ui);

    pthread_exit(&ret_mon);
}


/* Monitor and control the specified capture frame count */

void * monitor_frames(void *arg)
{
    MainUi *m_ui;
    CamData *cam_data;
    guint64 frames;
    const gchar *s;
    char *info_txt;
    char new_status[100];
    
    /* Base information text */
    ret_mon = TRUE;
    m_ui = (MainUi *) arg;
    cam_data = g_object_get_data (G_OBJECT (m_ui->window), "cam_data");
    s = gtk_label_get_text (GTK_LABEL (m_ui->status_info));
    info_txt = (char *) malloc(strlen(s) + 1);
    strcpy(info_txt, (char *) s);
    cam_data->u.v_capt.capt_opt = 2;			// Capture number of frames
    cam_data->u.v_capt.capt_reqd = m_ui->no_of_frames;

    /* Display a rolling frame count - EOS will be set when the set limit has been reached */
    while (frames <= m_ui->no_of_frames)
    {
    	usleep(500000);

    	/* Test if capture has been ended manually */
	if (! G_IS_OBJECT(cam_data->gst_objs.tee_capt_pad))
	    break;

    	/* Check if buffer limit has been reached */
	if (! G_IS_OBJECT(cam_data->gst_objs.vid_rate))
	    break;

	/* If the pipeline has been paused, suspend counter, otherwise continue */
	g_object_get(cam_data->gst_objs.vid_rate, "out", &frames, NULL);
	cam_data->u.v_capt.capt_actl = (long) frames;

	if (cam_data->state == GST_STATE_PAUSED)
	    sprintf(new_status, "Capture paused at %d of %d frames\n", (int) frames, m_ui->no_of_frames);
	else
	    sprintf(new_status, "%s    (%d of %d)\n", info_txt, (int) frames, m_ui->no_of_frames);

    	gtk_label_set_text (GTK_LABEL (m_ui->status_info), new_status);
    };

    free(info_txt);

    /* Time is up - stop capture and resume normal playback */
    cam_data = g_object_get_data (G_OBJECT(m_ui->window), "cam_data");

    pthread_exit(&ret_mon);
}


/* Monitor the time amount of capture duration */

void * monitor_unltd(void *arg)
{
    MainUi *m_ui;
    CamData *cam_data;
    const gchar *s;
    char *info_txt;
    char new_status[100];
    time_t start_time;
    double tmp_pause, total_pause;
    
    /* Base information text */
    ret_mon = TRUE;
    m_ui = (MainUi *) arg;
    cam_data = g_object_get_data (G_OBJECT (m_ui->window), "cam_data");
    s = gtk_label_get_text (GTK_LABEL (m_ui->status_info));
    info_txt = (char *) malloc(strlen(s) + 1);
    strcpy(info_txt, (char *) s);
    cam_data->u.v_capt.capt_opt = 3;			// Capture unlimited seconds 
    cam_data->u.v_capt.capt_reqd = -1;

    /* Monitor the current time against the start time */
    cam_data->u.v_capt.capt_actl = 0;
    start_time = time(NULL);
    tmp_pause = 0;
    total_pause = 0;
    
    while(1)
    {
    	usleep(500000);

    	/* Test if capture has ended manually */
	if (! G_IS_OBJECT(cam_data->gst_objs.tee_capt_pad))
	    break;

	/* If the pipeline has been paused, maintain how long for, otherwise continue */
	if (cam_data->state == GST_STATE_PAUSED)
	{
	    tmp_pause = difftime(time(NULL), start_time) - total_pause - (double) cam_data->u.v_capt.capt_actl;
	    sprintf(new_status, "Capture paused at %ld seconds\n", cam_data->u.v_capt.capt_actl);
	}
	else
	{
	    if (tmp_pause != 0)
	    {
	    	total_pause += tmp_pause;
	    	tmp_pause = 0;
	    }

	    cam_data->u.v_capt.capt_actl = (long) (difftime(time(NULL), start_time) - total_pause);
	    sprintf(new_status, "%s    %ld seconds\n", info_txt, cam_data->u.v_capt.capt_actl);
	}

    	gtk_label_set_text (GTK_LABEL (m_ui->status_info), new_status);
    };

    free(info_txt);
    pthread_exit(&ret_mon);
}


/* Set off an end-of-stream message */

int set_eos(MainUi *m_ui)
{
    int p_err;

    if ((p_err = pthread_create(&eos_tid, NULL, &send_EOS, (void *) m_ui)) != 0)
    {
	sprintf(app_msg_extra, "Error: %s", strerror(p_err));
	log_msg("SYS9016", NULL, "SYS9016", m_ui->window);
    }

    return p_err;
}


/* Send an EOS event allowing for exclusivity (mutex) */

void * send_EOS(void *arg)
{
    MainUi *m_ui;
    CamData *cam_data;

    /* Base information text */
    eos_tid = pthread_self();
    ret_eos = TRUE;
    m_ui = (MainUi *) arg;
    cam_data = g_object_get_data (G_OBJECT (m_ui->window), "cam_data");

    /* Ignore if capture has already stopped */
    if (! G_IS_OBJECT(cam_data->gst_objs.tee_capt_pad))
	pthread_exit(&ret_eos);

    /* Initiate capture stop and wait for completion */
    pthread_mutex_lock (&capt_lock_mutex);

    gst_element_send_event(cam_data->pipeline, gst_event_new_eos());

    /* If pipeline is paused, restart it */
    if (cam_data->state == GST_STATE_PAUSED)
    	cam_set_state(cam_data, GST_STATE_PLAYING, m_ui->window);

    /* Should be immediate, but wait for eos processing to complete */
    pthread_cond_wait(&capt_eos_cv, &capt_lock_mutex);
    pthread_mutex_unlock (&capt_lock_mutex);

    pthread_exit(&ret_eos);
}


/* Create the meta data file if necessary */

void setup_meta(CamData *cam_data)
{
    guint64 frames, dropped;

    if (TRUE != TRUE)
    	return;

    /* Get the frame total if available (should be) */
    frames = 0;

    if (G_IS_OBJECT(cam_data->gst_objs.vid_rate))
    {
	g_object_get(cam_data->gst_objs.vid_rate, "out", &frames, NULL);
	cam_data->u.v_capt.capt_frames = (long) frames;

	g_object_get(cam_data->gst_objs.vid_rate, "drop", &dropped, NULL);
	cam_data->u.v_capt.capt_dropped = (long) dropped;
    }
    else
    {
	cam_data->u.v_capt.capt_frames = 0;
	cam_data->u.v_capt.capt_dropped = 0;
    }

    write_meta_file('v', cam_data, NULL);

    return;
}


/* Destroy the mutex and condition (for completeness only here) */

void capture_cleanup()
{
    pthread_mutex_destroy(&capt_lock_mutex);
    pthread_cond_destroy(&capt_eos_cv);

    return;
}


/* Place or remove a reticule on the view area */

void set_reticule(MainUi *m_ui, CamData *cam_data)
{
    cam_data->gst_objs.probe_id = gst_pad_add_probe (cam_data->gst_objs.blockpad, 
						     GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, 
						     OnPadProbe, m_ui, NULL); 

    return;
}


/* Prepare the pipeline for a reticule */

int prepare_reticule(MainUi *m_ui, CamData *cam_data)
{
    GstState curr_state, pend_state;
    GstStateChangeReturn ret;
    CairoOverlayState *state;

    /* Unlink from v_sink */
    gst_element_unlink (cam_data->gst_objs.v_convert, cam_data->gst_objs.v_sink);

    /* Create reticule elements (cairo) */
    if (! create_element(&(cam_data->gst_objs.cairo_overlay), "cairooverlay", "reticule", cam_data, m_ui))
    	return FALSE;

    if (! create_element(&(cam_data->gst_objs.cairo_convert), "videoconvert", "r_convert", cam_data, m_ui))
    	return FALSE;

    /* Set up overlay state */
    cam_data->gst_objs.overlay_state = g_new0 (CairoOverlayState, 1);
    state = cam_data->gst_objs.overlay_state;

    /* Hook up the neccesary signals for cairooverlay */
    state->draw_handler = g_signal_connect (cam_data->gst_objs.cairo_overlay, 
					    "draw", G_CALLBACK (OnDrawReticule), m_ui);
    state->info_handler = g_signal_connect (cam_data->gst_objs.cairo_overlay, 
					    "caps-changed", G_CALLBACK (OnPrepReticule), m_ui);

    /* Add to pipeline */
    gst_bin_add_many (GST_BIN (cam_data->pipeline), 
    				cam_data->gst_objs.cairo_overlay, 
    				cam_data->gst_objs.cairo_convert, 
    				NULL);

    /* Link */
    if (gst_element_link_many (cam_data->gst_objs.v_convert,
			       cam_data->gst_objs.cairo_overlay, 
			       cam_data->gst_objs.cairo_convert,
			       cam_data->gst_objs.v_sink,
			       NULL) != TRUE)
    	return FALSE;

    /* Set state */
    if (gst_element_sync_state_with_parent(cam_data->gst_objs.cairo_overlay) == FALSE)
    {
	log_msg("CAM0028", "cairo_overlay", "CAM0028", m_ui->window);
    	return FALSE;
    }

    if (gst_element_sync_state_with_parent(cam_data->gst_objs.cairo_convert) == FALSE)
    {
	log_msg("CAM0028", "cairo_convert", "CAM0028", m_ui->window);
    	return FALSE;
    }

    ret = gst_element_get_state (cam_data->pipeline, &curr_state, &pend_state, GST_CLOCK_TIME_NONE);
    gst_element_set_state (cam_data->gst_objs.cairo_overlay, curr_state);
    gst_element_set_state (cam_data->gst_objs.cairo_convert, curr_state);

    return TRUE;
}


/* Remove reticule from the pipeline */

int remove_reticule(MainUi *m_ui, CamData *cam_data)
{
    CairoOverlayState *state;

    /* Set to NULL state */
    gst_element_set_state (cam_data->gst_objs.cairo_overlay, GST_STATE_NULL);
    gst_element_set_state (cam_data->gst_objs.cairo_convert, GST_STATE_NULL);

    /* Unlink cairo overlay objects */
    gst_element_unlink (cam_data->gst_objs.v_convert, cam_data->gst_objs.cairo_overlay);
    gst_element_unlink (cam_data->gst_objs.cairo_convert, cam_data->gst_objs.v_sink);

    /* Unhook the neccesary signals for cairooverlay */
    state = cam_data->gst_objs.overlay_state;
    g_signal_handler_disconnect (cam_data->gst_objs.cairo_overlay, state->draw_handler); 
    g_signal_handler_disconnect (cam_data->gst_objs.cairo_overlay, state->info_handler); 

    /* Remove reticule elements (cairo) */
    gst_bin_remove_many (GST_BIN (cam_data->pipeline), cam_data->gst_objs.cairo_overlay,
						       cam_data->gst_objs.cairo_convert,
						       NULL);

    /* Clear the overlay state */
    g_free (cam_data->gst_objs.overlay_state);

    /* Link */
    if (gst_element_link (cam_data->gst_objs.v_convert, cam_data->gst_objs.v_sink) != TRUE)
    	return FALSE;

    /* Reset */
    cam_data->gst_objs.cairo_overlay = NULL;
    cam_data->gst_objs.cairo_convert = NULL;

    return TRUE;
}


/* Debug the state of a GST Element */

void debug_state(GstElement *el, char *desc, CamData *cam_data)
{
    GstState curr, pend;
    GstStateChangeReturn ret;
    gboolean gb1, gb2;

    gb1 = gst_pad_is_blocked (cam_data->gst_objs.blockpad);
    gb2 = gst_pad_is_blocking (cam_data->gst_objs.blockpad);
    printf("%s Debug State: %s blocked %d blocking %d id %lu\n", 
    		debug_hdr, desc, gb1, gb2,cam_data->gst_objs.probe_id); fflush(stdout);

    if (! el)
    {
    	printf("%s Debug State: %s is null\n", debug_hdr, desc); fflush(stdout);
    	return;
    }

    ret = gst_element_get_state (el, &curr, &pend, GST_CLOCK_TIME_NONE);
    printf("%s Debug State: %s current %d pending %d\n", debug_hdr, desc, curr, pend); fflush(stdout);

    return;
}
