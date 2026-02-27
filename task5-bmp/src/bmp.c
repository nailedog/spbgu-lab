#include "bmp.h"
#include <stdlib.h>
#include <string.h>

const char* bmp_error_string(BMPError error) {
    switch (error) {
        case BMP_OK: return "Success";
        case BMP_ERROR_FILE_OPEN: return "Cannot open file";
        case BMP_ERROR_INVALID_SIGNATURE: return "Invalid BMP signature";
        case BMP_ERROR_INVALID_HEADER: return "Invalid header";
        case BMP_ERROR_UNSUPPORTED_FORMAT: return "Unsupported format";
        case BMP_ERROR_MEMORY: return "Memory allocation failed";
        case BMP_ERROR_FILE_READ: return "File read error";
        case BMP_ERROR_FILE_WRITE: return "File write error";
        case BMP_ERROR_INVALID_DIMENSIONS: return "Invalid dimensions";
        case BMP_ERROR_DATA_MISMATCH: return "Data size mismatch";
        default: return "Unknown error";
    }
}

static int calculate_row_size(int width, int bits_per_pixel) {
    return ((width * bits_per_pixel + 31) / 32) * 4;
}

BMPError bmp_validate(const BMPImage* image) {
    if (image->bmp_header.signature != BMP_SIGNATURE) {
        return BMP_ERROR_INVALID_SIGNATURE;
    }

    if (image->dib_header.header_size != DIB_HEADER_SIZE) {
        return BMP_ERROR_INVALID_HEADER;
    }

    if (image->dib_header.compression != 0) {
        return BMP_ERROR_UNSUPPORTED_FORMAT;
    }

    if (image->dib_header.bits_per_pixel != 8 && image->dib_header.bits_per_pixel != 24) {
        return BMP_ERROR_UNSUPPORTED_FORMAT;
    }

    if (image->dib_header.width <= 0) {
        return BMP_ERROR_INVALID_DIMENSIONS;
    }

    int32_t height = image->dib_header.height;
    if (height == 0) {
        return BMP_ERROR_INVALID_DIMENSIONS;
    }

    int32_t abs_height = height < 0 ? -height : height;
    int32_t row_size = calculate_row_size(image->dib_header.width, image->dib_header.bits_per_pixel);
    uint32_t expected_image_size = row_size * abs_height;

    uint32_t expected_offset = BMP_HEADER_SIZE + DIB_HEADER_SIZE;
    if (image->dib_header.bits_per_pixel == 8) {
        expected_offset += 256 * sizeof(RGBQuad);
    }

    if (image->bmp_header.data_offset < expected_offset) {
        return BMP_ERROR_INVALID_HEADER;
    }

    uint32_t expected_file_size = image->bmp_header.data_offset + expected_image_size;
    if (image->bmp_header.file_size < expected_file_size) {
        return BMP_ERROR_DATA_MISMATCH;
    }

    return BMP_OK;
}

BMPError bmp_read(const char* filename, BMPImage* image) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return BMP_ERROR_FILE_OPEN;
    }

    memset(image, 0, sizeof(BMPImage));

    if (fread(&image->bmp_header, sizeof(BMPHeader), 1, file) != 1) {
        fclose(file);
        return BMP_ERROR_FILE_READ;
    }

    if (fread(&image->dib_header, sizeof(DIBHeader), 1, file) != 1) {
        fclose(file);
        return BMP_ERROR_FILE_READ;
    }

    BMPError validation = bmp_validate(image);
    if (validation != BMP_OK) {
        fclose(file);
        return validation;
    }

    int32_t height = image->dib_header.height;
    int32_t abs_height = height < 0 ? -height : height;
    image->is_bottom_up = (height > 0);

    if (image->dib_header.bits_per_pixel == 8) {
        image->palette = (RGBQuad*)malloc(256 * sizeof(RGBQuad));
        if (!image->palette) {
            fclose(file);
            return BMP_ERROR_MEMORY;
        }

        if (fread(image->palette, sizeof(RGBQuad), 256, file) != 256) {
            free(image->palette);
            fclose(file);
            return BMP_ERROR_FILE_READ;
        }
    }

    image->row_size = calculate_row_size(image->dib_header.width, image->dib_header.bits_per_pixel);

    uint32_t data_size = image->row_size * abs_height;
    image->pixel_data = (uint8_t*)malloc(data_size);
    if (!image->pixel_data) {
        if (image->palette) free(image->palette);
        fclose(file);
        return BMP_ERROR_MEMORY;
    }

    if (fseek(file, image->bmp_header.data_offset, SEEK_SET) != 0) {
        bmp_free(image);
        fclose(file);
        return BMP_ERROR_FILE_READ;
    }

    if (fread(image->pixel_data, 1, data_size, file) != data_size) {
        bmp_free(image);
        fclose(file);
        return BMP_ERROR_FILE_READ;
    }

    fclose(file);
    return BMP_OK;
}

