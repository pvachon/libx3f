#ifndef __INCLUDE_X3F_PRIV_H__
#define __INCLUDE_X3F_PRIV_H__

#include <x3f.h>
#include <x3f_metatree.h>

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

struct x3f_extended_data {
    uint8_t type;
    uint32_t value;
};

struct x3f_directory_entry {
    uint32_t offset;
    uint32_t length;
    uint32_t type;
    int record; /* internal record ID */
};

struct x3f_directory {
    unsigned count;
    unsigned version;
    struct x3f_directory_entry *entries;
};

struct x3f_header {
    uint16_t ver_major;
    uint16_t ver_minor;
    uint8_t id[16];
    unsigned mark;
    unsigned columns;
    unsigned rows;
    unsigned rotation;
    char white_balance[32];
    struct x3f_extended_data ext[32];
};

struct x3f_prop_table_entry {
    uint32_t name_offset;
    uint32_t val_offset;
    char *name;
    char *value;
};

struct x3f_prop_table {
    uint16_t ver_major;
    uint16_t ver_minor;
    unsigned count;
    unsigned format;
    unsigned length;
    struct x3f_prop_table_entry *entries;
};

struct x3f_image_mode;

struct x3f_image {
    uint16_t ver_major;
    uint16_t ver_minor;
    unsigned type;
    unsigned format;
    unsigned cols;
    unsigned rows;
    unsigned row_bytes;
    size_t image_offset; /* Offset to the image data in file */

    struct x3f_image_mode *mode; /* Image accessors */
    void *mode_info; /* Private pointer for reader */
};

struct x3f_huff_leaf;

/* helper used in reading all CMb* records */
struct x3f_cmb_header {
    union {
        char magic[4];
        uint32_t magic_w;
    };
    uint16_t ver_minor;
    uint16_t ver_major;
    uint32_t rec_length; /* total length of record */
    uint32_t unknown; /* unknown */
    uint32_t hdr_len; /* length of x3f_cmb_header including description string */
};

/* helper used in reading in CMbM records */
struct x3f_cmbm_header {
    uint32_t type; /* Type of the array (float, etc.) */
    uint32_t dimension; /* Dimension of the array */
    uint32_t data_off; /* Start of data from start of record */
};

/* Helper for dealing with CMbM record dimension descriptors */
struct x3f_cmbm_dim_info {
    uint32_t size; /* length of dimension, in elements */
    uint32_t desc_off; /* offset from start of rec. to description word */
    uint32_t stride; /* stride, in elements, to subsequent element of array */
};

struct x3f_array_record {
    uint32_t type;
    uint32_t num_dims;
    uint32_t *dim_lengths;
    uint32_t bytes;

    union {
        float *f32;
        uint32_t *u32;
        uint16_t *u16;
        void *hdl;
    };
};

struct x3f_camf {
    uint16_t ver_major;
    uint16_t ver_minor;

    uint32_t type;
    uint32_t raw_data_size;

    struct x3f_metatree *meta;

    /* For older data */
    uint32_t key;

    /* For Type 4 CAMF sections */
    uint32_t predictor;
    unsigned block_count;
    unsigned block_size;
    struct x3f_huff_leaf *huff_root;
};

struct x3f_file {
    char *filename;
    void *fp;
    struct x3f_header hdr;
    struct x3f_directory dir;
    unsigned dir_offset;

    unsigned prop_table_count;
    struct x3f_prop_table **prop_tables;

    unsigned image_count;
    struct x3f_image **images;

    struct x3f_camf *camf;
};


