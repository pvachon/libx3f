#include <x3f.h>

#include <stdio.h>
#include <stdlib.h>

const char *progname = NULL;

const char *attribute_names[] = {
    "Unused",
    "Exposure adjust",
    "Contrast adjust",
    "Shadow adjust",
    "Highlight adjust",
    "Saturation adjust",
    "Sharpness adjust",
    "Color adjust red",
    "Color adjust green",
    "Color adjust blue",
    "X3 Fill Light adjust"
};

const char *get_attrib_name(int i)
{
    if (i > 10 || i < 0) {
        return "Invalid attribute";
    }

    return attribute_names[i];
}

void error(const char *str)
{
    printf("Error: ");
    printf("%s\n", str);
    printf("Usage: %s [filename]\n", progname);

    exit(-1);
}

void print_id(const char *id)
{
    int i;
    printf("\tID: ");
    for (i = 0; i < 16; i++) {
        printf("%02x", id[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    struct x3f_file *fp;
    X3F_STATUS ret;
    unsigned cols, rows, rotation, mark, maj, min;
    unsigned char id[16];
    char white_bal[32];
    int i, j;
    unsigned subimages;
    unsigned cp2_size;
    float cp2_buf[9];

    progname = argv[0];

    if (argc < 2) {
        error("no filename specified!\n");
    }

    x3f_initialize();

    ret = x3f_open(&fp,
                   argv[1],
                   "r");

    if (ret < 0) {
        printf("Failed to open file: %d\n",
            ret);
        error("Failed to open file");
    }

    printf("Filename: %s\n", argv[1]);
    x3f_get_ver(fp, &maj, &min);
    printf("\tX3F Version: %u.%u\n", maj, min);

    x3f_get_dims(fp, &cols, &rows, &rotation);
    printf("\tDimensions: %d x %d rotated %d degrees\n",
        cols, rows, rotation);

    x3f_get_id(fp, id, &mark);
    print_id(id);
    printf("\tMark: 0x%08x\n", mark);

    x3f_get_white_balance(fp, white_bal);
    printf("\tWhite balance: %s\n", white_bal);

    for (i = 0; i < 32; i++) {
        unsigned char type;
        float val;
        x3f_get_extended_attrib(fp, i, &type, (unsigned *)(&val));

        if (type != 0)
            printf("\t%s (%d): %f\n", get_attrib_name(type), (int)type, (float)val);
    }

    x3f_get_subimage_count(fp, &subimages);

    for (i = 0; i < subimages; i++) {
        X3F_STATUS stat;
        x3f_get_subimage_dims(fp, i, &rows, &cols);
        printf("\tSubimage %d: %u x %u\n", i + 1, cols, rows);
        stat = x3f_get_min_read_block(fp, i, &cols, &rows);
        if (stat == X3F_SUCCESS)
            printf("\t\tMin read block: %u x %u\n", cols, rows);
        else
            printf("\t\tSubimage is of an unsupported type.\n");
    }

    /* Get CP2_Matrix */
    if (x3f_get_array(fp, "CP2_Matrix", NULL, &cp2_size) != X3F_SUCCESS) {
        printf("Failed to get CP2_Matrix array!");
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

    printf("Cleaning up\n");

    x3f_close(fp);

    return 0;
}

