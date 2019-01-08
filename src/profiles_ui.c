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
** Description: Preset Profiles user interface and management.
**
** Author:	Anthony Buckley
**
** History
**	15-Aug-2014	Initial code
**
*/


/* Includes */

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
#include <cam.h>
#include <defs.h>
#include <preferences.h>
#include <main.h>
#include <session.h>


/* Defines */


/* Types */
typedef struct _profile_ui
{
    GtkWidget *window;
    GtkWidget *main_window;
    GtkWidget *profile_cntr;
    GtkWidget *prf_box;
    GtkWidget *cmd_btn_grid;
    GtkWidget *nm_box;
    GtkWidget *cbox_profile;
    GtkWidget *def_btn, *save_btn, *save_as_btn, *del_btn, *rename_btn;
    GtkWidget *profile_nm;
    gchar *cur_profile;			// From main window
    gchar *cur_cbox_txt;
    int btn_action;
    int close_hndlr_id;
    int preset_hndlr_id;
} ProfileUi;


/* Prototypes */

int profile_main(GtkWidget *, gchar *);
ProfileUi * new_profile_ui();
void profile_ui(ProfileUi *);
void profile_control(ProfileUi *);
void profile_list(ProfileUi *);
void cmd_buttons(ProfileUi *);
void set_btn_sensitivity(ProfileUi *);
void profile_nm(ProfileUi *);
char * get_profile_name(int);
int read_profiles(GtkWidget *);
void free_profiles();
void reset_default_profile(char *, char *, ProfileUi *);
int validate_profile_nm(char *, ProfileUi *);
void add_profile_list(char *, ProfileUi *);
void del_profile_list(ProfileUi *);
char * profile_dir_path();
void save_profile(char *);
void load_profile(char *);
void delete_profile(char *, ProfileUi *);
void rename_profile(char *, char *, ProfileUi *);
void set_profile_nm(ProfileUi *);

static void OnSetProfile(GtkWidget *, gpointer);
void OnSetProfileName(GtkWidget *, gpointer);
void OnProfileDef(GtkWidget *, gpointer);
void OnProfileSave(GtkWidget *, gpointer);
void OnProfileSaveAs(GtkWidget *, gpointer);
void OnProfileDel(GtkWidget *, gpointer);
void OnProfileRename(GtkWidget *, gpointer);
void OnProfileClose(GtkWidget *, gpointer);


extern void log_msg(char *, char *, char *, GtkWidget *);
extern void app_msg(char *, char *, GtkWidget *);
extern void register_window(GtkWidget *);
extern void deregister_window(GtkWidget *);
extern char * app_dir_path();
extern char * home_dir();
extern void string_trim(char *);
extern int read_saved_session(char *);
extern int set_user_pref(char *, char *);
extern int write_user_prefs(GtkWidget *);
extern void load_profiles(GtkWidget *, char *, int, int);
extern int get_user_pref(char *, char **);
extern int save_session(char *);
extern int set_session(char *, char *);
extern int get_session(char *, char **);
extern gint query_dialog(GtkWidget *, char *, char *);


/* Globals */

static const char *debug_hdr = "DEBUG-profiles_ui.c ";
static GList *profile_names = NULL;
static GList *profile_names_head = NULL;
static int profile_count;


/* Display and maintenance of saved profiles */

int profile_main(GtkWidget *window, gchar *cur_profile)
{
    ProfileUi *ui;

    /* Initial */
    ui = new_profile_ui();
    ui->main_window = window;
    ui->cur_profile = cur_profile;

    /* Create the interface */
    profile_ui(ui);
    gtk_widget_show_all(ui->window);
    gtk_widget_hide(ui->nm_box);

    /* Register the window */
    register_window(ui->window);

    return TRUE;
}


/* Create new screen data variable */

ProfileUi * new_profile_ui()
{
    ProfileUi *ui = (ProfileUi *) malloc(sizeof(ProfileUi));
    memset(ui, 0, sizeof(ProfileUi));

    return ui;
}


/* Create the user interface and set the CallBacks */

