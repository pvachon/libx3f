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

/* File containing housekeeping functions for opening/closing/manipulating
 * file pointers associated with libx3f
 */
#include <x3f.h>
#include <x3f_priv.h>
#include <x3f_priv_sh.h>

#include <string.h>
#include <stdlib.h>

static struct x3f_file *x3f_fp_construct(const char *filename,
                                         const char *mode)
{
    struct x3f_file *fp = NULL;
    int len = strlen(filename);
    int ret = 0;

    if (len != 0) {
        fp = (struct x3f_file *)malloc(sizeof(struct x3f_file));
        if (!fp) {
            X3F_TRACE("failed to allocate %d bytes", (int)sizeof(struct x3f_file));
            goto done;
        }

        memset(fp, 0, sizeof(struct x3f_file));

        if ((ret = x3f_fopen(fp, filename, mode)) < 0) {
            free(fp);
            goto done;
        }

        fp->filename = (char *)malloc(len);

        strcpy(fp->filename, filename);

    } else {
        X3F_TRACE("empty filename passed");
    }

done:
    return fp;
}

static X3F_STATUS x3f_fp_destroy(struct x3f_file *fp)
{
    X3F_ASSERT_ARG(fp);

    x3f_cleanup_all_sections(fp);

    if (fp->filename) {
        free(fp->filename);
        fp->filename = NULL;
    }

    if (fp->dir.entries) {
        free(fp->dir.entries);
        fp->dir.entries = NULL;
    }

    if (fp->fp) {
        x3f_fclose(fp);
    }

    free(fp);

    return X3F_SUCCESS;
}

X3F_STATUS x3f_identify(struct x3f_file *fp)
{
    char hdr_buf[X3F_HEADER_LEN];
    uint32_t version = 0;
    uint16_t major, minor;
    int ret = 0;
    size_t count = 0;

    if ((ret = x3f_fseek(fp, 0, X3F_SEEK_SET)) < 0) {
        X3F_TRACE("failed to seek");
        return ret;
    }


    if ((ret = x3f_fread(fp, X3F_HEADER_LEN, 1, hdr_buf, &count)) < 0) {
        X3F_TRACE("fread failed");
        return ret;
    }

    if (*((uint32_t*)hdr_buf) != X3F_MAGIC) {
        X3F_TRACE("file %s is not an X3F file", fp->filename);
        X3F_TRACE("got magic '%08x' expecting '%08x'",
            *((uint32_t*)hdr_buf), X3F_MAGIC);
        return X3F_NOT_X3F_FILE;
    }

    /* Read version word */
    version = *(((uint32_t*)hdr_buf) + 1);

    major = version >> 16;
    minor = version & 0xffff;

    X3F_TRACE("Version number: %d.%d (%08x)", major, minor, version);

    return X3F_SUCCESS;
}

static X3F_STATUS x3f_read_header(struct x3f_file *fp)
{
    uint32_t header[X3F_FULL_HEADER/4];

    uint8_t *ext_type;
    uint32_t *ext_val;

    X3F_STATUS ret;
    size_t count;
    uint32_t w;
    int i;

    X3F_ASSERT_ARG(fp);

    if ((ret = x3f_fseek(fp, 0, X3F_SEEK_SET)) < 0) {
        return ret;
    }

    if ((ret = x3f_fread(fp, X3F_FULL_HEADER, 1, header, &count)) < 0 ||
        count < 1)
    {
        return ret;
    }

    w = header[X3F_HEADER_VER/4];

    fp->hdr.ver_major = w >> 16;
    fp->hdr.ver_minor = w & 0xffff;

    memcpy(fp->hdr.id, &header[X3F_HEADER_ID/4], 16);

    fp->hdr.mark = header[X3F_HEADER_MARK/4];
    fp->hdr.columns = header[X3F_HEADER_COLUMNS/4];
    fp->hdr.rows = header[X3F_HEADER_ROWS/4];
    fp->hdr.rotation = header[X3F_HEADER_ROTATION/4];

    memcpy(fp->hdr.white_balance, &header[X3F_HEADER_WHITEBAL/4], 32);

    ext_type = (uint8_t*)&header[X3F_HEADER_EXTENDED_TYPES/4];
    ext_val = &header[X3F_HEADER_EXTENDED_DATA/4];

    for (i = 0; i < 32; i++) {
        fp->hdr.ext[i].type = ext_type[i];
        fp->hdr.ext[i].value = ext_val[i];
    }

    return X3F_SUCCESS;
}

