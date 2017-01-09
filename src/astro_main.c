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
** Application:	AstroCTC
**
** Author:	Anthony Buckley
**
** Description:
**  	Application control for AstroCTC. Allow connecting to a WebCam for capturing
**	video. May be used for anything really, but ostensibly for use in creating astronomical images.
**
** History
**	26-Dec-2013	Initial code
**
*/


/* Includes */

#include <stdlib.h>  
#include <string.h>  
#include <libgen.h>  
#include <gtk/gtk.h>  
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <main.h>
#include <cam.h>
#include <defs.h>
#include <preferences.h>


/* Defines */


/* Prototypes */

void initialise(CamData *, MainUi *);
void final();

extern void main_ui(CamData *, MainUi *);
extern int check_app_dir();
extern int reset_log();
extern void read_user_prefs(GtkWidget *);
extern void read_profiles(GtkWidget *);
extern void load_profile(char *);
extern int get_user_pref(char *, char **);
extern int save_session(char *);
extern void free_session();
extern void free_prefs();
extern void free_profiles();
extern void close_log();
extern void gst_view(CamData *, MainUi *);
extern void capture_cleanup();
extern void log_msg(char*, char*, char*, GtkWidget*);
//extern void debug_session();


/* Globals */

static const char *debug_hdr = "DEBUG-astroctc.c ";
guintptr video_window_handle = 0;


/* Main program control */

int main(int argc, char *argv[])
{  
    CamData cam_data;
    MainUi m_ui;

    /* Initial work */
    initialise(&cam_data, &m_ui);

    /* Initialise Gtk & GStreamer */
    gtk_init(&argc, &argv);  
    gst_init (&argc, &argv);

    main_ui(&cam_data, &m_ui);

    gst_view(&cam_data, &m_ui);

    gtk_main();  

    final();

    exit(0);
}  


/* Initial work */

void initialise(CamData *cam_data, MainUi *m_ui)
{
    char *p;

    /* Set variables */
    app_msg_extra[0] = '\0';
    memset(cam_data, 0, sizeof (CamData));
    memset(m_ui, 0, sizeof (MainUi));

    /* Set application directory */
    if (! check_app_dir())
    	exit(-1);

    /* Reset log file and log start */
    if (! reset_log())
    	exit(-1);

    log_msg("SYS9007", NULL, NULL, NULL);

    /* Load user preferences (a default set if required) */
    read_user_prefs(NULL);

    /* Load a list of preset profiles if any */
    read_profiles(NULL);

    /* Load data from the default preset profile if possible */
    get_user_pref(DEFAULT_PROFILE, &p);
    load_profile(p);

    return;
}


/* Final work */

void final()
{
    /* Capture cleanup */
    capture_cleanup();

    /* Save and free the session settings */
    save_session(NULL);
    free_session();

    /* Free user preferences */
    free_prefs();

    /* Free profile list */
    free_profiles();

    /* Close log file */
    log_msg("SYS9008", NULL, NULL, NULL);
    close_log();

    return;
}