void profile_ui(ProfileUi *p_ui)
{
    GtkWidget *close_btn;
    GtkWidget *main_vbox, *btn_box;

    /* Set up the UI window */
    p_ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(p_ui->window), PROFILE_UI);
    gtk_window_set_position(GTK_WINDOW(p_ui->window), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_default_size(GTK_WINDOW(p_ui->window), 200, 200);
    gtk_container_set_border_width(GTK_CONTAINER(p_ui->window), 10);
    gtk_window_set_transient_for (GTK_WINDOW (p_ui->window), GTK_WINDOW (p_ui->main_window));
    g_object_set_data (G_OBJECT (p_ui->window), "ui", p_ui);

    /* Main view */
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    /* Main update or view grid */
    profile_control(p_ui);

    /* Box container for action buttons */
    btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_widget_set_halign(GTK_WIDGET (btn_box), GTK_ALIGN_CENTER);

    /* Close button */
    close_btn = gtk_button_new_with_label("  Close  ");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(OnProfileClose), p_ui->window);
    gtk_box_pack_end (GTK_BOX (btn_box), close_btn, FALSE, FALSE, 0);
    gtk_widget_set_margin_top (btn_box, 10);

    /* Combine everything onto the window */
    gtk_box_pack_start (GTK_BOX (main_vbox), p_ui->profile_cntr, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), btn_box, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(p_ui->window), main_vbox);

    /* Exit when window closed */
    p_ui->close_hndlr_id = g_signal_connect(p_ui->window, "destroy", G_CALLBACK(OnProfileClose), NULL);

    /* Set the availability of each button */
    set_btn_sensitivity(p_ui);

    return;
}


/* Control container for user profile maintenance */

void profile_control(ProfileUi *p_ui)
{
    /* Saved profiles */
    profile_list(p_ui);

    /* Command buttons */
    cmd_buttons(p_ui);

    /* Name entry */
    profile_nm(p_ui);

    /* Container for main screen items */
    p_ui->profile_cntr = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_name(p_ui->profile_cntr, "profile_cntr");
    gtk_widget_set_halign(GTK_WIDGET (p_ui->profile_cntr), GTK_ALIGN_START);

    gtk_box_pack_start (GTK_BOX (p_ui->profile_cntr), p_ui->prf_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (p_ui->profile_cntr), p_ui->cmd_btn_grid, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (p_ui->profile_cntr), p_ui->nm_box, FALSE, FALSE, 0);

    return;
}


/* Create a list of all current profiles */

void profile_list(ProfileUi *p_ui)
{
    GtkWidget *label;
    PangoFontDescription *font_desc;
    int i, init;
    char s[20];
    char *nm;

    /* Set font */
    font_desc = pango_font_description_from_string ("Sans 9");

    /* Label */
    label = gtk_label_new("  Capture Presets  ");
    gtk_widget_override_font (label, font_desc);
    gtk_widget_override_color(label, GTK_STATE_FLAG_NORMAL, &DARK_BLUE);

    /* Saved profiles */
    p_ui->cbox_profile = gtk_combo_box_text_new();
    gtk_widget_override_font (p_ui->cbox_profile, font_desc);

    /* Load any preset profiles */
    load_profiles(p_ui->cbox_profile, p_ui->cur_profile, p_ui->preset_hndlr_id, FALSE);

    /* Setup */
    p_ui->cur_cbox_txt = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (p_ui->cbox_profile)); 
    p_ui->preset_hndlr_id = g_signal_connect(p_ui->cbox_profile, "changed", G_CALLBACK(OnSetProfile), p_ui);

    pango_font_description_free (font_desc);

    /* Put into a box container */
    p_ui->prf_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_halign(GTK_WIDGET (p_ui->prf_box), GTK_ALIGN_START);
    gtk_widget_set_margin_bottom (GTK_WIDGET (p_ui->prf_box), 10);
    gtk_box_pack_end (GTK_BOX (p_ui->prf_box), p_ui->cbox_profile, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (p_ui->prf_box), label, FALSE, FALSE, 0);
    gtk_widget_set_margin_top (p_ui->prf_box, 20);

    return;
}


/* Create the main command buttons */

