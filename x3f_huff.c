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
 * Generic Huffman encode/decode functions.
 * These are used for building and traversing Huffman coding trees. The actual
 * use of the data from this is context-dependent.
 */
#include <x3f.h>
#include <x3f_priv.h>
#include <x3f_huff.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct biterator *x3f_new_biterator(uint8_t *buffer,
                                    size_t byte_size)
{
    struct biterator *bit = NULL;

    bit = (struct biterator *)malloc(sizeof(struct biterator));

    if (bit == NULL) return NULL;

    bit->cached = *buffer;
    bit->buf_ptr = buffer;
    bit->buf_off = 0;
    bit->bit_off = 0;
    bit->buf_max = byte_size;

    return bit;
}


/* Really naive Huffman coding */
struct x3f_huff_leaf *x3f_new_huff_node()
{
    struct x3f_huff_leaf *branch = NULL;

    branch = (struct x3f_huff_leaf*)malloc(sizeof(struct x3f_huff_leaf));

    if (branch != NULL) {
        memset(branch, 0, sizeof(struct x3f_huff_leaf));
    }

    branch->leaf = 0xfffffffful;

    return branch;
}

X3F_STATUS x3f_huff_append_node(struct x3f_huff_leaf *root,
                                unsigned size,
                                unsigned value,
                                unsigned entrynum)
{
    unsigned code = value >> (8 - size);
    int i;

    struct x3f_huff_leaf *branch = root;

    if (size == 0) return X3F_SUCCESS;

    for (i = 0; i < size; i++) {
        int dir = (code >> (size - i - 1)) & 1;

        if (branch->branch[dir] == NULL) {
            branch->branch[dir] = x3f_new_huff_node();
        }

        branch = branch->branch[dir];
    }

    branch->leaf = entrynum;

    return X3F_SUCCESS;
}

int x3f_huff_get_value(struct x3f_huff_leaf *root,
                       struct biterator *bit)
{
    struct x3f_huff_leaf *node = root;
    int val;

    while (node->branch[0] != NULL || node->branch[1] != NULL) {
        int bit_val = x3f_biterator_advance(bit);
        if (bit_val == -1) {
            printf ("failed to get bit\n");
            return -33939;
        }

        node = node->branch[bit_val];

        if (node == NULL) {
            X3F_TRACE("Busted - got an unexpected bit");
            return -33939;
        }
    }

    val = node->leaf;

    if (val != 0) {
        uint8_t first = x3f_biterator_advance(bit);
        int i;
        int out = 0;

        out = first;

        for (i = 1; i < val; i++)
            out = (out << 1) | x3f_biterator_advance(bit);

        if (!first) {
            out -= (1 << val) - 1;
        }

        val = out;
    }

    return val;
}

X3F_STATUS x3f_huff_decode(struct x3f_huff_leaf *root,
                           uint8_t *encoded,
                           size_t encoded_size,
                           uint8_t *decoded,
                           size_t decoded_size)
{
    int out_byte = 0;
    struct biterator *iter = NULL;
    X3F_ASSERT_ARG(root);
    X3F_ASSERT_ARG(encoded);
    X3F_ASSERT_ARG(decoded);

    iter = x3f_new_biterator(encoded, encoded_size);

    while (out_byte != decoded_size) {
        decoded[out_byte] = x3f_huff_get_value(root, iter);
        out_byte++;
    }

    return X3F_SUCCESS;
}

X3F_STATUS x3f_quantized_huff_decode(struct x3f_huff_leaf *root,
                                     unsigned predictor,
                                     uint8_t *encoded,
                                     size_t encoded_size,
                                     uint16_t *decoded,
                                     unsigned rows,
                                     unsigned cols)
{
    size_t decoded_size = rows * cols;
    unsigned row, col;
    size_t cur = 0;
    struct biterator *iter;
    int32_t row_beg[2][2] = { { predictor, predictor },
                              { predictor, predictor } };

    X3F_ASSERT_ARG(root);
    X3F_ASSERT_ARG(encoded);
    X3F_ASSERT_ARG(decoded);

    if (decoded_size < encoded_size) {
        return X3F_NO_MEMORY;
    }

    iter = x3f_new_biterator(encoded, encoded_size);

    for (row = 0; row < rows; row++) {
        int32_t val[2];
        for (col = 0; col < cols; col++) {
            int32_t old = col < 2 ? row_beg[row&1][col&1] :
                val[col&1];
            int32_t res = x3f_huff_get_value(root, iter);

            if (res == -33939) {
                printf("Failed at %d, %d\n", col, row);
                res = 0;
            }

            old += res;
            val[col&1] = old;

            if (col < 2) row_beg[row&1][col&1] = val[col&1];
            decoded[cur++] = (uint16_t)(((val[col & 1] >> 8) & 0xff) |
                ( (val[col & 1] & 0xff) << 8));
        }
    }

    return X3F_SUCCESS;
}

X3F_STATUS x3f_decode_camf_type4(struct x3f_huff_leaf *root,
                                 unsigned predictor,
                                 uint8_t *encoded,
                                 size_t encoded_size,
                                 uint8_t *decoded,
                                 unsigned rows,
                                 unsigned cols)
{
    unsigned row, col;
    struct biterator *iter;
    int32_t row_beg[2][2] = { { predictor, predictor },
                              { predictor, predictor } };
    int flip = 0;

    X3F_ASSERT_ARG(root);
    X3F_ASSERT_ARG(encoded);
    X3F_ASSERT_ARG(decoded);

    iter = x3f_new_biterator(encoded, encoded_size);

    for (row = 0; row < rows; row++) {
        int32_t val[2];
        for (col = 0; col < cols; col++) {
            int32_t old = col < 2 ? row_beg[row&1][col&1] :
                val[col&1];
            int32_t res = x3f_huff_get_value(root, iter);

            if (res == -33939) {
                X3F_TRACE("Failed at %d, %d", col, row);
                res = 0;
            }

            old += res;
            val[col&1] = old;

            if (col < 2) row_beg[row&1][col&1] = val[col&1];

            if (!flip) {
                *decoded++ = (old >> 4) & 0xff;
                *decoded = (old << 4) & 0xf0;
            } else {
                *decoded++ |= (old >> 8) & 0x0f;
                *decoded++ = old & 0xff;
            }

            flip = !flip;
        }
    }

    return X3F_SUCCESS;
}

X3F_STATUS x3f_release_huff_tree(struct x3f_huff_leaf *root)
{
	return X3F_SUCCESS;
}

