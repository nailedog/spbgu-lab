#pragma once

#include <stdint.h>
#include <stdio.h>

#define BMP_HEADER_SIZE 14
#define DIB_HEADER_SIZE 40
#define BMP_SIGNATURE 0x4D42

typedef struct __attribute__((packed)) {
    uint16_t signature;
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t data_offset;
} BMPHeader;

typedef struct __attribute__((packed)) {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} DIBHeader;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
} RGBQuad;

typedef struct {
    BMPHeader bmp_header;
    DIBHeader dib_header;
    RGBQuad* palette;
    uint8_t* pixel_data;
    int32_t row_size;
    int is_bottom_up;
} BMPImage;

// Error codes
typedef enum {
    BMP_OK = 0,
    BMP_ERROR_FILE_OPEN = 1,
    BMP_ERROR_INVALID_SIGNATURE,
    BMP_ERROR_INVALID_HEADER,
    BMP_ERROR_UNSUPPORTED_FORMAT,
    BMP_ERROR_MEMORY,
    BMP_ERROR_FILE_READ,
    BMP_ERROR_FILE_WRITE,
    BMP_ERROR_INVALID_DIMENSIONS,
    BMP_ERROR_DATA_MISMATCH
} BMPError;

BMPError bmp_read(const char* filename, BMPImage* image);
BMPError bmp_write(const char* filename, const BMPImage* image);
void bmp_free(BMPImage* image);
BMPError bmp_validate(const BMPImage* image);
void bmp_invert_palette(BMPImage* image);
void bmp_invert_pixels(BMPImage* image);
int bmp_compare_pixels(const BMPImage* img1, const BMPImage* img2, int* diff_x, int* diff_y, int max_diffs);
const char* bmp_error_string(BMPError error);