void cmd_buttons(ProfileUi *p_ui)
{
    /* Place command buttons in a grid */
    p_ui->cmd_btn_grid = gtk_grid_new();

    /* Command buttons */
    p_ui->save_btn = gtk_button_new_with_label("  Save  ");
    g_signal_connect(p_ui->save_btn, "clicked", G_CALLBACK(OnProfileSave), (gpointer) p_ui);
    gtk_grid_attach(GTK_GRID (p_ui->cmd_btn_grid), p_ui->save_btn, 0, 0, 1, 1);

    p_ui->save_as_btn = gtk_button_new_with_label(" Save As ");
    g_signal_connect(p_ui->save_as_btn, "clicked", G_CALLBACK(OnProfileSaveAs), (gpointer) p_ui);
    gtk_grid_attach(GTK_GRID (p_ui->cmd_btn_grid), p_ui->save_as_btn, 1, 0, 1, 1);

    p_ui->del_btn = gtk_button_new_with_label("  Delete  ");
    g_signal_connect(p_ui->del_btn, "clicked", G_CALLBACK(OnProfileDel), (gpointer) p_ui);
    gtk_grid_attach(GTK_GRID (p_ui->cmd_btn_grid), p_ui->del_btn, 2, 0, 1, 1);

    p_ui->rename_btn = gtk_button_new_with_label("  Re-name  ");
    g_signal_connect(p_ui->rename_btn, "clicked", G_CALLBACK(OnProfileRename), (gpointer) p_ui);
    gtk_grid_attach(GTK_GRID (p_ui->cmd_btn_grid), p_ui->rename_btn, 0, 1, 1, 1);

    p_ui->def_btn = gtk_button_new_with_label(" Set Default ");
    g_signal_connect(p_ui->def_btn, "clicked", G_CALLBACK(OnProfileDef), (gpointer) p_ui);
    gtk_grid_attach(GTK_GRID (p_ui->cmd_btn_grid), p_ui->def_btn, 1, 1, 1, 1);

    return;
}


/* Create the entry for profile names */

void profile_nm(ProfileUi *p_ui)
{
    GtkWidget *label;
    PangoFontDescription *font_desc;

    /* Set font */
    font_desc = pango_font_description_from_string ("Sans 9");

    /* Label */
    label = gtk_label_new("Enter Preset name and press 'Enter'");
    gtk_widget_override_font (label, font_desc);
    gtk_widget_override_color(label, GTK_STATE_FLAG_NORMAL, &DARK_BLUE);

    /* Entry */
    p_ui->profile_nm = gtk_entry_new();
    gtk_widget_set_name(p_ui->profile_nm, "p_name");
    gtk_widget_override_font (p_ui->profile_nm, font_desc);
    gtk_entry_set_width_chars(GTK_ENTRY (p_ui->profile_nm), 30);
    gtk_widget_set_margin_start (p_ui->profile_nm, 10);
    gtk_widget_set_margin_end (p_ui->profile_nm, 10);

    g_signal_connect(p_ui->profile_nm, "activate", G_CALLBACK(OnSetProfileName), (gpointer) p_ui);

    pango_font_description_free (font_desc);

    /* Put into a box container */
    p_ui->nm_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_halign(GTK_WIDGET (p_ui->nm_box), GTK_ALIGN_START);
    gtk_box_pack_end (GTK_BOX (p_ui->nm_box), p_ui->profile_nm, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (p_ui->nm_box), label, FALSE, FALSE, 0);

    return;
}


/* Set the sensitivity of buttons depending on which is selected */

void set_btn_sensitivity(ProfileUi *p_ui)
{
    if ((strcmp(p_ui->cur_cbox_txt, PRF_NONE) == 0) || (strcmp(p_ui->cur_cbox_txt, LAST_SESSION) == 0))
    {
    	gtk_widget_set_sensitive(p_ui->save_btn, FALSE);
    	gtk_widget_set_sensitive(p_ui->del_btn, FALSE);
    	gtk_widget_set_sensitive(p_ui->rename_btn, FALSE);
    }
    else
    {
    	gtk_widget_set_sensitive(p_ui->save_btn, TRUE);
    	gtk_widget_set_sensitive(p_ui->del_btn, TRUE);
    	gtk_widget_set_sensitive(p_ui->rename_btn, TRUE);
    }

    return;
}


/* Return the next profile name */

char * get_profile_name(int init)
{
    char *nm = NULL;

    if (init == TRUE)
	profile_names = g_list_first(profile_names_head);
    else
	profile_names = g_list_next(profile_names);

    if (profile_names != NULL)
    	nm = (char *) profile_names->data;

    return nm;
}


/* Read any profile file names */

