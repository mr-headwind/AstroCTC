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
**  Error and Message Reference functions
**  Logging functions
**  Session management
**  Window management
**  General usage functions
**
** Author:	Anthony Buckley
**
** History
**	08-Jan-2014	Initial code
**
*/


/* Defines */

#define ERR_FILE
#define MAX_SETTING 50


/* Includes */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <session.h>
#include <gtk/gtk.h>
#include <linux/videodev2.h>
#include <codec.h>
#include <cam.h>
#include <defs.h>


/* Prototypes */

void app_msg(char*, char *, GtkWidget*);
void log_msg(char*, char*, char*, GtkWidget*);
void info_dialog(GtkWidget *, char *, char *);
gint query_dialog(GtkWidget *, char *, char *);
int reset_log();
int read_saved_session(char *);
int save_session(char *);
int set_session(char*, char*);
void get_session(char*, char**);
void get_session_reset(char*, char**);
void free_session();
void match_session(char *, char *, int, int *);
void check_session_save(camera_t *);
void close_log();
int check_app_dir();
void get_msg(char*, char*, char*);
void string_trim(char*);
char *camfile_name();
void register_window(GtkWidget *);
void deregister_window(GtkWidget *);
void free_window_reg();
void close_open_ui();
int close_ui(char *);
int is_ui_reg(char *, int);
char * log_name();
char * app_dir_path();
char * home_dir();
void strlower(char *, char *);
void dttm_stamp(char *, size_t);
int check_dir(char *);
int make_dir(char *);
int val_str2numb(char *, int *, char *, GtkWidget *);
int check_errno(char *);
int64_t msec_time();
void print_bits(size_t const, void const * const);
GtkWidget * find_parent(GtkWidget *);
GtkWidget * find_widget_by_name(GtkWidget *, char *);
GtkWidget * find_widget_by_parent(GtkWidget *, char *);
GList * ctrl_widget_list(GtkWidget *, GtkWidget *);
void delete_menu_items(GtkWidget *, char *);
int write_meta_file(char, CamData *, char *);
void cur_date_str(char *, int, char *);
void video_meta(FILE *, CamData *);
void snap_meta(FILE *, CamData *);
void common_meta(FILE *, const gchar *, char *, char *);
void settings_meta(FILE *, CamData *);
void debug_session();

extern int find_ctl(camera_t *, char *);
extern void get_file_name(char *, char *, char *, char *, char, char, char);
extern struct v4l2_queryctrl * get_next_ctrl(int);
extern struct v4l2_list * get_next_oth_ctrl(struct v4l2_list *, CamData *);
extern void session_ctrl_val(struct v4l2_queryctrl *, char *, long *);


/* Globals */

static const char *app_messages[][2] = 
{ 
    { "CAM0001", "Error: Can't open sys classes directory (%s) "},
    { "CAM0002", "Error: Failed to read file details (stat). %s "},
    { "CAM0003", "Error: Failed to open video device: %s "},
    { "CAM0004", "Error: Failed to get video capabilities. %s "},
    { "CAM0005", "Error: Failed to get video controls. %s "},
    { "CAM0006", "Warning: No %s found. "},
    { "CAM0007", "This camera does not support capture. "},
    { "CAM0008", "Error: No %s found. "},
    { "CAM0010", "No cameras found. Please connect a camera and either select menu Camera->Relaod or restart %s "},
    { "CAM0011", "Failed to get value for %s control. "},
    { "CAM0012", "Failed to set value for %s control. "},
    { "CAM0013", "Failed to get current camera format. "},
    { "CAM0014", "Failed to set camera format. "},
    { "CAM0015", "Failed to get current streaming parameters. "},
    { "CAM0016", "Failed to set camera streaming parameters. "},
    { "CAM0017", "Snapshot error: %s. "},
    { "CAM0020", "Not all GST elements could be created. "},
    { "CAM0021", "GST Pipeline elements could not be linked. "},
    { "CAM0022", "Unable to set the pipeline to the %s state. "},
    { "CAM0023", "GST Pipeline message error found during %s. "},
    { "CAM0024", "GST capture error. Codec %s not found. "},
    { "CAM0025", "This action is not permitted during capture. "},
    { "CAM0026", "Camera mode is incorrect %s. "},
    { "CAM0027", "Error: Could not find 'framerate' in caps filter. "},
    { "CAM0028", "Could not sync %s with parent. "},
    { "CAM0029", "Could not retrieve pad to check negotiation status. "},
    { "CAM0030", "Caps negotiation problem. Caps set to %s. "},
    { "CAM0031", "Unknown or error 'fourcc' colour format found: %s. "},
    { "CAM0040", "The camera / driver does not support %s. "},
    { "APP0001", "Error: Filename may have only one Prefix, Mid or Suffix. "},
    { "APP0002", "Error: %s has an invalid value. "},
    { "APP0003", "Error: Please enter a value for %s. "},
    { "APP0004", "Error: %s is not unique. "},
    { "APP0005", "Debug: %s. "},
    { "APP0006", "Error: Capture location %s does not exist. Please create and retry. "},
    { "SYS9000", "Failed to start application. "},
    { "SYS9001", "Failed to read $HOME variable. "},
    { "SYS9002", "Failed to create Application directory: %s "},
    { "SYS9003", "Failed to create log file: %s "},
    { "SYS9004", "Ignoring last_session file error: %s "},
    { "SYS9005", "Failed to create file: %s "},
    { "SYS9006", "'video-dev' field size is not enough - change or log a bug. "},
    { "SYS9007", "Session started. "},
    { "SYS9008", "Session ends. "},
    { "SYS9009", "Failed to get or set a control. "},
    { "SYS9010", "File %s does not exist or cannot be read. "},
    { "SYS9011", "Failed to get parent container widget. %s "},
    { "SYS9012", "Failed to find widget. %s "},
    { "SYS9013", "Application message: "},
    { "SYS9014", "File error: %s "},
    { "SYS9015", "Warning: default user preferences being set up. "},
    { "SYS9016", "Error: Unable to create capture monitoring thread. "},
    { "SYS9017", "Mutex function error: %s. "},
    { "SYS9018", "%s does not exist. "},
    { "SYS9019", "Failed to open %s. "},
    { "SYS9020", "Failed to delete file: %s "},
    { "SYS9021", "Failed to rename file: %s "},
    { "SYS9040", "The Camera / Driver does support this function. "},
    { "UKN9999", "Error - Unknown error message given. "}			// NB - MUST be last
};