static X3F_STATUS x3f_read_directory(struct x3f_file *fp)
{
    uint32_t dir_off = 0;
    X3F_STATUS ret;
    uint32_t version, id, num;
    size_t count = 0;
    int i;

    X3F_ASSERT_ARG(fp);

    if ( (ret = x3f_fseek(fp, -4, X3F_SEEK_END)) < 0 ) {
        return ret;
    }

    if ( (ret = x3f_fread(fp, 4, 1, &dir_off, &count)) < 0 ) {
        return ret;
    }

    X3F_TRACE("Directory offset = %08x", dir_off);

    fp->dir_offset = dir_off;

    if ( (ret = x3f_fseek(fp, dir_off, X3F_SEEK_SET)) < 0 ) {
        return ret;
    }

    /* Read in directory */
    if ( (ret = x3f_fread(fp, 4, 1, &id, &count)) < 0 ) {
        return ret;
    }

    if (id != X3F_DIR_SEC) {
        X3F_TRACE("Section type found: %08x is not what we wanted\n",
            id);
        return X3F_NOT_X3F_FILE;
    }

    if ( (ret = x3f_fread(fp, 4, 1, &version, &count)) < 0 ) {
        return ret;
    }

    fp->dir.version = version;

    if ( (ret = x3f_fread(fp, 4, 1, &num, &count)) < 0) {
        return ret;
    }

    fp->dir.count = num;

    if (num == 0) {
        return X3F_RANGE;
    }

    X3F_TRACE("Found a directory with %u entries", fp->dir.count);

    fp->dir.entries = (struct x3f_directory_entry*)malloc(
        sizeof(struct x3f_directory_entry) * num);

    if (fp->dir.entries == NULL) {
        return X3F_NO_MEMORY;
    }

    for (i = 0; i < num; i++) {
        uint32_t type, offset, length;
        if ( (ret = x3f_fread(fp, 4, 1, &offset, &count)) < 0 ) {
            return ret;
        }

        if ( (ret = x3f_fread(fp, 4, 1, &length, &count)) < 0 ) {
            return ret;
        }

        if ( (ret = x3f_fread(fp, 4, 1, &type, &count)) < 0 ) {
            return ret;
        }

        fp->dir.entries[i].offset = offset;
        fp->dir.entries[i].length = length;
        fp->dir.entries[i].type = type;
        fp->dir.entries[i].record = -1;
    }

    return X3F_SUCCESS;
}

#ifdef _DEBUG
static const char* x3f_id_to_string(unsigned id, char *out)
{
    int i;
    for (i = 0; i < 4; i++) {
        out[i] = (char)(id >> (i * 8));
    }

    return out;
}

static void x3f_dump_dir(struct x3f_file *fp)
{
    int i;
    char out[5];
    out[4] = '\0';

    printf("%-12s %-12s %-12s\n",
           "------------",
           "------------",
           "------------");
    printf("%-12s %-12s %-12s\n",
            "Type",
            "Offset",
            "Length");
    printf("%-12s %-12s %-12s\n",
           "------------",
           "------------",
           "------------");
    for (i = 0; i < fp->dir.count; i++) {
        printf("%-12.4s %-12.08x %-12.08x\n",
            x3f_id_to_string(fp->dir.entries[i].type, out),
            fp->dir.entries[i].offset, fp->dir.entries[i].length);
    }
}
#endif

X3F_STATUS x3f_open(struct x3f_file **fp,
                    const char *filename,
                    const char *mode)
{
    int ret = 0, i;
    struct x3f_file *fpt = NULL;

    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(filename);
    X3F_ASSERT_ARG(mode);

    *fp = NULL;

    fpt = x3f_fp_construct(filename, mode);

    if ((ret = x3f_identify(fpt)) < 0) {
        x3f_fp_destroy(fpt);
        return ret;
    }

    if ((ret = x3f_read_header(fpt)) < 0) {
        x3f_fp_destroy(fpt);
        return ret;
    }

    if ((ret = x3f_read_directory(fpt)) < 0) {
        x3f_fp_destroy(fpt);
        return ret;
    }

#ifdef _DEBUG
    x3f_dump_dir(fpt);
#endif

    for (i = 0; i < fpt->dir.count; i++) {
        if ( (ret = x3f_read_section(fpt, i)) < 0 ) {
            x3f_fp_destroy(fpt);
            return ret;
        }
    }

    *fp = fpt;

    return X3F_SUCCESS;
}

X3F_STATUS x3f_close(struct x3f_file *fp)
{
    int ret = 0;

    X3F_ASSERT_ARG(fp);

    if ((ret = x3f_fp_destroy(fp)) < 0) {
        return ret;
    }

    return X3F_SUCCESS;
}

X3F_STATUS x3f_get_subimage_count(struct x3f_file *fp,
                                  unsigned *count)
{
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(count);

    *count = fp->image_count;

    return X3F_SUCCESS;
}

X3F_STATUS x3f_get_subimage_dims(struct x3f_file *fp,
                                 unsigned subimage,
                                 unsigned *cols,
                                 unsigned *rows)
{
    X3F_ASSERT_ARG(fp)

    if (subimage > fp->image_count) return X3F_RANGE;

    if (rows) *rows = fp->images[subimage]->rows;
    if (cols) *cols = fp->images[subimage]->cols;

    return X3F_SUCCESS;
}

