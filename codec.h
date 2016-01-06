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
** Description:	Codec structure definition
**
** Author:	Anthony Buckley
**
** History
**	20-Oct-2014	Initial
**
*/


/* Defines */

#ifndef CODEC_HDR
#define CODEC_HDR
#endif

/* Includes */


/* Structure to hold information about each codec used */

typedef struct _codec
{
    char fourcc[5];
    char short_desc[10];
    char long_desc[50];
    char extn[5];
    char encoder[20];
    char muxer[30];
} codec_t;


/* Structure to hold some encoder properties */

typedef struct _encoder
{
    char encoder[20];
    char property_nm[30];
    char default_val[30];
    int type;
    char tooltip[160];
} encoder_t;