static const int Msg_Count = 57;
static char *Home;
static char *logfile = NULL;
static char *app_dir;
static char *preset_dir;
static char *sessionfile = NULL;
static char *camfile = NULL;
static FILE *lf = NULL;
static SessionData Settings[50];
static int setting_count, app_dir_len;
static const char *debug_hdr = "DEBUG-utility.c ";
static GList *open_ui_list_head = NULL;
static GList *open_ui_list = NULL;


/* Process additional application messages and error conditions */

void app_msg(char *msg_id, char *opt_str, GtkWidget *window)
{
    char msg[512];
    int i;

    /* Lookup the error */
    get_msg(msg, msg_id, opt_str);
    strcat(msg, " \n\%s");

    /* Display the error */
    info_dialog(window, msg, app_msg_extra);

    /* Reset global error details */
    app_msg_extra[0] = '\0';

    return;
}


/* Add a message to the log file and optionally display a popup */

void log_msg(char *msg_id, char *opt_str, char *sys_msg_id, GtkWidget *window)
{
    char msg[512];
    char date_str[50];

    /* Lookup the error */
    get_msg(msg, msg_id, opt_str);

    /* Log the message */
    cur_date_str(date_str, sizeof(date_str), "%d-%b-%Y %I:%M:%S %p");

    /* This may before anything has been set up (chicken & egg !). Use stderr if required */
    if (lf == NULL)
    	lf = stderr;

    fprintf(lf, "%s - %s\n", date_str, msg);

    if (strlen(app_msg_extra) > 0)
	fprintf(lf, "\t%s\n", app_msg_extra);

    fflush(lf);

    /* Reset global error details */
    app_msg_extra[0] = '\0';

    /* Optional display */
    if (sys_msg_id && window && logfile)
    {
    	sprintf(app_msg_extra, "\nLog file (%s) may contain more details.", logfile);
    	app_msg(sys_msg_id, opt_str, window);
    }

    return;
}


/* General prupose information dialog */

void info_dialog(GtkWidget *window, char *msg, char *opt)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (GTK_WINDOW (window),
				     GTK_DIALOG_MODAL,
				     GTK_MESSAGE_ERROR,
				     GTK_BUTTONS_CLOSE,
				     msg,
				     opt);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    return;
}


/* General prupose query dialog */

gint query_dialog(GtkWidget *window, char *msg, char *opt)
{
    GtkWidget *dialog;
    gint res;

    GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;

    dialog = gtk_message_dialog_new (GTK_WINDOW (window),
				     flags,
				     GTK_MESSAGE_QUESTION,
				     GTK_BUTTONS_YES_NO,
				     msg,
				     opt);

    res = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    return res;
}


/* Reset the log file */

