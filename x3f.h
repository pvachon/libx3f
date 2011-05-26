#ifndef __INCLUDE_X3F_H__
#define __INCLUDE_X3F_H__

struct x3f_file;

typedef int X3F_STATUS;

#define X3F_SUCCESS          0
#define X3F_BAD_FILENAME    -1
#define X3F_NO_MEMORY       -2
#define X3F_BAD_ARG         -3
#define X3F_NOT_X3F_FILE    -4
#define X3F_CANT_SEEK       -5
#define X3F_RANGE           -6
#define X3F_UNSUPP_MODE     -7 /* Unsupported image mode */
#define X3F_NOT_FOUND       -8
#define X3F_UNSPECIFIED    -99

X3F_STATUS x3f_initialize();

X3F_STATUS x3f_open(struct x3f_file **fp,
                    const char *filename,
                    const char *mode);

X3F_STATUS x3f_close(struct x3f_file *fp);

X3F_STATUS x3f_get_ver(struct x3f_file *fp, unsigned *major, unsigned *minor);

X3F_STATUS x3f_get_dims(struct x3f_file *fp,
                        unsigned *columns,
                        unsigned *rows,
                        unsigned *rotation);

X3F_STATUS x3f_get_id(struct x3f_file *fp, unsigned char *id, unsigned *mark);

X3F_STATUS x3f_get_white_balance(struct x3f_file *fp, char *white_balance);

X3F_STATUS x3f_get_extended_attrib(struct x3f_file *fp, int num,
                                   unsigned char *type, unsigned *value);

/* Functions for managing information about subimages */
X3F_STATUS x3f_get_subimage_dims(struct x3f_file *fp,
                                 unsigned image_id,
                                 unsigned *cols,
                                 unsigned *rows);

X3F_STATUS x3f_get_subimage_count(struct x3f_file *fp,
                                  unsigned *count);

/* Functions for managing read operations for X3F data */
X3F_STATUS x3f_get_min_read_block(struct x3f_file *fp,
                                  unsigned image_id,
                                  unsigned *width,
                                  unsigned *height);

X3F_STATUS x3f_read_image_data(struct x3f_file *fp,
                               unsigned image_id,
                               unsigned x,
                               unsigned y,
                               unsigned width,
                               unsigned height,
                               void *buf);

#endif /* __INCLUDE_X3F_H__ */
