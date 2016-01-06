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
** Description:	Application session settings
**
** Author:	Anthony Buckley
**
** History
**	17-Mar-2014	Initial
**
*/


// Structure to contain all current settings.
// This is loaded (if it exists) on startup, matched to a camera and saved on exit.
// Items are updated as they are altered.
// The format of the settings file in the application directory is as follows:-
//	name_key|setting_value		- leading and trailing spaces are ignored
//					- no spaces in key

#ifndef SESSIONDATA_H
#define SESSIONDATA_H

typedef struct _SessionData
{
    char key[10];
    char *val;
    char *reset_val;
    int save_flg;
} SessionData;

// Key values for each session detail stored

#define CAMERA "CAMERA"
#define CLRFMT "CLRFMT"
#define RESOLUTION "RESLTN"
#define FPS "FPS"

#endif
