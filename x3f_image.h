/* Private header for declaring different image decoding types */
#ifndef __INCLUDE_X3F_IMAGE_H__
#define __INCLUDE_X3F_IMAGE_H__

#include <x3f.h>
#include <x3f_priv.h>

struct x3f_image_mode {
    unsigned type; /* The type code, as seen in the image section header */
    const char *name; /* Name of the mode */

    /* Check if the requested read can be serviced */
    X3F_STATUS (*check_read)(struct x3f_image *fp,
                             unsigned x, unsigned y,
                             unsigned w, unsigned h);

    /* Read a block of the image */
    X3F_STATUS (*read_image)(struct x3f_file *fp, struct x3f_image *img,
                             unsigned x, unsigned y,
                             unsigned w, unsigned h, void *buf);

    /* Do initial setup */
    X3F_STATUS (*setup)(struct x3f_file *fp, struct x3f_image *img);

    /* Get minimum read block supported */
    X3F_STATUS (*get_min_block)(struct x3f_file *fp, struct x3f_image *img,
                                unsigned *w, unsigned *h);
};

/* Add a mode */
X3F_STATUS x3f_add_mode(struct x3f_image_mode *mode);

/* Register Huffman mode */
X3F_STATUS x3f_huff_register();

/* Register JPEG mode */
/* X3F_STATUS x3f_jpeg_register(); */

/* Register RAW mode */
/* X3F_STATUS x3f_raw_register(); */

#endif /* __INCLUDE_X3F_IMAGE_H__ */

