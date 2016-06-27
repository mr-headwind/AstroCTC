CC=cc
CFLAGS=-I. `pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 cairo`
DEPS=
OBJ= test_dyn.o
LIBS= `pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 cairo`

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test_dyn: $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJ)
