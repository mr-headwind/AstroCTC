bin_PROGRAMS = astroctc
astroctc_SOURCES = \
		cam.h               \
		codec.h             \
		defs.h              \
		main.h              \
		preferences.h       \
		session.h           \
		version.h           \
		about_ui.c          \
		astro_main.c        \
		callbacks.c         \
		camera.c            \
		camera_info_ui.c    \
		capture_ui.c        \
		codec_ui.c          \
		gst_view_capture.c  \
		main_ui.c           \
		other_ctrl_ui.c     \
		prefs_ui.c          \
		profiles_ui.c       \
		snapshot.c          \
		snapshot_ui.c       \
		utility.c           \
		view_file_ui.c

astroctc_CFLAGS = $(X_CFLAGS) -Wno-deprecated-declarations
astroctc_LDADD = $(X_LIBS) -ljpeg -lpthread -lm