int read_profiles(GtkWidget *window)
{
    char *profile_dir;
    char *nm, *lnm;
    int err;
    DIR *dp = NULL;
    struct dirent *ep;

    /* Initial */
    profile_count = 0;
    profile_dir = profile_dir_path();
    lnm = NULL;

    /* Open profile directory */
    if((dp = opendir(profile_dir)) == NULL)
    {
	log_msg("SYS9019", "Profile directory", "SYS9019", window);
	free(profile_dir);
        return FALSE;
    }

    /* Read the file names */
    while (ep = readdir(dp))
    {
    	if ((strcmp(ep->d_name, ".") == 0) || (strcmp(ep->d_name, "..") == 0))
	    continue;

    	if (strcmp(ep->d_name, LAST_SESSION) == 0)
    	{
	    lnm = (char *) malloc(strlen(ep->d_name) + 1);
	    strcpy(lnm, ep->d_name);
    	}
    	else
    	{
	    nm = (char *) malloc(strlen(ep->d_name) + 1);
	    strcpy(nm, ep->d_name);
	    profile_names = g_list_prepend(profile_names, nm);
    	}

	profile_count++;
    }

    /* Correct order - if there is a last session (nearly always) it needs to go first */
    profile_names_head = g_list_reverse(profile_names);

    if (lnm != NULL)
	profile_names_head = g_list_prepend(profile_names_head, lnm);

    /* Close off */
    closedir(dp);
    free(profile_dir);
}


/* Return the profile directory path */

char * profile_dir_path()
{
    char *path;
    char *app_dir;
    int app_dir_len;

    app_dir = app_dir_path();
    app_dir_len = strlen(app_dir);
    path = (char *) malloc(app_dir_len + strlen(PROFILES) + 2);
    sprintf(path, "%s/%s", app_dir, PROFILES);

    return path;
}


/* Free the profile list */

void free_profiles()
{
    char *nm;

    profile_names = g_list_first(profile_names_head);

    while(profile_names != NULL)
    {
    	nm = (char *) profile_names->data;
    	free(nm);

	profile_names = g_list_next(profile_names);
    }

    g_list_free(profile_names_head);

    return;
}


/* Set the default text for a profile name */

void set_profile_nm(ProfileUi *p_ui)
{
    MainUi *m_ui;
    const gchar *obj;
    int len;
    char *s;

    if ((strcmp(p_ui->cur_cbox_txt, PRF_NONE) == 0) || (strcmp(p_ui->cur_cbox_txt, LAST_SESSION) == 0))
    {
    	/* Use the camera and object text as guide */
	m_ui = g_object_get_data (G_OBJECT(p_ui->main_window), "ui");
	obj = gtk_entry_get_text (GTK_ENTRY (m_ui->obj_title));

	len = strlen(obj);

	if (len > 0)
	{
	    s = (char *) malloc(len + 15);
	    sprintf(s, "camera_%s_nn", obj);
	}
	else
	{
	    s = (char *) malloc(20);
	    strcpy(s, "camera_object_nn");
	}

	gtk_entry_set_text (GTK_ENTRY (p_ui->profile_nm), s);
	free(s);
    }
    else
    {
	/* Use the current name as a base */
	gtk_entry_set_text (GTK_ENTRY (p_ui->profile_nm), p_ui->cur_cbox_txt);
    }

    gtk_editable_select_region (GTK_EDITABLE (p_ui->profile_nm), 0, -1);
    gtk_widget_grab_focus (p_ui->profile_nm);

    return;
}


/* Remove a profile name from the names list */

void names_list_remove(char *nm)
{
    char *s;
    GList *llink = NULL;

    llink = g_list_first(profile_names_head);

    while(llink != NULL)
    {
    	s = (char *) llink->data;

    	if (strcmp(s, nm) == 0)
    	{
	    profile_names_head = g_list_remove_link(profile_names_head, llink);
	    free(s);
	    g_list_free(llink);
	    profile_count--;
	    break;
    	}

	llink = g_list_next(llink);
    }

    return;
}


/* Update the default and current main profile if necessary */

void reset_default_profile(char *old_nm, char *new_nm, ProfileUi *p_ui)
{
    char *p;

    get_user_pref(DEFAULT_PROFILE, &p);

    if (strcmp(p, old_nm) == 0)
    {
	set_user_pref(DEFAULT_PROFILE, new_nm);
	write_user_prefs(p_ui->window);
    }

    if (strcmp(p_ui->cur_profile, old_nm) == 0)
    	strcpy(p_ui->cur_profile, new_nm);

    return;
}



/* Validate a profile name */

int validate_profile_nm(char *nm, ProfileUi *p_ui)
{
    char *s;

    /* Initial */
    s = (char *) malloc(strlen(nm) + 1);
    strcpy(s, nm);

    /* Check not empty or spaces */
    string_trim(s);

    if (*s == '\0')
    {
    	free(s);
	app_msg("APP0003", "Profile Name", p_ui->window);
    	return FALSE;
    }

    free(s);

    /* Check name is unique */
    profile_names = g_list_first(profile_names_head);

    while(profile_names != NULL)
    {
    	s = (char *) profile_names->data;

    	if (strcmp(s, nm) == 0)
    	{
	    app_msg("APP0004", "Profile Name", p_ui->window);
	    return FALSE;
    	}

	profile_names = g_list_next(profile_names);
    }

    return TRUE;
}


