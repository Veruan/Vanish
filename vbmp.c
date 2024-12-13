#include "vbmp.h"


//---------------------------------------CONSTANTS----------------------------------------

// 2 + 4 + 2 + 2 + 4
// BM bytes, file size in bytes, reserved, reserved, offset of pixel array
const uint32_t VBMP_BMP_HEADER_SIZE = 14;

// I use BITMAPCOREHEADER OS21XBITMAPHEADER
const uint32_t VBMP_DIB_HEADER_SIZE = 12;

// OFFSET_TO_PIXEL_ARRAY = BMP_HEADER_SIZE + DIB_HEADER_SIZE;
const uint32_t VBMP_PIXEL_ARRAY_OFFSET = 26;

// 3 bytes ppixel
const uint16_t VBMP_BITS_PER_PIXEL = 24;


//----------------------------------SIZE CALCULATION-------------------------------------

uint32_t bmp_size(uint16_t rows, uint16_t columns)
{
	return VBMP_BMP_HEADER_SIZE + VBMP_DIB_HEADER_SIZE + pixel_array_size(rows, columns);
}


uint32_t pixel_array_size(uint16_t rows, uint16_t columns)
{
	return rows * row_size(columns);
}


uint32_t row_size(uint16_t columns)
{
	return bare_row_size(columns) + padding_bytes(columns);
}


uint32_t bare_row_size(uint16_t columns)
{
	return columns * (VBMP_BITS_PER_PIXEL / 8);
}


uint32_t padding_bytes(uint16_t columns)
{
	// if padding there is 0 padding needed there will be 4 useless bytes so %4
	return (4 - (bare_row_size(columns) % 4)) % 4;
}


//----------------------------------FILE CREATION--------------------------------------
int create_bmp_file(const char* const file_name, uint16_t rows, uint16_t columns)
{
	FILE* file = fopen(file_name, "wb");

	if (create_bmp_header(file, bmp_size(rows, columns)) == -1)
	{
		perror("Error - creating bmp file failed\n bmp header");
		fclose(file);
		return -1;
	}


	if (create_dib_header(file, columns, rows) == -1)
	{
		perror("Error - creating bmp file failed\n dib header");
		fclose(file);
		return -1;
	}


	if (create_pixel_array(file, rows, columns) == -1)
	{
		perror("Error - creating bmp file failed\n pixel array");
		fclose(file);
		return -1;
	}

	return 0;
}


int create_bmp_header(FILE* file, uint32_t file_size)
{
	// BM magic bytes (2 * 1 byte)
	const char BM[] = { 'B', 'M'};
	if (fwrite(BM, sizeof(BM), 1, file) < 1)
		return -1;

	// file size (4 bytes)
	if (fwrite(&file_size, sizeof(file_size), 1, file) < 1)
		return -1;

	// reserved bytes (2 * 2bytes)
	const uint16_t reserved1= 0, reserved2 = 0;
	if (fwrite(&reserved1, sizeof(reserved1), 1, file) < 1)
		return -1;
	if (fwrite(&reserved2, sizeof(reserved1), 1, file) < 1)
		return -1;

	// pixel array offset (4 bytes)
	if (fwrite(&VBMP_PIXEL_ARRAY_OFFSET, sizeof(VBMP_PIXEL_ARRAY_OFFSET), 1, file) < 1)
		return -1;

	return 0;
}



int create_dib_header(FILE* file, uint16_t pixel_image_width, uint16_t pixel_image_height)
{
	// DIB Header Size (4 bytes)
	if (fwrite(&VBMP_DIB_HEADER_SIZE, sizeof(VBMP_DIB_HEADER_SIZE), 1, file) < 1)
		return -1;

	// image width (2 bytes)
	if (fwrite(&pixel_image_width, sizeof(pixel_image_width), 1, file) < 1)
		return -1;

	// image height (2 bytes)
	if (fwrite(&pixel_image_height, sizeof(pixel_image_height), 1, file) < 1)
		return -1;

	// color planes, must be 1 (2 bytes)
	uint16_t color_planes = 1;
	if (fwrite(&color_planes, sizeof(color_planes), 1, file) < 1)
		return -1;

	// BPP (2 bytes)
	uint16_t bits_per_pixel = VBMP_BITS_PER_PIXEL;
	if (fwrite(&bits_per_pixel, sizeof(bits_per_pixel), 1, file) < 1)
		return -1;

	return 0;
}



int create_pixel_array(FILE* file, uint16_t rows, uint16_t columns)
{
	srand(time(NULL));
	uint8_t padding[] = { 0, 0, 0 };
	int padding_byte_count = padding_bytes(columns);

	for (uint16_t i= 0; i < rows; i++)
	{
		for (uint16_t j = 0; j < columns; j++)
		{
			uint8_t BGR[3];
			BGR[0] = rand() % 256;
			BGR[1] = rand() % 256;
			BGR[2] = rand() % 256;

			if (fwrite(BGR, 3, 1, file) < 1)
				return -1;
		}

		if (fwrite(&padding, 1, padding_byte_count, file) < padding_byte_count)
			return -1;
	}

	return 0;
}