int reset_log()
{
    logfile = (char *) malloc(strlen(Home) + (strlen(TITLE) * 2) + 10);
    sprintf(logfile, "%s/.%s/%s.log", Home, TITLE, TITLE);

    if ((lf = fopen(logfile, "w")) == (FILE *) NULL)
    {
	log_msg("SYS9003", logfile, NULL, NULL);
	free(logfile);
	return FALSE;
    }
    else
    {
    	g_print("%s: See Log file - %s for all details.\n", TITLE, logfile);
    }

    return TRUE;
}


/* Close the log file and free any memory */

void close_log()
{
    fclose(lf);
    free(logfile);
    free(sessionfile);
    free(camfile);
    free(app_dir);
    free(preset_dir);

    return;
}


/* Return the logfile name */

char * log_name()
{
    return logfile;
}


/* Return the application directory path */

char * app_dir_path()
{
    return app_dir;
}


/* Return the Home directory path */

char * home_dir()
{
    return Home;
}


/* Set up application directory(s) for the user if necessary */

int check_app_dir()
{
    struct stat fileStat;
    int err;

    if ((Home = getenv("HOME")) == NULL)
    {
    	log_msg("SYS9001", NULL, NULL, NULL);
    	return FALSE;
    }

    app_dir = (char *) malloc(strlen(Home) + strlen(TITLE) + 5);
    sprintf(app_dir, "%s/.%s", Home, TITLE);
    app_dir_len = strlen(app_dir);

    if ((err = stat(app_dir, &fileStat)) < 0)
    {
	if ((err = mkdir(app_dir, 0700)) != 0)
	{
	    log_msg("SYS9002", app_dir, NULL, NULL);
	    free(app_dir);
	    return FALSE;
	}
    }

    preset_dir = (char *) malloc(app_dir_len + strlen(PROFILES) + 2);
    sprintf(preset_dir, "%s/%s", app_dir, PROFILES);

    if ((err = stat(preset_dir, &fileStat)) < 0)
    {
	if ((err = mkdir(preset_dir, 0700)) != 0)
	{
	    log_msg("SYS9002", preset_dir, NULL, NULL);
	    free(preset_dir);
	    return FALSE;
	}
    }

    sessionfile = (char *) malloc(app_dir_len + strlen(PROFILES) + strlen(LAST_SESSION) + 3);
    sprintf(sessionfile, "%s/%s/%s", app_dir, PROFILES, LAST_SESSION);

    return TRUE;
}


/* Error lookup and optional string argument substitution */

void get_msg(char *s, char *msg_id, char *opt_str)
{
    int i;
    char *p, *p2;

    /* Find message */
    for(i = 0; i < Msg_Count; i++)
    {
    	if ((strcmp(msg_id, app_messages[i][0])) == 0)
	    break;
    }

    if (i >= Msg_Count)
    	i--;

    /* Check substitution. If none, show message as is with any '%s' blanked out. */
    p = (char *) app_messages[i][1];
    p2 = strstr(p, "%s");

    if ((! opt_str) || (strlen(opt_str) == 0) || (p2 == NULL))
    {
	sprintf(s, "(%s) %s", app_messages[i][0], app_messages[i][1]);

	if (p2 != NULL)
	{
	    p2 = strstr(s, "%s");
	    *p2++ = ' ';
	    *p2 = ' ';
	}

    	return;
    }

    /* Add substitution string */
    *s = '\0';
    sprintf(s, "(%s) ", app_messages[i][0]);

    for(s = (s + strlen(app_messages[i][0]) + 3); p < p2; p++)
    	*s++ = *p;

    *s = '\0';

    strcat(s, opt_str);
    strcat(s, p2 + 2);

    return;
}


/* Read the saved settings from the last session or a preset profile */

