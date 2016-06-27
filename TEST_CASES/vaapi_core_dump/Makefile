CC=cc
CFLAGS=-I. `pkg-config --cflags gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0`
DEPS=
OBJ= testcoredump.o
LIBS= `pkg-config --libs gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0 libv4l2`

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

testcoredump: $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJ)
