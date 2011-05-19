#ifndef __INCLUDE_X3F_HUFF_H__
#define __INCLUDE_X3F_HUFF_H__

#include <x3f.h>
#include <x3f_priv.h>

#include <stdint.h>
#include <stdlib.h>

struct x3f_huff_leaf {
    struct x3f_huff_leaf *branch[2];
    uint32_t leaf;
};

struct x3f_huff_mode_info {
    uint32_t predictor[4]; /* Starting points for huffman decoding */
    uint32_t plane_size[3];

    size_t start_off;

    struct x3f_huff_leaf *root;
};

X3F_STATUS x3f_huff_append_node(struct x3f_huff_leaf *root,
                                unsigned size,
                                unsigned value,
                                unsigned entrynum);

struct x3f_huff_leaf *x3f_new_huff_node();

void print_huffman_tree(struct x3f_huff_leaf *t, int length, uint32_t code);

/* A little helper bit iterator to assist with traversing buffers o'
 * bits
 */
struct biterator {
    uint8_t cached;

    uint8_t *buf_ptr;
    size_t buf_off; /* offset in buffer, in bytes */
    size_t bit_off; /* offset in current byte */
    size_t buf_max; /* maximum bytes in the buffer */
};

int x3f_huff_get_value(struct x3f_huff_leaf *root,
                       struct biterator *bit);

static inline int x3f_biterator_advance(struct biterator *bit)
{
    if (bit->bit_off == 8) {
        if (bit->buf_max == bit->buf_off + 1) {
            X3F_TRACE("reached the end of biterator. This is bad.");
            return -1;
        }
        bit->cached = *(++bit->buf_ptr);
        bit->bit_off = 0;
        bit->buf_off++;
    }

    return (bit->cached >> (8 - bit->bit_off++ - 1)) & 0x1;
}

struct biterator *x3f_new_biterator(uint8_t *buffer,
                                    size_t byte_size);

X3F_STATUS x3f_quantized_huff_decode(struct x3f_huff_leaf *root,
                                     unsigned predictor,
                                     uint8_t *encoded,
                                     size_t encoded_size,
                                     uint16_t *decoded,
                                     unsigned rows,
                                     unsigned cols);

#endif /* __INCLUDE_X3F_HUFF_H__ */

