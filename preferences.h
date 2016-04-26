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
** Description:	User Preferences
**
** Author:	Anthony Buckley
**
** History
**	8-Aug-2014	Initial
**
*/


// Structure to contain all user preferences.
// This list is loaded on startup if it exists. If not a default set is created.
// Items are updated on the preferences screen.
// The format of the settings file in the application directory is as follows:-
//	name_key|setting_value		- leading and trailing spaces are ignored
//					- no spaces in key

#ifndef USER_PREFS_H
#define USER_PREFS_H

#define PREF_KEY_SZ 25

typedef struct _UserPrefData
{
    char key[PREF_KEY_SZ];
    char *val;
} UserPrefData;

// Key values for each session detail stored

#define IMAGE_TYPE "IMAGETYPE"
#define JPEG_QUALITY "JPEGQUAL"
#define SNAPSHOT_DELAY "SNP_DELAY"
#define FITS_BITS "FITS_BITS"
#define CAPTURE_FORMAT "CAPTFMT"
#define CAPTURE_DURATION "CAPTDUR"
#define CAPTURE_LOCATION "CAPTDIR"
#define FN_ID "FN_ID"
#define FN_TITLE "FN_TITLE"
#define FN_TIMESTAMP "FN_TMSTMP"
#define DEFAULT_PROFILE "DEF_PROFILE"
#define MPG2_FRAMERATE "MPEG2_FPS"
#define AUDIO_MUTE "AUDIO"
#define WARN_EMPTY_TITLE "NO_TITLE"

#endif
