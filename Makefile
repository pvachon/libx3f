CFLAGS?= # Just in case
LDFLAGS?= # Just in case

# Custom CFLAGS
CFLAGS+=-O0 -g

# Custom LDFLAGS
LDFLAGS+=-liconv

# don't edit anything below this
OBJ=x3f.o x3f_info.o x3f_fm.o x3f_dir.o x3f_utf16.o x3f_image.o \
	x3f_image_huff.o x3f_camera_data.o x3f_huff.o
CFLAGS+=-Wall -I. -D_DEBUG
CC?=gcc

TARGET=libx3f

LDFLAGS+=-shared -o $(TARGET).so


PHONY := all clean

all: $(TARGET).so

$(TARGET).so: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(OBJ)
	$(RM) $(TARGET).so

