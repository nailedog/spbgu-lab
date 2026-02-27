#include "bmp.h"
#include <stdio.h>
#include <string.h>

#define MAX_DIFFS 100

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: Invalid arguments\n");
        fprintf(stderr, "Usage: %s image1.bmp image2.bmp\n", argv[0]);
        return 1;
    }

    const char* file1 = argv[1];
    const char* file2 = argv[2];

    BMPImage img1, img2;

    BMPError error1 = bmp_read(file1, &img1);
    if (error1 != BMP_OK) {
        fprintf(stderr, "Error reading first image: %s\n", bmp_error_string(error1));
        return 1;
    }

    BMPError error2 = bmp_read(file2, &img2);
    if (error2 != BMP_OK) {
        fprintf(stderr, "Error reading second image: %s\n", bmp_error_string(error2));
        bmp_free(&img1);
        return 1;
    }

    if (img1.dib_header.width != img2.dib_header.width) {
        fprintf(stderr, "Error: Images have different widths\n");
        bmp_free(&img1);
        bmp_free(&img2);
        return 1;
    }

    int32_t height1 = img1.dib_header.height;
    int32_t height2 = img2.dib_header.height;
    int32_t abs_height1 = height1 < 0 ? -height1 : height1;
    int32_t abs_height2 = height2 < 0 ? -height2 : height2;

    if (abs_height1 != abs_height2) {
        fprintf(stderr, "Error: Images have different heights\n");
        bmp_free(&img1);
        bmp_free(&img2);
        return 1;
    }

    if (img1.dib_header.bits_per_pixel != img2.dib_header.bits_per_pixel) {
        fprintf(stderr, "Error: Images have different bit depths\n");
        bmp_free(&img1);
        bmp_free(&img2);
        return 1;
    }

    int diff_x[MAX_DIFFS];
    int diff_y[MAX_DIFFS];

    int diff_count = bmp_compare_pixels(&img1, &img2, diff_x, diff_y, MAX_DIFFS);

    if (diff_count < 0) {
        fprintf(stderr, "Error: Images are not comparable\n");
        bmp_free(&img1);
        bmp_free(&img2);
        return 1;
    }

    if (diff_count == 0) {
        printf("Images are same\n");
        bmp_free(&img1);
        bmp_free(&img2);
        return 0;
    }

    fprintf(stderr, "Next pixels are different:\n");
    for (int i = 0; i < diff_count; i++) {
        fprintf(stderr, "x%-6d y%-6d\n", diff_x[i], diff_y[i]);
    }

    bmp_free(&img1);
    bmp_free(&img2);
    return 2;
}