int read_saved_session(char *profile_fn)
{
    FILE *sf = NULL;
    char buf[150];
    char *p, *p2, *s;
    int c;

    if ((sf = fopen(profile_fn, "r")) == (FILE *) NULL)
    {
	return FALSE;
    }
    
    setting_count = 0;
    memset(Settings, 0, sizeof(Settings));

    while ((fgets(buf, 100, sf)) != NULL)
    {
	/* Check and save key */
	if ((p = strchr(buf, '|')) == NULL)
	{
	    sprintf(app_msg_extra, "%s", buf);
	    log_msg("SYS9004", "Invalid key format", NULL, NULL);
	    return FALSE;
	}

	if ((p - buf) > (sizeof(Settings[setting_count].key)) - 1)
	{
	    sprintf(app_msg_extra, "%s", buf);
	    log_msg("SYS9004", "Invalid key format", NULL, NULL);
	    return FALSE;
	}

	for(p2 = buf, s = Settings[setting_count].key; p2 < p; s++, p2++)
	{
	    if (! isspace(*p2))
		*s = *p2;
	}

	*s = '\0';

	/* Check and save value */
	p++;

	if ((p2 = strchr(p, '\n')) == NULL)
	{
	    sprintf(app_msg_extra, "%s", buf);
	    log_msg("SYS9004", "Invalid value", NULL, NULL);
	    return FALSE;
	}

	*p2 = '\0';
	Settings[setting_count].val = (char *) malloc(strlen(p) + 1);
	strcpy(Settings[setting_count].val, p);
	string_trim(Settings[setting_count].val);

	Settings[setting_count].reset_val = (char *) malloc(strlen(p) + 1);
	strcpy(Settings[setting_count].reset_val, Settings[setting_count].val);
	    
	if (setting_count > MAX_SETTING)
	{
	    log_msg("SYS9004", "Too many settings (read)", NULL, NULL);
	    return FALSE;
	}

	/* Set usage off */
	Settings[setting_count].save_flg = FALSE;

	setting_count++;
    }

    fclose(sf);

    return TRUE;
}


/* Save all the current settings */

int save_session(char *fn)
{
    FILE *sf = NULL;
    char buf[256];
    char *s_file;
    int i;

    if (fn == NULL)
    	s_file = sessionfile;
    else
    	s_file = fn;

    if ((sf = fopen(s_file, "w")) == (FILE *) NULL)
	return FALSE;

    for(i = 0; i < setting_count; i++)
    {
    	if (Settings[i].save_flg == FALSE)
	    continue;

    	sprintf(buf, "%s|%s\n", Settings[i].key, Settings[i].val);
    	
    	if ((fputs(buf, sf)) == EOF)
    	{
	    log_msg("SYS9005", s_file, NULL, NULL);
	    return FALSE;
	}
    }

    fclose(sf);

    return TRUE;
}


/* Update a session setting */

int set_session(char *key, char *val)
{
    int i, fnd;

    fnd = FALSE;

    for(i = 0; i < setting_count; i++)
    {
    	if (strcmp(Settings[i].key, key) == 0)
    	{
	    Settings[i].val = realloc(Settings[i].val, strlen(val) + 1);
	    strcpy(Settings[i].val, val);
	    Settings[i].save_flg = TRUE;
	    fnd = TRUE;
	    break;
    	}
    }

    if (! fnd)
    {
	if (setting_count < MAX_SETTING)
	{
	    strcpy(Settings[setting_count].key, key);
	    Settings[setting_count].val = (char *) malloc(strlen(val) + 1);
	    Settings[setting_count].reset_val = (char *) malloc(strlen(val) + 1);
	    strcpy(Settings[setting_count].val, val);
	    strcpy(Settings[setting_count].reset_val, val);
	    Settings[setting_count].save_flg = TRUE;
	    setting_count++;
	}
	else
	{
	    log_msg("SYS9004", "Too many settings (upd)", NULL, NULL);
	}
    }

    return TRUE;
}


/* Return a pointer to a session value for a key or NULL */

void get_session(char *key, char **val)
{
    int i;

    *val = NULL;

    for(i = 0; i < setting_count; i++)
    {
    	if (strcmp(Settings[i].key, key) == 0)
    	{
	    *val = Settings[i].val;
	    break;
    	}
    }

    return;
}


/* Return a pointer to a session reset value for a key or NULL */

void get_session_reset(char *key, char **val)
{
    int i;

    *val = NULL;

    for(i = 0; i < setting_count; i++)
    {
    	if (strcmp(Settings[i].key, key) == 0)
    	{
	    *val = Settings[i].reset_val;
	    break;
    	}
    }

    return;
}


/* Test if a session value matches a given value */

void match_session(char *sess_val, char *match_str, int idx, int *sess_idx)
{
    if (sess_val == NULL || *sess_idx >= 0)
    	return;

    if (strcmp(sess_val, match_str) == 0)
	*sess_idx = idx;

    return;
}


/* Check for unset control session items that need to be saved */

void check_session_save(camera_t *cam)
{
    int i;

    for(i = 0; i < setting_count; i++)
    {
    	if (Settings[i].save_flg == FALSE)
	    if (strncmp(Settings[i].key, "ctl-", 4) == 0)
	    	if (find_ctl(cam, Settings[i].key) == TRUE)
		    Settings[i].save_flg = TRUE;
    }

    return;
}


/* Free the session settings */

void free_session()
{
    int i;

    for(i = 0; i < setting_count; i++)
    {
    	free(Settings[i].val);
    	free(Settings[i].reset_val);
    }

    return;
}


