
#  Copyright (C) 2016 Anthony Buckley
# 
#  This file is part of AstroCTC.
# 
#  AstroCTC is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
# 
#  AstroCTC is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with AstroCTC.  If not, see <http://www.gnu.org/licenses/>.

CC=cc
CFLAGS=-I. `pkg-config --cflags gtk+-3.0 gstreamer-1.0 cairo` 
CFLAGS2=-Wno-deprecated-declarations
DEPS = defs.h main.h cam.h session.h preferences.h codec.h version.h
OBJ = astro_main.o callbacks.o camera.o main_ui.o utility.o gst_view_capture.o camera_info_ui.o prefs_ui.o view_file_ui.o snapshot.o prefs_ui.o profiles_ui.o codec_ui.o capture_ui.o snapshot_ui.o about_ui.o other_ctrl_ui.o
LIBS = `pkg-config --libs gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0 libv4l2 cairo libpng`
LIBS2 = -ljpeg -lpthread
LIBS3 = `pkg-config --libs --static cfitsio`

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(CFLAGS2)

astroctc: $(OBJ)
	$(CC) -o $@ $^ $(LIBS) $(LIBS2) $(LIBS3)

clean:
	rm -f $(OBJ)
