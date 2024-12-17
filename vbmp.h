#ifndef VBMP_H
#define VBMP_H

#include <stdio.h>
#include <stdint.h>

//---------------------------------------CONSTANTS--------------------------------------

enum RETURNVALUES
{
	SUCCESS = 0,
	FAILURE = -1
};

extern const uint32_t VBMP_BMP_HEADER_SIZE;
extern const uint32_t VBMP_DIB_HEADER_SIZE;
extern const uint32_t VBMP_PIXEL_ARRAY_OFFSET;
extern const uint16_t VBMP_BITS_PER_PIXEL;


//---------------------------------------FUNCTIONS--------------------------------------

//-----------------SIZE CALCULATION---------------------
int estimate_size(FILE* hidden_file, uint32_t* hidden_file_size, uint16_t* rows, uint16_t* columns);
uint32_t bmp_size(uint16_t rows, uint16_t columns);
uint32_t pixel_array_size(uint16_t rows, uint16_t columns);
uint32_t row_size(uint16_t columns);
uint32_t bare_row_size(uint16_t columns);
uint32_t padding_bytes(uint16_t columns);


//-----------------FILE CREATION------------------------

int embed_file_in_bmp(const char* const input_file_name, const char* const output_file_name);
int create_bmp_file(FILE* hidden_file, const char* const output_file_name, uint32_t hidden_file_size, uint16_t rows, uint16_t columns);

int create_bmp_header(FILE* file, uint32_t file_size);
int create_dib_header(FILE* file, uint16_t pixel_image_width, uint16_t pixel_image_height);
int create_pixel_array(FILE* file, uint32_t hidden_file_size, uint16_t rows, uint16_t columns, FILE* hidden_file);

int embed_size(FILE* hidden_file, uint32_t hidden_file_size);
int alter_lsb(FILE* hidden_file, uint8_t* BGR);


//-----------------FILE EXTRACTION--------------------

int extract_from_bmp(const char* const input_file_name, const char* const output_file_name);
int extract_data(FILE* composite_file, uint16_t rows, uint16_t columns, FILE* extracted_file);


//-----------------FILE APPEND------------------------

#endif // VBMP_H