/* Write the camera information and settings (meta data) for captures and snapshots to file */

int write_meta_file(char capt_type, CamData *cam_data, char *tm_stmp)
{
    FILE *mf = NULL;
    char buf[256];
    char fn[100];

    int i;

    /* Set file name and open */
    if (capt_type == 'v')
    {
	sprintf(buf, "%s/%s.metadata", cam_data->u.v_capt.locn, cam_data->u.v_capt.fn);
    }
    else if (capt_type == 's')
    {
	if (cam_data->u.s_capt.snap_max == 1)
	{
	    sprintf(buf, "%s/%s.metadata", cam_data->u.s_capt.locn, cam_data->u.s_capt.fn);
	}
	else
	{
	    get_file_name(fn, "xxx", (char *) cam_data->u.s_capt.obj_title, tm_stmp,
			  cam_data->u.s_capt.id, cam_data->u.s_capt.tt, cam_data->u.s_capt.ts);
	    sprintf(buf, "%s/%s.metadata", cam_data->u.s_capt.locn, fn);
	}
    }
    else
    {
    	return FALSE;
    }

    if ((mf = fopen(buf, "w")) == (FILE *) NULL)
	return FALSE;

    /* Video or snapshot specific details */
    if (capt_type == 'v')
    	video_meta(mf, cam_data);
    else
    	snap_meta(mf, cam_data);

    /* Camera settings */
    settings_meta(mf, cam_data);

    fclose(mf);

    return TRUE;
}


/* Write the meta data video related details */

void video_meta(FILE *mf, CamData *cam_data)
{
    char desc[100];
    char s[100];

    /* Common details */
    common_meta(mf, cam_data->u.v_capt.obj_title, cam_data->cam->vcaps.card, cam_data->u.v_capt.out_name);

    /* Codec format */
    sprintf(desc, "Codec: %s\n", cam_data->u.v_capt.codec_data->short_desc);
    fputs(desc, mf);

    /* Video capture mode - duration, frames, umlimited */
    switch (cam_data->u.v_capt.capt_opt)
    {
    	case 1: 
	    strcpy(desc, "seconds");
	    break;
    	case 2: 
	    strcpy(desc, "frames");
	    break;
    	case 3:
	    strcpy(desc, "unlimited - seconds");
	    break;
    	default:
	    strcpy(desc, "Unknown");
	    break;
    }

    sprintf(s, "Video requested: %ld (%s)\n", cam_data->u.v_capt.capt_reqd, desc);
    fputs(s, mf);

    /* Actual */
    if (cam_data->u.v_capt.capt_frames != 0)
    	sprintf(desc, "  (%ld frames, %ld dropped)", cam_data->u.v_capt.capt_frames, cam_data->u.v_capt.capt_dropped);
    else
    	desc[0] = '\0';

    switch (cam_data->u.v_capt.capt_opt)
    {
    	case 1: 
	    sprintf(s, "Output: %ld %s\n", cam_data->u.v_capt.capt_actl, desc);
	    break;
    	case 2: 
	    if (cam_data->u.v_capt.capt_frames == 0)
		sprintf(s, "Output: %ld (may vary, approx. only)\n", cam_data->u.v_capt.capt_actl);
	    else
		sprintf(s, "Output: %ld (%ld dropped)\n", cam_data->u.v_capt.capt_frames, 
							  cam_data->u.v_capt.capt_dropped);

	    break;
    	case 3:
	    sprintf(s, "Output: %ld %s\n", cam_data->u.v_capt.capt_actl, desc);
	    break;
    	default:
	    sprintf(s, "Output: %ld\n", cam_data->u.v_capt.capt_actl);
	    break;
    }

    fputs(s, mf);

    return;
}


/* Write the meta data snapshot related details */

void snap_meta(FILE *mf, CamData *cam_data)
{
    char s[100];

    /* Common details */
    common_meta(mf, cam_data->u.s_capt.obj_title, cam_data->cam->vcaps.card, cam_data->u.s_capt.out_name);

    /* Codec format */
    sprintf(s, "Codec: %s", cam_data->u.s_capt.codec);

    if (strcmp(cam_data->u.s_capt.codec, "jpg") == 0)
	sprintf(s, "%s (%u%%)\n", s, cam_data->u.s_capt.jpeg_quality);
    else
	sprintf(s, "%s\n", s);

    fputs(s, mf);

    /* Frames requested */
    sprintf(s, "Frames Requested: %ld\n", cam_data->u.s_capt.snap_max);
    fputs(s, mf);

    /* Options - Delay, Frame Group Delay every n frames */
    if (cam_data->u.s_capt.delay <= 0)
	sprintf(s, "Delay: Immediate\n");
    else
	sprintf(s, "Delay: %d seconds\n", cam_data->u.s_capt.delay);

    fputs(s, mf);

    if (cam_data->u.s_capt.delay_grp > 0)
    {
	sprintf(s, "       and a delay of %d seconds every %d frames\n", cam_data->u.s_capt.delay,
									 cam_data->u.s_capt.delay_grp);
	fputs(s, mf);
    }

    /* Frames delivered */
    sprintf(s, "Frames delivered: %ld\n", cam_data->u.s_capt.snap_count);
    fputs(s, mf);

    return;
}


