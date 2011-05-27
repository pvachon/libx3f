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

/* Functions for reading and managing metadata associated with the various
 * different data sections in an X3F image. This does not handle anything
 * to do with image data (but sets up the structures used in reading and
 * manipulating image data).
 */
#include <x3f.h>
#include <x3f_priv.h>
#include <x3f_priv_sh.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static X3F_STATUS x3f_destroy_prop_record(struct x3f_prop_table *table)
{
    int i;

    X3F_ASSERT_ARG(table);

    for (i = 0; i < table->count; i++) {
        if (table->entries[i].name != NULL)
            free(table->entries[i].name);

        if (table->entries[i].value != NULL)
            free(table->entries[i].value);
    }

    free(table->entries);

    free(table);

    return X3F_SUCCESS;
}

static X3F_STATUS x3f_find_first_free_prop(struct x3f_file *fp,
                                           unsigned *num)
{
    int i;
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(num);

    for (i=0; i < fp->prop_table_count; i++) {
        if (fp->prop_tables[i] == NULL) {
            *num = i;
            return X3F_SUCCESS;
        }
    }

    return X3F_NO_MEMORY;
}

static X3F_STATUS x3f_destroy_image(struct x3f_image *image)
{
    X3F_ASSERT_ARG(image);

    memset(image, 0, sizeof(struct x3f_image));

    free(image);

    return X3F_SUCCESS;
}

static X3F_STATUS x3f_find_first_free_image(struct x3f_file *fp,
                                            unsigned *num)
{
    int i;
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(num);

    for (i = 0; i < fp->image_count; i++) {
        if (fp->images[i] == NULL) {
            *num = i;
            return X3F_SUCCESS;
        }
    }

    return X3F_NO_MEMORY;
}

static X3F_STATUS x3f_read_prop_section(struct x3f_file *fp,
                                        struct x3f_directory_entry *dirent)
{
    X3F_STATUS ret;
    unsigned prop_id = 0xfffffffful;
    char *buf = NULL, *propbuf = NULL;
    uint32_t header[X3F_PROP_HEADER_LEN/4];
    size_t count;
    struct x3f_prop_table *tbl;
    unsigned entry_count;
    int i;

    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(dirent);

    if (dirent->record != -1) {
        return X3F_SUCCESS;
    }

    if ( (ret = x3f_lock(fp)) < 0) {
        return ret;
    }

    if ( (ret = x3f_fseek(fp, dirent->offset, SEEK_SET)) < 0 ) {
        goto done;
    }

    if ( (ret = x3f_fread(fp, X3F_PROP_HEADER_LEN, 1, header, &count)) < 0 )
    {
        goto done;
    }

    if (header[X3F_PROP_HEADER_LENGTH/4] == 0) {
        ret = X3F_RANGE;
        goto done;
    }

    entry_count = header[X3F_PROP_HEADER_COUNT/4];

    propbuf = (char *)malloc(8 * entry_count);
    buf = (char *)malloc(header[X3F_PROP_HEADER_LENGTH/4]);

    if (buf == NULL) {
        ret = X3F_NO_MEMORY;
        goto done;
    }

    if ( (ret = x3f_fread(fp, 8 * entry_count, 1, propbuf, &count)) < 0 ) {
        goto done;
    }

    if ( (ret = x3f_fread(fp, header[X3F_PROP_HEADER_LENGTH/4], 1, buf, &count)) < 0)
    {
        goto done;
    }


    X3F_TRACE("Parsing property table");

    ret = x3f_find_first_free_prop(fp, &prop_id);

    tbl = (struct x3f_prop_table *)malloc(sizeof(struct x3f_prop_table));

    if (tbl == NULL) {
        ret = X3F_NO_MEMORY;
        goto done;
    }

    memset(tbl, 0, sizeof(struct x3f_prop_table));

    tbl->entries = (struct x3f_prop_table_entry*)malloc(
        sizeof(struct x3f_prop_table_entry) * entry_count);

    memset(tbl->entries, 0, sizeof(struct x3f_prop_table_entry) * entry_count);

