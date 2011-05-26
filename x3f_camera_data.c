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

/* This file contains methods for manipulating the vendor-specific camera
 * metadata. This will suck all the data in as a blob, decrypt it, then
 * attempt to parse the data as a property list.
 */
#include <x3f.h>
#include <x3f_priv.h>
#include <x3f_huff.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _DEBUG
static void dump_16(uint8_t *bytes, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (i % 8 == 0 && !(i % 16 == 0)) {
            printf("-%02x", (unsigned int)bytes[i]);
        } else {
            printf(" %02x", (unsigned int)bytes[i]);
        }
    }
    printf("   |");
    for (i = 0; i < count; i++) {
        if (isprint(bytes[i])) printf("%c", bytes[i]);
        else printf(".");
    }
    printf("|\n");
}

static void dump_buf(uint8_t *bytes, size_t length)
{
    size_t i, off = 0;

    for (i = 0; i < length; i += 16) {
        off = i;
        printf("%08x: ", (unsigned)off);
        dump_16(&bytes[i], 16);
    }

    if (off != length) {
        printf("%08x: ", (unsigned)off + 16);
        dump_16(&bytes[off], (unsigned)(length - off));
    }
}
#endif

static X3F_STATUS x3f_old_camf_decrypt(struct x3f_camf *camf,
                                       uint8_t *data,
                                       size_t length)
{
    size_t i;
    unsigned key, val;
    X3F_ASSERT_ARG(camf);
    X3F_ASSERT_ARG(data);

    key = camf->key;

    for (i = 0; i < length; i++) {
        key = (key * 1597 + 51749) % 244944;
        val = key * (int64_t)301593171 >> 24;
        data[i] ^= ((((key << 8) - val) >> 1) + val) >> 17;
    }

#ifdef _DEBUG
    dump_buf(data, length);
#endif

    return X3F_SUCCESS;
}

#ifdef _DEBUG
static void x3f_dump_camf_data(uint8_t *data, size_t length)
{
    FILE *fp = fopen("camf.dat", "w+");

    if (fp == NULL) {
        X3F_TRACE("Unable to open camf.dat for dumping!\n");
        return;
    }

    fwrite(data, length, 1, fp);

    fclose(fp);
}
#endif

/* Yes, this is for real. Don't ask me why, I don't want to know. */
static X3F_STATUS x3f_type4_camf_decrypt(struct x3f_camf *camf,
                                         uint8_t *data,
                                         size_t length)
{
    unsigned size, val;
    size_t start = 0;
    int i = 0;
    X3F_STATUS ret = X3F_SUCCESS;
    uint8_t *decoded = NULL;
    uint32_t outsize;

    camf->huff_root = x3f_new_huff_node();

    /* Read Huffman Table entries from start of data */
    do {
        size = data[start++];
        val = data[start++];
        x3f_huff_append_node(camf->huff_root, size, val, i);
        i++;
    } while (size != 0);

    outsize = (camf->block_size * camf->block_count * 3)/2;
    decoded = (uint8_t*)calloc(1, outsize);

    if (decoded == NULL) {
        return X3F_NO_MEMORY;
    }

    X3F_TRACE("Starting decoding %zd bytes from start of buffer",
        start);
    X3F_TRACE("Input size: %u, output size = %u\n", camf->raw_data_size,
        outsize);

    if ( (ret = x3f_decode_camf_type4(camf->huff_root,
                                      camf->predictor,
                                      &data[start + 4],
                                      camf->raw_data_size,
                                      decoded,
                                      camf->block_size,
                                      camf->block_count) ) < 0 )
    {
        X3F_TRACE("Error while decoding type4 data!\n");
        goto done;
    }

#ifdef _DEBUG
    x3f_dump_camf_data(decoded, outsize);
#endif

done:
    free(decoded);
    return ret;
}

X3F_STATUS x3f_read_camf_metadata(struct x3f_file *fp,
                                  struct x3f_directory_entry *dirent)
{
    unsigned type, key;
    X3F_STATUS ret = X3F_SUCCESS;
    size_t count;
    uint8_t buf[28];
    uint8_t *data = NULL;

    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(dirent);

    if ( (ret = x3f_lock(fp)) < 0 ) {
        return ret;
    }

    if ( (ret = x3f_fseek(fp, dirent->offset, X3F_SEEK_SET)) < 0) {
        goto done;
    }

    if ( (ret = x3f_fread(fp, 28, 1, buf, &count)) < 0) {
        goto done;
    }

    fp->camf = (struct x3f_camf *)malloc(sizeof(struct x3f_camf));

    if (fp->camf == NULL) {
        ret = X3F_NO_MEMORY;
        goto done;
    }

    memset(fp->camf, 0, sizeof(struct x3f_camf));

    type = X3F_WORD_AT(buf, 8);
    key = X3F_WORD_AT(buf, 24);

    fp->camf->type = type;
    fp->camf->key = key;
    fp->camf->predictor = X3F_WORD_AT(buf, 16);
    fp->camf->block_count = X3F_WORD_AT(buf, 20);
    fp->camf->block_size = X3F_WORD_AT(buf, 24);
    fp->camf->raw_data_size = dirent->length - 28;

    X3F_TRACE("Found CAMF metadata type %u key %08x", type, key);

 #ifdef _DEBUG
    if (type == 4) {
        X3F_TRACE("Type 4 header: %u blocks of size %u, predictor = %08x",
            fp->camf->block_count, fp->camf->block_size,
            (unsigned int)fp->camf->predictor);
    }
 #endif

    data = (uint8_t*)malloc(dirent->length - 28);

    if ( (ret = x3f_fread(fp, dirent->length - 28, 1, data, &count)) < 0 ) {
        goto done;
    }

    switch (fp->camf->type)
    {
    case 2:
    case 3:
        x3f_old_camf_decrypt(fp->camf, data, dirent->length - 28);
        break;
    case 4:
    default:
        x3f_type4_camf_decrypt(fp->camf, data, dirent->length - 28);
        break;
    }

done:
    if (data) free(data);

    if ( (ret = x3f_unlock(fp)) < 0 ) {
        return X3F_NO_MEMORY;
    }
    return ret;
}

