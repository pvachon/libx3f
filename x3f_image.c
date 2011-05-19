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

/* Functions for manipulating image segments in an X3F file.
 */
#include <x3f.h>
#include <x3f_priv.h>
#include <x3f_image.h>

#include <stdlib.h>

static struct x3f_image_mode **modes = NULL;
static int x3f_image_mode_count = 0;
static int x3f_initialized = 0;

X3F_STATUS x3f_setup_image(struct x3f_file *fp,
                           struct x3f_image *img);

static X3F_STATUS x3f_get_image_by_id(struct x3f_file *fp,
                                      unsigned image_id,
                                      struct x3f_image **img)
{
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(img);

    *img = NULL;

    if (image_id > fp->image_count) { return X3F_RANGE; }

    *img = fp->images[image_id];

    return X3F_SUCCESS;
}

X3F_STATUS x3f_get_min_read_block(struct x3f_file *fp,
                                  unsigned image_id,
                                  unsigned *width,
                                  unsigned *height)
{
    X3F_STATUS ret;
    struct x3f_image *img = NULL;

    X3F_ASSERT_ARG(fp);

    if ( (ret = x3f_get_image_by_id(fp, image_id, &img)) < 0 ) {
        X3F_TRACE("Unable to find image %u\n", image_id);
        return ret;
    }

    if (img->mode == NULL) {
        ret = x3f_setup_image(fp, img);

        if (ret < 0) {
            X3F_TRACE("Error while setting up access to image");
            return ret;
        }
    }

    if ( (ret = img->mode->get_min_block(fp, img, width, height)) < 0) {
        return ret;
    }

    return X3F_SUCCESS;
}

X3F_STATUS x3f_read_image_data(struct x3f_file *fp,
                               unsigned image_id,
                               unsigned x,
                               unsigned y,
                               unsigned width,
                               unsigned height,
                               void *buf)
{
    struct x3f_image *img = NULL;
    X3F_STATUS ret;

    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(buf);

    if ( (ret  = x3f_get_image_by_id(fp, image_id, &img)) < 0 ) {
        X3F_TRACE("Unable to find image %u\n", image_id);
        return ret;
    }

    if (img->mode == NULL) {
        ret = x3f_setup_image(fp, img);

        if (ret < 0) {
            X3F_TRACE("Error while setting up access to image");
            return ret;
        }
    }

    return img->mode->read_image(fp, img, x, y, width, height, buf);
}

static X3F_STATUS x3f_find_mode(unsigned type,
                                struct x3f_image_mode **mode)
{
    int i;
    if (modes == NULL) {
        return X3F_RANGE;
    }

    for (i = 0; i < x3f_image_mode_count; i++) {
        if (modes[i]->type == type) {
            *mode = modes[i];
            return X3F_SUCCESS;
        }
    }

    return X3F_UNSUPP_MODE;
}

X3F_STATUS x3f_setup_image(struct x3f_file *fp,
                           struct x3f_image *img)
{
    struct x3f_image_mode *mode = NULL;
    X3F_STATUS ret;

    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(img);

    X3F_TRACE("Setting up image access");

    if ( (ret = x3f_find_mode(img->format, &mode)) < 0) {
        if (ret != X3F_UNSUPP_MODE)
            X3F_TRACE("Failed to setup image access");
        else
            X3F_TRACE("Unsupported mode: %u", img->format);
        return ret;
    }

    img->mode = mode;
    ret = mode->setup(fp, img);

    return ret;
}

X3F_STATUS x3f_add_mode(struct x3f_image_mode *mode)
{
    int off = 0;
    X3F_ASSERT_ARG(mode);

    X3F_TRACE("Registering mode %s (%d)", mode->name, mode->type);

    x3f_image_mode_count++;
    modes = (struct x3f_image_mode **)realloc(modes,
                sizeof(struct x3f_image_mode*) * x3f_image_mode_count);

    modes[off] = mode;

    return X3F_SUCCESS;
}

X3F_STATUS x3f_initialize()
{
    if (x3f_initialized) return X3F_SUCCESS;

    x3f_huff_register();

    /* TODO: atexit teardown registration */

    x3f_initialized = 1;

    return X3F_SUCCESS;
}

