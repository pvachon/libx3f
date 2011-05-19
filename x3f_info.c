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
 * Information getters for the public libx3f API.
 */

#include <x3f.h>
#include <x3f_priv.h>

#include <string.h>

X3F_STATUS x3f_get_ver(struct x3f_file *fp, unsigned *major, unsigned *minor)
{
    X3F_ASSERT_ARG(fp);

    if (major) *major = fp->hdr.ver_major;
    if (minor) *minor = fp->hdr.ver_minor;

    return X3F_SUCCESS;
}

X3F_STATUS x3f_get_dims(struct x3f_file *fp, unsigned *columns, unsigned *rows,
                        unsigned *rotation)
{
    X3F_ASSERT_ARG(fp);

    if (columns)
        *columns = fp->hdr.columns;

    if (rows)
        *rows = fp->hdr.rows;

    if (rotation)
        *rotation = fp->hdr.rotation;

    return X3F_SUCCESS;
}

X3F_STATUS x3f_get_id(struct x3f_file *fp, unsigned char *id, unsigned *mark)
{
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(id);

    memcpy(id, fp->hdr.id, 16);

    if (mark) *mark = fp->hdr.mark;

    return X3F_SUCCESS;
}

X3F_STATUS x3f_get_white_balance(struct x3f_file *fp, char *white_balance)
{
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(white_balance);

    memcpy(white_balance, fp->hdr.white_balance, 32);

    return X3F_SUCCESS;
}

X3F_STATUS x3f_get_extended_attrib(struct x3f_file *fp, int num,
                                   unsigned char *type, unsigned *value)
{
    X3F_ASSERT_ARG(fp);
    X3F_ASSERT_ARG(type);
    X3F_ASSERT_ARG(value);

    X3F_ASSERT(num < 32);
    X3F_ASSERT(num >= 0);

    if (num >= 32 || num < 0) {
        return X3F_RANGE;
    }

    *type = fp->hdr.ext[num].type;
    *value = fp->hdr.ext[num].value;

    return X3F_SUCCESS;
}


