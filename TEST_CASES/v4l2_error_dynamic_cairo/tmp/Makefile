CC=cc
CFLAGS=-I. `pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 cairo`
DEPS=
OBJ= test_valve.o
LIBS= `pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 cairo`

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test_valve: $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJ)