#ifdef _DEBUG
#define X3F_PRINT(x, ...) \
    printf(x, ##__VA_ARGS__)

#define X3F_TRACE(x, ...) \
    fprintf(stderr, "x3f: %s:%d " x "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define X3F_ERROR(x, d, ...) \
    X3F_TRACE("error (%d): " x, d, ##__VA_ARGS__)

#define X3F_ASSERT(c) \
    if (!(c)) { \
        X3F_TRACE("assertion failure: " #c " == false"); \
        assert(c); \
    }

#else
#define X3F_PRINT(...)
#define X3F_TRACE(...)
#define X3F_ERROR(...)
#define X3F_ASSERT(...)
#endif

#define X3F_ASSERT_ARG(x) \
    if (!(x)) { \
        X3F_TRACE("assertion failure: " #x " == false"); \
        return X3F_BAD_ARG; \
    }


X3F_STATUS x3f_fopen(struct x3f_file *fp,
                     const char *filename,
                     const char *mode);

X3F_STATUS x3f_fclose(struct x3f_file *fp);

X3F_STATUS x3f_fread(struct x3f_file *fp,
                     size_t size,
                     size_t count,
                     void *buf,
                     size_t *count_read);

X3F_STATUS x3f_ftell(struct x3f_file *fp, size_t *off);

/* Whence modes */
#define X3F_SEEK_SET    SEEK_SET
#define X3F_SEEK_CUR    SEEK_CUR
#define X3F_SEEK_END    SEEK_END

X3F_STATUS x3f_fseek(struct x3f_file *fp,
                     off_t offset,
                     int whence);


X3F_STATUS x3f_lock(struct x3f_file *fp);
X3F_STATUS x3f_unlock(struct x3f_file *fp);

/* Magical UTF-16 handling functions */
size_t x3f_utf16_to_utf8(char *utf8,
                         size_t *out_buf_bytes,
                         char *utf16,
                         size_t *in_buf_bytes);

size_t x3f_utf16_strlen(const char *str);

/* Macros for extracting various field types */
#define X3F_WORD_AT(buf, byte) \
    (*(uint32_t*)&((uint8_t*)(buf))[byte])

#define X3F_SHORT_AT(buf, byte) \
    (*(uint16_t*)&((uint8_t*)(buf))[byte])


/* X3F File attributes */
#define X3F_HEADER_LEN            40
#define X3F_FULL_HEADER          256

#define X3F_HEADER_FILEID          0
#define X3F_HEADER_VER             4
#define X3F_HEADER_ID              8
#define X3F_HEADER_MARK           24
#define X3F_HEADER_COLUMNS        28
#define X3F_HEADER_ROWS           32
#define X3F_HEADER_ROTATION       36
#define X3F_HEADER_WHITEBAL       40
#define X3F_HEADER_EXTENDED_TYPES 104
#define X3F_HEADER_EXTENDED_DATA  136

/* X3F PROP section header attributes */
#define X3F_PROP_HEADER_LEN        24

#define X3F_PROP_HEADER_ID          0
#define X3F_PROP_HEADER_VER         4
#define X3F_PROP_HEADER_COUNT       8
#define X3F_PROP_HEADER_FORMAT     12
#define X3F_PROP_HEADER_RESERVED   16
#define X3F_PROP_HEADER_LENGTH     20

#define X3F_PROP_ENTRY_SIZE         8
#define X3F_PROP_ENTRY_NAME         0
#define X3F_PROP_ENTRY_VALUE        4

/* X3F IMAG and IMA2 section header attributes */
#define X3F_IMAG_HEADER_LEN        28

#define X3F_IMAG_HEADER_ID          0
#define X3F_IMAG_HEADER_VERSION     4
#define X3F_IMAG_HEADER_TYPE        8
#define X3F_IMAG_DATA_FORMAT       12
#define X3F_IMAG_COLUMNS           16
#define X3F_IMAG_ROWS              20
#define X3F_IMAG_ROW_BYTES         24


X3F_STATUS x3f_read_section(struct x3f_file *fp,
                            int dir_ent);
X3F_STATUS x3f_cleanup_all_sections(struct x3f_file *fp);

X3F_STATUS x3f_read_camf_metadata(struct x3f_file *fp,
                                  struct x3f_directory_entry *dirent);

X3F_STATUS x3f_free_camf(struct x3f_camf *camf);

#endif /* __INCLUDE_X3F_PRIV_H__ */