/* Write the common meta data details - Date, Object Title, Camera, File */

void common_meta(FILE *mf, const gchar *obj_title, char *camera_nm, char *out_name)
{
    char date_str[50];

    cur_date_str(date_str, sizeof(date_str), "%d-%b-%Y %I:%M:%S %p");
    fputs(date_str, mf);
    fputs("\n\n", mf);

    fputs("Title: ", mf);
    fputs(obj_title, mf);
    fputs("\n", mf);

    fputs("Camera: ", mf);
    fputs(camera_nm, mf);
    fputs("\n", mf);

    fputs("File: ", mf);
    fputs(out_name, mf);
    fputs("\n", mf);

    return;
}


/* Write the camera settings to the meta data file */

void settings_meta(FILE *mf, CamData *cam_data)
{
    int init;
    long ctl_val;
    char ctl_key[10];
    char s[100];
    char *p;
    struct v4l2_queryctrl *qctrl;
    struct v4l2_list *tmp, *last;

    fputs("\nVIDEO FORMAT\n", mf);

    /* Video format */
    get_session(CLRFMT, &p);
    fputs("Video format: ", mf);
    fputs(p, mf);
    fputs("\n", mf);

    /* Resolution */
    get_session(RESOLUTION, &p);
    fputs("Resolution: ", mf);
    fputs(p, mf);
    fputs("\n", mf);

    /* Frame rate */
    get_session(FPS, &p);
    fputs("Frame rate: ", mf);
    fputs(p, mf);
    fputs("\n", mf);

    /* Controls */
    fputs("\nCONTROLS\n", mf);
    init = TRUE;
    last = NULL;

    /* Standard */
    while((qctrl = get_next_ctrl(init)) != NULL)
    {
	session_ctrl_val(qctrl, ctl_key, &ctl_val);
	sprintf(s, "%s: %ld\n", qctrl->name, ctl_val);
	fputs(s, mf);
	init = FALSE;
    }

    /* Other */
    while((tmp = get_next_oth_ctrl(last, cam_data)) != NULL)
    {
	qctrl = (struct v4l2_queryctrl *) tmp->v4l2_data;
	session_ctrl_val(qctrl, ctl_key, &ctl_val);
	sprintf(s, "%s: %ld\n", qctrl->name, ctl_val);
	fputs(s, mf);
	last = tmp;
    }

    /* Private */
    tmp = cam_data->cam->pctl_head;

    for(tmp = cam_data->cam->pctl_head; tmp != NULL; tmp = last)
    {
	qctrl = (struct v4l2_queryctrl *) tmp->v4l2_data;
	session_ctrl_val(qctrl, ctl_key, &ctl_val);
	sprintf(s, "%s: %ld\n", qctrl->name, ctl_val);
	fputs(s, mf);
	last = tmp->next;
    }

    return;
}


/* Remove leading and trailing spaces from a string */

void string_trim(char *s)
{
    int i;
    char *p;

    /* Trailing */
    for(i = strlen(s) - 1; i >= 0; i--)
    {
	if (isspace(s[i]))
	    s[i] = '\0';
	else
	    break;
    }

    /* Empty - all spaces */
    if (*s == '\0')
    	return;

    /* Leading */
    p = s;

    while(isspace(*p))
    {
    	p++;
    }

    while(*p != '\0')
    {
    	*s++ = *p++;
    }

    *s = '\0';

    return;
}


/* Set and retrieve the camera information file name */

char *camfile_name()
{
    /* First time set up */
    if (camfile == NULL)
    {
	camfile = (char *) malloc(app_dir_len + 10);
	sprintf(camfile, "%s/cam_info", app_dir);
    }

    return camfile;
}


/* Regiser the window as open */

void register_window(GtkWidget *window)
{
    open_ui_list = g_list_append (open_ui_list_head, window);

    if (open_ui_list_head == NULL)
    	open_ui_list_head = open_ui_list;

    return;
}


/* De-register the window as closed */