/* Add a new profile name to list */

void add_profile_list(char *new_nm, ProfileUi *p_ui)
{
    char *nm;
    char s[20];

    nm = (char *) malloc(strlen(new_nm) + 1);
    strcpy(nm, new_nm);
    profile_names = g_list_append(profile_names_head, nm);
    profile_count++;

    if (profile_names_head == NULL)
    	profile_names_head = profile_names;

    sprintf(s, "%d", profile_count);
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (p_ui->cbox_profile), s, nm);
    gtk_combo_box_set_active(GTK_COMBO_BOX (p_ui->cbox_profile), profile_count);

    return;
}


/* Remove from profile list and set new default if necessary */

void del_profile_list(ProfileUi *p_ui)
{
    int idx;

    idx = gtk_combo_box_get_active (GTK_COMBO_BOX (p_ui->cbox_profile));
    g_signal_handler_block (p_ui->cbox_profile, p_ui->preset_hndlr_id);
    gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (p_ui->cbox_profile), idx);

    names_list_remove(p_ui->cur_cbox_txt);

    g_signal_handler_unblock (p_ui->cbox_profile, p_ui->preset_hndlr_id);
    gtk_combo_box_set_active(GTK_COMBO_BOX (p_ui->cbox_profile), 0);

    return;
}


/* Save a profile (really just a session copy overriding the name) */

void save_profile(char *nm)
{
    char *profile_dir, *fn, *cam_nm;

    profile_dir = profile_dir_path();
    fn = (char *) malloc(strlen(nm) + strlen(profile_dir) + 2);
    sprintf(fn, "%s/%s", profile_dir, nm);

    /* Force the canera into the new profile */
    get_session(CAMERA, &cam_nm);
    set_session(CAMERA, cam_nm);

    save_session(fn);

    free(profile_dir);
    free(fn);

    return;
}


/* Read a profile */

void load_profile(char *nm)
{
    char *profile_dir;
    char *fn;

    profile_dir = profile_dir_path();
    fn = (char *) malloc(strlen(nm) + strlen(profile_dir) + 2);
    sprintf(fn, "%s/%s", profile_dir, nm);

    read_saved_session(fn);

    free(profile_dir);
    free(fn);

    return;
}


/* Rename a profile */

void rename_profile(char *old_nm, char *new_nm, ProfileUi *p_ui)
{
    char *profile_dir;
    char *fn_old, *fn_new;

    profile_dir = profile_dir_path();
    fn_old = (char *) malloc(strlen(old_nm) + strlen(profile_dir) + 2);
    sprintf(fn_old, "%s/%s", profile_dir, old_nm);
    fn_new = (char *) malloc(strlen(new_nm) + strlen(profile_dir) + 2);
    sprintf(fn_new, "%s/%s", profile_dir, new_nm);

    if (rename(fn_old, fn_new) != 0)
    {
    	sprintf(app_msg_extra, "%s", strerror(errno));
	log_msg("SYS9021", fn_old, "SYS9021", p_ui->window);
    }

    free(profile_dir);
    free(fn_old);
    free(fn_new);

    return;
}


/* Delete a profile */

void delete_profile(char *nm, ProfileUi *p_ui)
{
    char *profile_dir;
    char *fn;

    /* Delete file */
    profile_dir = profile_dir_path();
    fn = (char *) malloc(strlen(nm) + strlen(profile_dir) + 2);
    sprintf(fn, "%s/%s", profile_dir, nm);

    if (remove(fn) != 0)
    {
    	sprintf(app_msg_extra, "%s", strerror(errno));
	log_msg("SYS9020", fn, "SYS9020", p_ui->window);
    }

    free(profile_dir);
    free(fn);

    return;
}


/* Callback for current text currency */

static void OnSetProfile(GtkWidget *cbox, gpointer user_data)
{ 
    ProfileUi *ui;

    /* Get data */
    ui = (ProfileUi *) user_data;
    ui->cur_cbox_txt = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cbox)); 

    /* Set the availability of each button */
    set_btn_sensitivity(ui);

    return;
}


/* Callback for setting profile name */

