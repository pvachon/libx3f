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

#include <x3f.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAT_GET(mat, row, col) \
    ((mat)[(row) * 3 + (col)])

void mat_mul(float matrix[9], int16_t *pixels)
{
    int i, j, k;

    float pin[3] = { pixels[0], pixels[1], pixels[2] };

    float pout[3] = { 0.0f, 0.0f, 0.0f };

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            pout[i] += pin[j] * MAT_GET(matrix, i, j);
        }
    }

    pixels[0] = (int16_t)(pout[0] + 0.5f);
    pixels[1] = (int16_t)(pout[1] + 0.5f);
    pixels[2] = (int16_t)(pout[2] + 0.5f);
}

void dump_buf(const char *file, float matrix[9], uint16_t *buf, size_t bytes,
    int cols, int rows)
{
    FILE *fp;
    uint16_t *buf_out, *outbuf, *pix;
    size_t i, j;
    size_t stride = bytes / 6;

    outbuf = (uint16_t*)malloc(cols * rows * 3 * sizeof(uint16_t));

    buf_out = outbuf;

    if (buf_out == NULL) {
        perror(malloc);
        exit(-1);
    }

    for (i = 0; i < rows*cols; i++) {
        pix = buf_out;
        for (j = 0; j < 3; ++j) {
            *buf_out = buf[(rows * cols * j) + i];
            buf_out++;
        }
        mat_mul(matrix, pix);
    }

    printf("\nDumping to %s\n", file);
    fp = fopen(file, "w+");
    if (fp == NULL) {
        perror(fopen);
        exit(-1);
    }
    fprintf(fp, "P6\n%d %d\n65535\n", cols, rows);

    fwrite(outbuf, bytes, 1, fp);

    free(outbuf);

    fclose(fp);
}

int main(int argc, char *argv[])
{
    struct x3f_file *fp = NULL;
    unsigned rows, cols, cp2_size;
    uint16_t *buf = NULL;
    float cp2_buf[9];
    int i, j;


    if (3 > argc) {
        printf("usage: %s [filename] [outname]\n", argv[0]);
        return -1;
    }

    x3f_initialize();
    x3f_open(&fp, argv[1], "r");

    x3f_get_subimage_dims(fp, 1, &cols, &rows);

    printf("Reading image %d x %d\n", cols, rows);

    buf = (uint16_t*)malloc(cols * rows * 3 * 2);

    if (buf == NULL) {
        printf("failed to allocate out buffer\n");
        goto done;
    }

    memset(buf, 0, cols * rows * 3 * 2);

    x3f_read_image_data(fp, 1, 0, 0, cols, rows, buf);
    /* Get CP2_Matrix */
    if (x3f_get_array(fp, "CP2_Matrix", NULL, &cp2_size) != X3F_SUCCESS) {
        printf("Failed to get CP2_Matrix array!");
        goto done;
    } else {
        printf("CP2_Matrix size = %u bytes\n", cp2_size);
    }

    if (x3f_get_array(fp, "CP2_Matrix", cp2_buf, NULL) != X3F_SUCCESS) {
        printf("Could not get CP2_Matrix array values!");
    } else {
        for (i = 0; i < 3; i++) {
            printf("[ ");
            for (j = 0; j < 3; j++) {
                printf("%f ", (double)(cp2_buf[i * 3 + j]));
            }
            printf("]\n");
        }
    }
    dump_buf(argv[2], cp2_buf, buf, cols * rows * 3 * 2, cols, rows);
    free(buf);

done:
    x3f_close(fp);

    return 0;
}