    for (i = 0; i < entry_count; i++) {
        size_t proplen, vallen;
        size_t propbytes, propinbytes, valinbytes, valbytes;
        char *prop, *value;

        tbl->entries[i].name_offset = ((uint32_t*)propbuf)[2 * i];
        tbl->entries[i].val_offset = ((uint32_t*)propbuf)[2 * i + 1];

        /* Extract values from string tables */
        prop = (char*)(&buf[tbl->entries[i].name_offset]);
        value = (char*)(&buf[tbl->entries[i].val_offset]);

        proplen = x3f_utf16_strlen(prop);
        vallen = x3f_utf16_strlen(value);

        if (proplen > 0) {
            tbl->entries[i].name = (char *)malloc(sizeof(uint16_t) * proplen);

            if (tbl->entries[i].name == NULL) {
                ret = X3F_NO_MEMORY;
                goto done;
            }

            memset(tbl->entries[i].name, 0, sizeof(uint16_t) * proplen);

            propinbytes = propbytes = sizeof(uint16_t) * proplen;
            x3f_utf16_to_utf8(tbl->entries[i].name, &propbytes,
                    prop, &propinbytes);
        } else {
            tbl->entries[i].name = NULL;
        }

        if (vallen > 0) {

            tbl->entries[i].value = (char *)malloc(sizeof(uint16_t) * proplen);

            if (tbl->entries[i].value == NULL) {
                ret = X3F_NO_MEMORY;
                goto done;
            }

            memset(tbl->entries[i].name, 0, sizeof(uint16_t) * proplen);

            valinbytes = valbytes = sizeof(uint16_t) * vallen;
            x3f_utf16_to_utf8(tbl->entries[i].value, &valbytes,
                    value, &valinbytes);
        } else {
            tbl->entries[i].value = NULL;
        }

        X3F_TRACE("Name: %s Value: %s", tbl->entries[i].name,
            tbl->entries[i].value);
    }

    X3F_TRACE("WARNING: property reading is incomplete");

    fp->prop_tables[prop_id] = tbl;

    ret = X3F_SUCCESS;
done:
    if (propbuf) free(propbuf);
    if (buf) free(buf);

    if ( (x3f_unlock(fp)) < 0 ) {
        X3F_TRACE("Failed to unlock file lock. Something is wrong.");
    }

    return ret;
}

static X3F_STATUS x3f_read_image_section(struct x3f_file *fp,
                                         struct x3f_directory_entry *dirent)
{
    X3F_STATUS ret = X3F_SUCCESS;
    uint8_t image_header[X3F_IMAG_HEADER_LEN];
    unsigned type, format, columns, rows, row_bytes, version;
    size_t count = 0;
    unsigned i = -1;

    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(dirent);

    if ((ret = x3f_lock(fp)) < 0) {
        return ret;
    }

    if ((ret = x3f_fseek(fp, dirent->offset, X3F_SEEK_SET)) < 0) {
        goto done;
    }

    if ((ret = x3f_fread(fp, X3F_IMAG_HEADER_LEN, 1, image_header, &count)) < 0)
    {
        goto done;
    }

    if (*((uint32_t*)image_header) != X3F_IMAGE_SEC) {
        X3F_TRACE("Section marked as an IMAG or IMA2 section is not what it seems");
        ret = X3F_NOT_X3F_FILE;
        goto done;
    }

    type = X3F_WORD_AT(image_header, X3F_IMAG_HEADER_TYPE);
    format = X3F_WORD_AT(image_header, X3F_IMAG_DATA_FORMAT);
    columns = X3F_WORD_AT(image_header, X3F_IMAG_COLUMNS);
    rows = X3F_WORD_AT(image_header, X3F_IMAG_ROWS);
    row_bytes = X3F_WORD_AT(image_header, X3F_IMAG_ROW_BYTES);
    version = X3F_WORD_AT(image_header, X3F_IMAG_HEADER_VERSION);

    X3F_TRACE("Image section: ");
    X3F_TRACE("type = %u, format = %u, cols = %u, rows = %u, row_bytes = %u",
        type, format, columns, rows, row_bytes);
    X3F_TRACE("offset = %08x, version = %u.%u",
        dirent->offset + X3F_IMAG_HEADER_LEN, (unsigned)(version >> 16),
        (unsigned)(version & 0xffff));

    if ( (ret = x3f_find_first_free_image(fp, &i)) < 0 ) {
        goto done;
    }

    fp->images[i] = (struct x3f_image*)malloc(sizeof(struct x3f_image));
    memset(fp->images[i], 0, sizeof(struct x3f_image));

    if (fp->images[i] == NULL) {
        ret = X3F_NO_MEMORY;
        goto done;
    }

