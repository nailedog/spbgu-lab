#include "bmp.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: Invalid arguments\n");
        fprintf(stderr, "Usage: %s input.bmp output.bmp\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    BMPImage image;
    BMPError error = bmp_read(input_file, &image);

    if (error != BMP_OK) {
        fprintf(stderr, "Error: %s\n", bmp_error_string(error));
        return 1;
    }

    if (image.dib_header.bits_per_pixel == 8) {
        bmp_invert_palette(&image);
    } else if (image.dib_header.bits_per_pixel == 24) {
        bmp_invert_pixels(&image);
    } else {
        fprintf(stderr, "Error: Unsupported bit depth\n");
        bmp_free(&image);
        return 1;
    }

    error = bmp_write(output_file, &image);
    if (error != BMP_OK) {
        fprintf(stderr, "Error: %s\n", bmp_error_string(error));
        bmp_free(&image);
        return 1;
    }

    bmp_free(&image);
    return 0;
}