void deregister_window(GtkWidget *window)
{
    open_ui_list_head = g_list_remove (open_ui_list_head, window);

    return;
}


/* Check if a window title is registered (open) and present it to the user if reguired */

int is_ui_reg(char *s, int present)
{
    GtkWidget *window;
    const gchar *title;

    open_ui_list = g_list_first(open_ui_list_head);

    while(open_ui_list != NULL)
    {
	window = GTK_WIDGET (open_ui_list->data);
	title = gtk_window_get_title(GTK_WINDOW (window));

	if (strcmp(s, title) == 0)
	{
	    if (present == TRUE)
	    {
	    	gtk_window_present (GTK_WINDOW (window));
	    }

	    return TRUE;
	}

	open_ui_list = g_list_next(open_ui_list);
    }

    return FALSE;
}


/* Close any open windows */

void close_open_ui()
{
    GtkWidget *window;

    open_ui_list = g_list_first(open_ui_list_head);

    while(open_ui_list != NULL)
    {
	window = GTK_WIDGET (open_ui_list->data);
	gtk_window_close(GTK_WINDOW (window));
	open_ui_list = g_list_next(open_ui_list);
    }

    return;
}


/* Close a window */

int close_ui(char *s)
{
    GtkWidget *window;
    const gchar *title;

    open_ui_list = g_list_first(open_ui_list_head);

    while(open_ui_list != NULL)
    {
	window = GTK_WIDGET (open_ui_list->data);
	title = gtk_window_get_title(GTK_WINDOW (window));

	if (strcmp(s, title) == 0)
	{
	    gtk_window_close(GTK_WINDOW (window));
	    return TRUE;
	}

	open_ui_list = g_list_next(open_ui_list);
    }

    return FALSE;
}


/* Free the window register */

void free_window_reg()
{
    g_list_free (open_ui_list_head);

    return;
}


/* Convert a string to lowercase */

void strlower(char *s1, char *s2)
{
    for(; *s1 != '\0'; s1++, s2++)
    	*s2 = tolower(*s1);

    *s2 = *s1;

    return;
}


/* Return a date and time stamp */

void dttm_stamp(char *s, size_t max)
{
    size_t sz;
    struct tm *tm;
    time_t current_time;

    *s = '\0';
    current_time = time(NULL);
    tm = localtime(&current_time);
    sz = strftime(s, max, "%d%m%Y_%H%M%S", tm);

    return;
}


/* Check directory exists */

int check_dir(char *s)
{
    struct stat fileStat;
    int err;

    if ((err = stat(s, &fileStat)) < 0)
	return FALSE;

    if ((fileStat.st_mode & S_IFMT) == S_IFDIR)
	return TRUE;
    else
	return FALSE;
}


/* Create a directory */

int make_dir(char *s)
{
    int err;

    if ((err = mkdir(s, 0700)) != 0)
    {
	log_msg("SYS9002", s, NULL, NULL);
	return FALSE;
    }

    return TRUE;
}


/* Convert a string to a number and validate */

int val_str2numb(char *s, int *numb, char *subst, GtkWidget *window)
{
    int i;
    char *end;

    if (strlen(s) > 0)
    {
	errno = 0;
	i = strtol(s, &end, 10);

	if (errno != 0)
	{
	    app_msg("APP0002", subst, window);
	    return FALSE;
	}
	else if (*end)
	{
	    app_msg("APP0002", subst, window);
	    return FALSE;
	}
    }
    else
    {
    	i = 0;
    }

    *numb = i;

    return TRUE;
}


/* Check and print error message */

int check_errno(char *s)
{
    int err;

    if (errno != 0)
    {
	printf("%s %s - error: (%d) %s\n", debug_hdr, s, errno, strerror(errno));
	return errno;
    }

    return 0;
}


/* Return the current time in milliseconds */

int64_t msec_time()
{
    int64_t msecs;
    struct timespec t;

    clock_gettime(CLOCK_REALTIME_COARSE, &t);
    msecs = t.tv_sec * INT64_C(1000) + t.tv_nsec / 1000000;

    return msecs;
}


/* Show binary representation of value (useful debug) */

void print_bits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for(i = size - 1; i >= 0; i--)
    {
        for(j = 7; j >= 0; j--)
        {
            byte = b[i] & (1<<j);
            byte >>= j;
            printf("%u", byte);
        }
    }

    printf("\n");
    fflush(stdout);
}


/* Return the parent of a widget */

GtkWidget * find_parent(GtkWidget *init_widget)
{
    GtkWidget *parent_contnr;

    parent_contnr = gtk_widget_get_parent(init_widget);

    if (! GTK_IS_CONTAINER(parent_contnr))
    {
	log_msg("SYS9011", "From initial widget", NULL, NULL);
    	return NULL;
    }

    return parent_contnr;
}


