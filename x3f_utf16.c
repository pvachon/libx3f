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

/* Helper functions for managing UTF-16 properties. This is a kludge
 * and is inelegant. However, unicode is a PITA as a programmer, IMO.
 */
#include <x3f_priv.h>

#include <iconv.h>

size_t x3f_utf16_strlen(const char *str)
{
    uint16_t *rstr = (uint16_t*)str;
    size_t count = 0;

    while (*rstr++ != 0) {
        count++;
    }

    return count;
}

size_t x3f_utf16_to_utf8(char *utf8,
                         size_t *out_buf_bytes,
                         char *utf16,
                         size_t *in_buf_bytes)
{
    iconv_t conv = iconv_open("UTF-8", "UTF-16");
    size_t count = 0;
    int ret;

    if (conv == (iconv_t)-1) {
        X3F_TRACE("Failed to construct transform from UTF-16 to UTF-8");
        return 0;
    }

    count = iconv(conv, &utf16, in_buf_bytes, &utf8, out_buf_bytes);

    ret = iconv_close(conv);

    if (ret == -1) {
        X3F_TRACE("Failed to tear down UTF-16 transform");
    }

    return count;
}


