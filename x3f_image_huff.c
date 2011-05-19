/*
  Copyright (c) 2011, Phil Vachon <phil@cowpig.ca>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  - Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  - Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Implementation of Huffman coding for images.
 */
#include <x3f.h>
#include <x3f_priv.h>

#include <x3f_image.h>
#include <x3f_huff.h>

#include <stdlib.h>
#include <string.h>

static X3F_STATUS x3f_huff_read_table(struct x3f_file *fp,
                                      struct x3f_huff_mode_info *inf)
{
    X3F_STATUS ret;
    unsigned size, val;
    uint8_t entry[2];
    size_t count = 0;
    unsigned entry_count = 0;

    inf->root = x3f_new_huff_node();

    do {
        if ( (ret = x3f_fread(fp, 2, 1, entry, &count)) < 0) {
            return ret;
        }

        X3F_TRACE("{ .size = 0x%02x, .value = 0x%02x }, ",
            entry[0], entry[1]);

        size = entry[0];
        val = entry[1];

        /* Construct Huffman tree nodes */
        x3f_huff_append_node(inf->root, size, val, entry_count);
        entry_count++;
    } while (size != 0);

#ifdef _DEBUG
    print_huffman_tree(inf->root, 0, 0);
#endif

    return X3F_SUCCESS;
}

static X3F_STATUS x3f_huff_setup_table(struct x3f_file *fp,
                                       struct x3f_image *img,
                                       struct x3f_huff_mode_info *inf)
{
    X3F_STATUS ret;
    uint8_t header[8];
    size_t count;
    int i;

    if ( (ret = x3f_lock(fp)) < 0 ) {
        return X3F_NO_MEMORY;
    }

    if ( (ret = x3f_fseek(fp, img->image_offset, X3F_SEEK_SET)) < 0 ) {
        goto done;
    }

    if ( (ret = x3f_fread(fp, 8, 1, header, &count)) < 0 ) {
        goto done;
    }

    inf->predictor[0] = X3F_SHORT_AT(header, 0);
    inf->predictor[1] = X3F_SHORT_AT(header, 2);
    inf->predictor[2] = X3F_SHORT_AT(header, 4);
    inf->predictor[3] = X3F_SHORT_AT(header, 6);

    if ( (ret = x3f_huff_read_table(fp, inf)) < 0 ) {
        goto done;
    }

    for (i = 0; i < 3; i++) {
        uint32_t off = 0;
        if ( (ret = x3f_fread(fp, sizeof(off), 1, &off, &count)) < 0 ) {
            goto done;
        }
        inf->plane_size[i] = off;
        X3F_TRACE("Plane %d offset: %08x", i + 1, off);
    }

    if ( (ret = x3f_ftell(fp, &inf->start_off)) < 0) {
        goto done;
    }

done:
    if ( (ret = x3f_unlock(fp)) < 0 ) {
        return X3F_NO_MEMORY;
    }

    return ret;
}

static X3F_STATUS x3f_huff_setup(struct x3f_file *fp,
                                 struct x3f_image *img)
{
    struct x3f_huff_mode_info *inf = NULL;
    X3F_STATUS ret;

    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(img);

    inf = (struct x3f_huff_mode_info *)malloc(sizeof(struct x3f_huff_mode_info));

    if (inf == NULL) {
        return X3F_NO_MEMORY;
    }

    memset(inf, 0, sizeof(struct x3f_huff_mode_info));

    if ( (ret = x3f_huff_setup_table(fp, img, inf)) < 0 ) {
        return ret;
    }

    img->mode_info = (void*)inf;
    return X3F_SUCCESS;
}

static X3F_STATUS x3f_huff_check_read(struct x3f_image *img,
                                      unsigned x, unsigned y,
                                      unsigned w, unsigned h)
{
    X3F_ASSERT_ARG(img);

    if (x != 0 || y != 0) {
        return X3F_RANGE;
    }

    if (img->rows != h || img->cols != w) {
        return X3F_RANGE;
    }

    return X3F_SUCCESS;
}

static X3F_STATUS x3f_huff_read_image(struct x3f_file *fp, struct x3f_image *img,
                                      unsigned x, unsigned y,
                                      unsigned w, unsigned h, void *buf)
{
    struct x3f_huff_mode_info *inf = NULL;
    int plane;
    uint16_t *cur = NULL;
    X3F_STATUS ret = X3F_SUCCESS;
    size_t count;

    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(img);
    X3F_ASSERT_ARG(buf);

    if (x3f_huff_check_read(img, x, y, w, h) < 0) {
        return X3F_RANGE;
    }

    if ( (ret = x3f_lock(fp)) < 0 ) {
        return ret;
    }

    /* Get the state structure */
    inf = (struct x3f_huff_mode_info *)img->mode_info;

    if ( (ret = x3f_fseek(fp, inf->start_off, X3F_SEEK_SET)) < 0 ) {
        goto done;
    }

    for (plane = 0; plane < 3; plane++) {
        uint8_t *encoded = NULL;
        uint32_t plane_size = ((inf->plane_size[plane] + 15)/16) * 16;
        printf("plane = %d\n", plane);
        cur = &((uint16_t*)buf)[plane * img->rows * img->cols];

        encoded = (uint8_t*)malloc(plane_size);

        if (encoded == NULL) return X3F_NO_MEMORY;

        /* Read in buffer */
        if ( (ret = x3f_fread(fp, plane_size, 1, encoded, &count) < 0) )
        {
            X3F_TRACE("Failed to read %u bytes\n", plane_size);
            goto done;
        }

        x3f_quantized_huff_decode(inf->root,
                                  inf->predictor[plane],
                                  encoded,
                                  plane_size,
                                  cur,
                                  img->rows,
                                  img->cols);

        free(encoded);
    }

done:
    ret = x3f_unlock(fp);
    return ret;
}

static X3F_STATUS x3f_huff_get_min_block(struct x3f_file *fp, struct x3f_image *img,
                                        unsigned *w, unsigned *h)
{
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(img);

    if (h) *w = img->cols;
    if (w) *h = img->rows;

    return X3F_SUCCESS;
}

struct x3f_image_mode x3f_huff_mode = {
    .type = 30,
    .name = "Special Huffman compression (1024-entry)",
    .check_read = x3f_huff_check_read,
    .read_image = x3f_huff_read_image,
    .setup = x3f_huff_setup,
    .get_min_block = x3f_huff_get_min_block
};

X3F_STATUS x3f_huff_register()
{
    return x3f_add_mode(&x3f_huff_mode);
}