/* Search for a child widget using the widget name */

GtkWidget * find_widget_by_name(GtkWidget *parent_contnr, char *nm)
{
    GtkWidget *widget;
    const gchar *widget_name;

    if (! GTK_IS_CONTAINER(parent_contnr))
    {
	log_msg("SYS9011", "By name", NULL, NULL);
    	return NULL;
    }

    GList *child_widgets = gtk_container_get_children(GTK_CONTAINER (parent_contnr));

    child_widgets = g_list_first(child_widgets);

    while (child_widgets != NULL)
    {
	widget = child_widgets->data;
	widget_name = gtk_widget_get_name (widget);

	if (widget_name != NULL)
	{
	    if (strcmp(widget_name, nm) == 0)
	    {
		g_list_free (child_widgets);
		return widget;
	    }
	}

	child_widgets = g_list_next(child_widgets);
    }

    g_list_free (child_widgets);

    return NULL;
}


/* Search for a widget using the parent of an initiating widget */

GtkWidget * find_widget_by_parent(GtkWidget *init_widget, char *nm)
{
    GtkWidget *widget;
    GtkWidget *parent_contnr;
    const gchar *widget_name;

    parent_contnr = gtk_widget_get_parent(init_widget);

    if (! GTK_IS_CONTAINER(parent_contnr))
    {
	log_msg("SYS9011", "By parent", NULL, NULL);
    	return NULL;
    }

    GList *child_widgets = gtk_container_get_children(GTK_CONTAINER (parent_contnr));

    child_widgets = g_list_first(child_widgets);

    while (child_widgets != NULL)
    {
	widget = child_widgets->data;
	widget_name = gtk_widget_get_name (widget);

	if (strcmp(widget_name, nm) == 0)
	{
	    g_list_free (child_widgets);
	    return widget;
	}

	child_widgets = g_list_next(child_widgets);
    }

    g_list_free (child_widgets);

    return NULL;
}


/* Find all the control widgets in a container */

GList * ctrl_widget_list(GtkWidget *contr, GtkWidget *window)
{
    GtkWidget *widget;
    GList *ctl_list = NULL;
    GList *tmp_list = NULL;

    if (! GTK_IS_CONTAINER(contr))
    {
	log_msg("SYS9011", "Get Next Control", "SYS9011", window);
	return NULL;
    }

    GList *child_widgets = gtk_container_get_children(GTK_CONTAINER (contr));
    child_widgets = g_list_first(child_widgets);

    while (child_widgets != NULL)
    {
	widget = child_widgets->data;

	if ((GTK_IS_RANGE (widget)) || (GTK_IS_COMBO_BOX_TEXT (widget)) || (GTK_IS_RADIO_BUTTON (widget)))
	{
	    ctl_list = g_list_prepend(ctl_list, widget);
	}
	else if (GTK_IS_CONTAINER (widget))
	{
	    tmp_list = ctrl_widget_list(widget, window);
	    ctl_list = g_list_concat(ctl_list, tmp_list);
	}

	child_widgets = g_list_next(child_widgets);
    }

    return ctl_list;
}


/* Delete items in a menu by name */

void delete_menu_items(GtkWidget *menu, char *nm)
{
    int len;
    GtkWidget *widget;
    const gchar *s;

    len = strlen(nm);

    GList *child_widgets = gtk_container_get_children(GTK_CONTAINER (menu));
    child_widgets = g_list_first(child_widgets);

    while (child_widgets != NULL)
    {
	widget = child_widgets->data;
	s = gtk_widget_get_name(widget);

	if (strncmp(s, nm, len) == 0)
	    gtk_widget_destroy(widget);

	child_widgets = g_list_next(child_widgets);
    }

    g_list_free(child_widgets);

    return;
}


/* Get a string for the current time */

void cur_date_str(char *date_str, int s_sz, char *fmt)
{
    struct tm *tm;
    time_t current_time;
    size_t sz;

    *date_str = '\0';
    current_time = time(NULL);
    tm = localtime(&current_time);
    sz = strftime(date_str, s_sz, fmt, tm);

    return;
}


/* Debug session details */

void debug_session()
{
    int i;

    printf("\n%s\n key	   value	reset	  used	(count: %d)\n", debug_hdr, setting_count);
    fflush(stdout);

    for(i = 0; i < setting_count; i++)
    {
    	printf("%s\t%s\t%s\t%d\n", Settings[i].key, Settings[i].val, Settings[i].reset_val, 
    				   Settings[i].save_flg);
	fflush(stdout);
    }

    return;
}