    fp->images[i]->ver_major = version >> 16;
    fp->images[i]->ver_minor = version & 0xffff;
    fp->images[i]->type = type;
    fp->images[i]->format = format;
    fp->images[i]->cols = columns;
    fp->images[i]->rows = rows;
    fp->images[i]->row_bytes = row_bytes;
    fp->images[i]->image_offset = dirent->offset + X3F_IMAG_HEADER_LEN;

done:
    if (x3f_unlock(fp) < 0) {
        X3F_TRACE("Failed to unlock file lock. Something is wrong.");
    }

    return ret;
}

static X3F_STATUS x3f_dir_count(struct x3f_file *fp,
                                uint32_t type,
                                unsigned *count)
{
    int i;

    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(count);

    *count = 0;

    for (i = 0; i < fp->dir.count; i++) {
        if (fp->dir.entries[i].type == type) *count += 1;
    }

    return X3F_SUCCESS;
}

static void x3f_setup_prop(struct x3f_file *fp)
{
    unsigned count = 0;

    if (fp->prop_tables != NULL) return;

    x3f_dir_count(fp, X3F_DIR_PROP, &count);

    X3F_TRACE("found %u property tables", count);

    if (count == 0) return;

    fp->prop_tables = (struct x3f_prop_table**)malloc(sizeof(struct x3f_prop_table*) *
        count);

    if (!fp->prop_tables) return;

    memset(fp->prop_tables, 0, sizeof(struct x3f_prop_table*) * count);
    fp->prop_table_count = count;
}

static void x3f_setup_images(struct x3f_file *fp)
{
    unsigned icount = 0, count = 0;

    if (fp->images != NULL) {
        return;
    }

    x3f_dir_count(fp, X3F_DIR_IMAG, &count);
    x3f_dir_count(fp, X3F_DIR_IMA2, &icount);

    count += icount;

    X3F_TRACE("%u image sections", count);

    fp->images = (struct x3f_image**)malloc(sizeof(struct x3f_image*) * count);

    if (!fp->images) {
        X3F_TRACE("Failed to allocate data!");
        return;
    }

    memset(fp->images, 0, sizeof(struct x3f_image*) * count);
    fp->image_count = count;
}

X3F_STATUS x3f_cleanup_all_sections(struct x3f_file *fp)
{
    int i = 0;
    if (fp->prop_tables) {
        for (i = 0; i < fp->prop_table_count; i++) {
            if (fp->prop_tables[i] != NULL)
                x3f_destroy_prop_record(fp->prop_tables[i]);
        }
        free(fp->prop_tables);
        fp->prop_tables = NULL;
    }

    if (fp->images) {
        for (i = 0; i < fp->image_count; i++) {
            if (fp->images[i] != NULL)
                x3f_destroy_image(fp->images[i]);
        }
        free(fp->images);
        fp->images = NULL;
    }

	if (fp->camf) {
		x3f_free_camf(fp->camf);
	}

    return X3F_SUCCESS;
}

X3F_STATUS x3f_read_section(struct x3f_file *fp,
                            int dir_ent)
{
    X3F_STATUS ret = X3F_SUCCESS;

    X3F_ASSERT_ARG(fp);

    if (fp->dir.entries == NULL) {
        X3F_TRACE("Something is busted - a file with no directory?!");
        return X3F_NO_MEMORY; /* Something is horrendously busted */
    }

    X3F_ASSERT(dir_ent >= 0);
    X3F_ASSERT(dir_ent < fp->dir.count);

    switch (fp->dir.entries[dir_ent].type) {
    case X3F_DIR_IMAG:
        X3F_TRACE("dirtype: IMAG");
        x3f_setup_images(fp);
        ret = x3f_read_image_section(fp, &fp->dir.entries[dir_ent]);
        break;
    case X3F_DIR_CAMF:
        X3F_TRACE("dirtype: CAMF");
        ret = x3f_read_camf_metadata(fp, &fp->dir.entries[dir_ent]);
        break;
    case X3F_DIR_IMA2:
        X3F_TRACE("dirtype: IMA2");
        x3f_setup_images(fp);
        ret = x3f_read_image_section(fp, &fp->dir.entries[dir_ent]);
        break;
    case X3F_DIR_PROP:
        X3F_TRACE("dirtype: PROP");
        x3f_setup_prop(fp);
        ret = x3f_read_prop_section(fp, &fp->dir.entries[dir_ent]);
        break;
    default:
        X3F_TRACE("Unknown dirtype: %08x",
            fp->dir.entries[dir_ent].type);
    }

    return ret;
}

