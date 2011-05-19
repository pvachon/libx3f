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
 * Simple wrapper for C standard I/O functions. Eventually this should be
 * replaced with simple buffering I/O functions that directly call
 * low-level function.
 */
#include <x3f_priv.h>
#include <stdio.h>
#include <stdlib.h>

X3F_STATUS x3f_fopen(struct x3f_file *fp,
                     const char *filename,
                     const char *mode)
{
    FILE *fpt;
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(filename);

    fpt = fopen(filename, "r");

    if (fpt == NULL) {
        return X3F_BAD_FILENAME;
    }

    fp->fp = fpt;

    return X3F_SUCCESS;
}

X3F_STATUS x3f_fclose(struct x3f_file *fp)
{
    X3F_ASSERT_ARG(fp);

    fclose((FILE *)fp->fp);

    return X3F_SUCCESS;
}

X3F_STATUS x3f_fread(struct x3f_file *fp,
                     size_t size,
                     size_t count,
                     void *buf,
                     size_t *count_read)
{
    int countr = 0;

    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(buf);

    X3F_ASSERT(fp->fp != NULL);

    countr = fread(buf, size, count, (FILE *)fp->fp);

    if (countr < 0) {
        return X3F_CANT_SEEK;
    }

    if (count_read) {
        *count_read = countr;
    }

    return X3F_SUCCESS;
}

X3F_STATUS x3f_fseek(struct x3f_file *fp,
                     off_t offset,
                     int whence)
{
    int res = 0;
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT(whence == X3F_SEEK_SET ||
               whence == X3F_SEEK_CUR ||
               whence == X3F_SEEK_END);
    X3F_ASSERT(fp->fp != NULL)

    res = fseek((FILE *)fp->fp, offset, whence);

    if (res < 0) {
        return X3F_CANT_SEEK;
    }

    return X3F_SUCCESS;
}

X3F_STATUS x3f_ftell(struct x3f_file *fp, size_t *off)
{
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(off);

    if (fp->fp == NULL) return X3F_BAD_ARG;

    *off = ftell(fp->fp);

    return X3F_SUCCESS;
}

X3F_STATUS x3f_lock(struct x3f_file *fp)
{
    return X3F_SUCCESS;
}

X3F_STATUS x3f_unlock(struct x3f_file *fp)
{
    return X3F_SUCCESS;
}


