#ifndef VBMP_H
#define VBMP_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

//---------------------------------------CONSTANTS--------------------------------------

extern const uint32_t VBMP_BMP_HEADER_SIZE;
extern const uint32_t VBMP_DIB_HEADER_SIZE;
extern const uint32_t VBMP_PIXEL_ARRAY_OFFSET;
extern const uint16_t VBMP_BITS_PER_PIXEL;


//---------------------------------------FUNCTIONS--------------------------------------

//-----------------SIZE CALCULATION---------------------
uint32_t bmp_size(uint16_t rows, uint16_t columns);
uint32_t pixel_array_size(uint16_t rows, uint16_t columns);
uint32_t row_size(uint16_t columns);
uint32_t bare_row_size(uint16_t columns);
uint32_t padding_bytes(uint16_t columns);


//-----------------FILE CREATION------------------------
int create_bmp_file(const char* const file_name, uint16_t rows, uint16_t columns);

int create_bmp_header(FILE *file, uint32_t file_size);
int create_dib_header(FILE* file, uint16_t pixel_image_width, uint16_t pixel_image_height);
int create_pixel_array(FILE* file, uint16_t rows, uint16_t columns);

#endif