void OnSetProfileName(GtkWidget *prf_nm, gpointer user_data)
{ 
    ProfileUi *ui;
    const gchar *nm;

    /* Get data */
    ui = (ProfileUi *) user_data;
    nm = gtk_entry_get_text (GTK_ENTRY (prf_nm)); 

    /* Validate name */
    if (validate_profile_nm((char *) nm, ui) == FALSE)
        return;

    switch(ui->btn_action)
    {
    	case 1:			// Save as
	    ui->btn_action = 0;

	    /* Save to file */
	    save_profile((char *) nm);

	    /* Add to list and set as current */
	    add_profile_list((char *) nm, ui);

	    break;

    	case 2:			// Rename
	    ui->btn_action = 0;

	    /* Rename file */
	    rename_profile(ui->cur_cbox_txt, (char *) nm, ui);

	    /* Default preference */
	    reset_default_profile(ui->cur_cbox_txt, (char *) nm, ui);

	    /* Replace in list */
	    del_profile_list(ui);
	    add_profile_list((char *) nm, ui);

	    break;

	default:
	    ui->btn_action = 0;
	    break;
    }

    /* Reset window */
    gtk_widget_show (ui->cmd_btn_grid);
    gtk_widget_hide (ui->nm_box);

    return;
}


/* Callback for set default profile */

void OnProfileDef(GtkWidget *btn, gpointer user_data)
{ 
    ProfileUi *ui;

    /* Get data */
    ui = (ProfileUi *) user_data;

    /* Save default preference */
    set_user_pref(DEFAULT_PROFILE, ui->cur_cbox_txt);
    write_user_prefs(ui->window);

    return;
}


/* Callback for Save */

void OnProfileSave(GtkWidget *btn, gpointer user_data)
{ 
    ProfileUi *ui;

    /* Get data */
    ui = (ProfileUi *) user_data;

    /* Save to file */
    save_profile((char *) ui->cur_cbox_txt);

    return;
}


/* Callback for Save As */

void OnProfileSaveAs(GtkWidget *btn, gpointer user_data)
{ 
    ProfileUi *ui;

    /* Get data */
    ui = (ProfileUi *) user_data;

    /* Get a new name */
    ui->btn_action = 1;
    gtk_widget_hide(ui->cmd_btn_grid);
    gtk_widget_show_all(ui->nm_box);

    /* Set entry to a (possibly default) name */
    set_profile_nm(ui);

    return;
}


/* Callback for Delete */

void OnProfileDel(GtkWidget *btn, gpointer user_data)
{ 
    ProfileUi *ui;
    gint res;

    /* Get data */
    ui = (ProfileUi *) user_data;

    /* Confirm */
    res = query_dialog(ui->window, "Confirm delete profile '%s'?", ui->cur_cbox_txt);

    if (res == GTK_RESPONSE_NO)
    	return;

    /* Default preference */
    reset_default_profile(ui->cur_cbox_txt, LAST_SESSION, ui);

    /* Delete file */
    delete_profile(ui->cur_cbox_txt, ui);

    /* Remove from list */
    del_profile_list(ui);

    return;
}


/* Callback for Re-name */

void OnProfileRename(GtkWidget *btn, gpointer user_data)
{ 
    ProfileUi *ui;

    /* Get data */
    ui = (ProfileUi *) user_data;

    /* Get a new name */
    ui->btn_action = 2;
    gtk_widget_hide(ui->cmd_btn_grid);
    gtk_widget_show_all(ui->nm_box);

    /* Set entry to a (possibly default) name */
    set_profile_nm(ui);

    return;
}


// Callback for window close
// Destroy the window and de-register the window 
// Refresh main window preset profiles

void OnProfileClose(GtkWidget *window, gpointer user_data)
{ 
    ProfileUi *ui;
    MainUi *m_ui;

    /* Get data */
    ui = (ProfileUi *) g_object_get_data (G_OBJECT (window), "ui");
    m_ui = g_object_get_data (G_OBJECT(ui->main_window), "ui");

    /* Close the window, free the screen data and block any secondary close signal */
    g_signal_handler_block (window, ui->close_hndlr_id);

    deregister_window(window);
    gtk_window_close(GTK_WINDOW(window));
    
    load_profiles(m_ui->cbox_profile, ui->cur_profile, m_ui->preset_hndlr_id, TRUE);
	
    g_free(ui->cur_profile);
    g_free(ui->cur_cbox_txt);
    free(ui);

    return;
}