BMPError bmp_write(const char* filename, const BMPImage* image) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        return BMP_ERROR_FILE_OPEN;
    }

    if (fwrite(&image->bmp_header, sizeof(BMPHeader), 1, file) != 1) {
        fclose(file);
        return BMP_ERROR_FILE_WRITE;
    }

    if (fwrite(&image->dib_header, sizeof(DIBHeader), 1, file) != 1) {
        fclose(file);
        return BMP_ERROR_FILE_WRITE;
    }

    if (image->dib_header.bits_per_pixel == 8) {
        if (fwrite(image->palette, sizeof(RGBQuad), 256, file) != 256) {
            fclose(file);
            return BMP_ERROR_FILE_WRITE;
        }
    }

    int32_t height = image->dib_header.height;
    int32_t abs_height = height < 0 ? -height : height;
    uint32_t data_size = image->row_size * abs_height;

    if (fwrite(image->pixel_data, 1, data_size, file) != data_size) {
        fclose(file);
        return BMP_ERROR_FILE_WRITE;
    }

    fclose(file);
    return BMP_OK;
}

void bmp_free(BMPImage* image) {
    if (image->palette) {
        free(image->palette);
        image->palette = NULL;
    }
    if (image->pixel_data) {
        free(image->pixel_data);
        image->pixel_data = NULL;
    }
}

void bmp_invert_palette(BMPImage* image) {
    if (image->dib_header.bits_per_pixel != 8 || !image->palette) {
        return;
    }

    for (int i = 0; i < 256; i++) {
        image->palette[i].red = 255 - image->palette[i].red;
        image->palette[i].green = 255 - image->palette[i].green;
        image->palette[i].blue = 255 - image->palette[i].blue;
    }
}

void bmp_invert_pixels(BMPImage* image) {
    if (image->dib_header.bits_per_pixel != 24) {
        return;
    }

    int32_t height = image->dib_header.height;
    int32_t abs_height = height < 0 ? -height : height;

    for (int32_t y = 0; y < abs_height; y++) {
        uint8_t* row = image->pixel_data + y * image->row_size;
        for (int32_t x = 0; x < image->dib_header.width; x++) {
            int pixel_offset = x * 3;
            row[pixel_offset] = 255 - row[pixel_offset];         // Blue
            row[pixel_offset + 1] = 255 - row[pixel_offset + 1]; // Green
            row[pixel_offset + 2] = 255 - row[pixel_offset + 2]; // Red
        }
    }
}

int bmp_compare_pixels(const BMPImage* img1, const BMPImage* img2, int* diff_x, int* diff_y, int max_diffs) {
    if (img1->dib_header.width != img2->dib_header.width) {
        return -1;
    }

    int32_t height1 = img1->dib_header.height;
    int32_t height2 = img2->dib_header.height;
    int32_t abs_height1 = height1 < 0 ? -height1 : height1;
    int32_t abs_height2 = height2 < 0 ? -height2 : height2;

    if (abs_height1 != abs_height2) {
        return -1;
    }

    if (img1->dib_header.bits_per_pixel != img2->dib_header.bits_per_pixel) {
        return -1;
    }

    int diff_count = 0;
    int32_t width = img1->dib_header.width;
    int32_t abs_height = abs_height1;

    if (img1->dib_header.bits_per_pixel == 8) {
        for (int32_t y = 0; y < abs_height && diff_count < max_diffs; y++) {
            uint8_t* row1 = img1->pixel_data + y * img1->row_size;
            uint8_t* row2 = img2->pixel_data + y * img2->row_size;

            for (int32_t x = 0; x < width && diff_count < max_diffs; x++) {
                uint8_t index1 = row1[x];
                uint8_t index2 = row2[x];

                if (index1 != index2) {
                    RGBQuad color1 = img1->palette[index1];
                    RGBQuad color2 = img2->palette[index2];

                    if (color1.red != color2.red ||
                        color1.green != color2.green ||
                        color1.blue != color2.blue) {
                        diff_x[diff_count] = x;
                        diff_y[diff_count] = img1->is_bottom_up ? y : (abs_height - 1 - y);
                        diff_count++;
                    }
                }
            }
        }
    } else if (img1->dib_header.bits_per_pixel == 24) {
        for (int32_t y = 0; y < abs_height && diff_count < max_diffs; y++) {
            uint8_t* row1 = img1->pixel_data + y * img1->row_size;
            uint8_t* row2 = img2->pixel_data + y * img2->row_size;

            for (int32_t x = 0; x < width && diff_count < max_diffs; x++) {
                int offset = x * 3;
                if (row1[offset] != row2[offset] ||
                    row1[offset + 1] != row2[offset + 1] ||
                    row1[offset + 2] != row2[offset + 2]) {
                    diff_x[diff_count] = x;
                    diff_y[diff_count] = img1->is_bottom_up ? y : (abs_height - 1 - y);
                    diff_count++;
                }
            }
        }
    }

    return diff_count;
}
