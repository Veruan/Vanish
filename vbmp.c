#include "vbmp.h"

#include <time.h>
#include <math.h>

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

int estimate_size(FILE* hidden_file, uint32_t* hidden_file_size, uint16_t* rows, uint16_t* columns)
{
	if (fseek(hidden_file, 0, SEEK_END) != 0)
		goto ERROR_OCCURED;

	long int byte_size = ftell(hidden_file);
	if (byte_size > UINT32_MAX)
	{
		perror("Error - file too large\n");
		goto ERROR_OCCURED;
	}

	*hidden_file_size = byte_size;
	// convert bytes to bits
	long int bit_size = 8 * byte_size;

	// given that for each pixel we can write 3bits we need ceiling(size/3) pixels so +1 to be sure
	long int pixels_needed = (bit_size / 3) + 1;
	uint16_t side = (int)ceil(sqrt(pixels_needed));

	*rows = side;
	*columns = side;

	// set file pointer back to 0
	if (fseek(hidden_file, 0, SEEK_SET) != 0)
		goto ERROR_OCCURED;

	return SUCCESS;

ERROR_OCCURED:
	return FAILURE;
}


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

int embed_file_in_bmp(const char* const input_file_name, const char* const output_file_name)
{
	FILE* hidden_file = fopen(input_file_name, "rb");
	if (!hidden_file)
	{
		perror("Error - embeding bmp failed\n opening files");
		goto ERROR_OCCURED;
	}

	uint32_t hidden_file_size;
	uint16_t rows, columns;
	if (estimate_size(hidden_file, &hidden_file_size, &rows, &columns) == FAILURE)
	{
		perror("Error - embeding bmp failed\n estimating size");
		goto ERROR_OCCURED;
	}

	if (create_bmp_file(hidden_file, output_file_name, hidden_file_size, rows, columns))
	{
		perror("Error - embeding bmp failed\n creating bmp");
		goto ERROR_OCCURED;
	}

	fclose(hidden_file);

	return SUCCESS;

ERROR_OCCURED:
	if (hidden_file)
		fclose(hidden_file);

	return FAILURE;
}


int create_bmp_file(FILE* hidden_file, const char* const output_file_name, uint32_t hidden_file_size, uint16_t rows, uint16_t columns)
{
	FILE* file = fopen(output_file_name, "wb");
	if (!file)
	{
		perror("Error - creating bmp failed\n opening files");
		goto ERROR_OCCURED;
	}

	if (create_bmp_header(file, bmp_size(rows, columns)) == FAILURE)
	{
		perror("Error - creating bmp file failed\n bmp header");
		goto ERROR_OCCURED;
	}

	if (create_dib_header(file, columns, rows) == FAILURE)
	{
		perror("Error - creating bmp file failed\n dib header");
		goto ERROR_OCCURED;
	}

	if (create_pixel_array(file, hidden_file_size, rows, columns, hidden_file) == FAILURE)
	{
		perror("Error - creating bmp file failed\n pixel array");
		goto ERROR_OCCURED;
	}

	fclose(file);

	return SUCCESS;

ERROR_OCCURED:
	if (file)
		fclose(file);

	return FAILURE;
}


int create_bmp_header(FILE* file, uint32_t file_size)
{
	// BM magic bytes (2 * 1 byte)
	const char BM[] = { 'B', 'M'};
	if (fwrite(BM, sizeof(BM), 1, file) < 1)
		goto ERROR_OCCURED;

	// file size (4 bytes)
	if (fwrite(&file_size, sizeof(file_size), 1, file) < 1)
		goto ERROR_OCCURED;

	// reserved bytes (2 * 2bytes)
	const uint16_t reserved1= 0, reserved2 = 0;
	if (fwrite(&reserved1, sizeof(reserved1), 1, file) < 1)
		goto ERROR_OCCURED;
	if (fwrite(&reserved2, sizeof(reserved1), 1, file) < 1)
		goto ERROR_OCCURED;

	// pixel array offset (4 bytes)
	if (fwrite(&VBMP_PIXEL_ARRAY_OFFSET, sizeof(VBMP_PIXEL_ARRAY_OFFSET), 1, file) < 1)
		goto ERROR_OCCURED;

	return SUCCESS;

ERROR_OCCURED:
	return FAILURE;
}


int create_dib_header(FILE* file, uint16_t pixel_image_width, uint16_t pixel_image_height)
{
	// DIB Header Size (4 bytes)
	if (fwrite(&VBMP_DIB_HEADER_SIZE, sizeof(VBMP_DIB_HEADER_SIZE), 1, file) < 1)
		goto ERROR_OCCURED;

	// image width (2 bytes)
	if (fwrite(&pixel_image_width, sizeof(pixel_image_width), 1, file) < 1)
		goto ERROR_OCCURED;

	// image height (2 bytes)
	if (fwrite(&pixel_image_height, sizeof(pixel_image_height), 1, file) < 1)
		goto ERROR_OCCURED;

	// color planes, must be 1 (2 bytes)
	uint16_t color_planes = 1;
	if (fwrite(&color_planes, sizeof(color_planes), 1, file) < 1)
		goto ERROR_OCCURED;

	// BPP (2 bytes)
	uint16_t bits_per_pixel = VBMP_BITS_PER_PIXEL;
	if (fwrite(&bits_per_pixel, sizeof(bits_per_pixel), 1, file) < 1)
		goto ERROR_OCCURED;

	return SUCCESS;

ERROR_OCCURED:
	return FAILURE;
}



int create_pixel_array(FILE* file, uint32_t hidden_file_size, uint16_t rows, uint16_t columns, FILE* hidden_file)
{
	srand(time(NULL));
	uint8_t padding[] = { 0, 0, 0 };
	int padding_byte_count = padding_bytes(columns);

	//embed_size(hidden_file, hidden_file_size);

	for (uint16_t i= 0; i < rows; i++)
	{
		for (uint16_t j = 0; j < columns; j++)
		{
			uint8_t BGR[3] = { 0 };
			BGR[0] = rand() % 256;
			BGR[1] = rand() % 256;
			BGR[2] = rand() % 256;
			
			if (alter_lsb(hidden_file, BGR) == FAILURE)
			{
				perror("Error - altering lsb\n");
				goto ERROR_OCCURED;
			}

			if (fwrite(BGR, 3, 1, file) < 1)
				goto ERROR_OCCURED;
		}

		if (fwrite(&padding, 1, padding_byte_count, file) < padding_byte_count)
			goto ERROR_OCCURED;
	}

	return SUCCESS;

ERROR_OCCURED:
	return FAILURE;
}


int embed_size(FILE* hidden_file, uint32_t hidden_file_size)
{
	// we need 32 bits to embed a dword, and we can store 3 bits per pixel so 11 iterations, last is garbage
	for (int i = 0; i < 10; i++)
	{
		uint8_t current_bits = ((hidden_file_size >> (3* i)) & 1);

		uint8_t BGR[3] = { 0 };
		BGR[0] = rand() % 256;
		BGR[1] = rand() % 256;
		BGR[2] = rand() % 256;
	}

	uint8_t BGR[3] = { 0 };
	BGR[0] = rand() % 256;
	BGR[1] = rand() % 256;
	BGR[2] = rand() % 256;

	return 0;
}


int alter_lsb(FILE* hidden_file, uint8_t* BGR)
{
	// pointer to the currently processed byte in the file, and pointer to a bit to read
	static long int file_pointer = 0;
	static uint8_t byte_pointer = 0;

	if (fseek(hidden_file, file_pointer, SEEK_SET) != 0)
		goto ERROR_OCCURED;

	// kinda doubles the operation? to do smth with that later?
	uint8_t byte = fgetc(hidden_file);
	if (byte == EOF)
		return SUCCESS;

	uint8_t bits[3] = { 0 };
	for (int i = 0; i < 3; i++)
	{
		bits[i] = (byte >> ((7 - byte_pointer++)) & 1);
		if (byte_pointer > 7)
		{
			// move the byte and file pointer to the next byte, and read new content
			byte_pointer = 0;
			if (fseek(hidden_file, ++file_pointer, SEEK_SET) != 0)
				goto ERROR_OCCURED;

			byte = fgetc(hidden_file);
			if (byte == EOF)
				return SUCCESS;
		}
	}

	// Alter BGR lsb's according to bit value
	for(int i = 0; i < 3; i++)
		BGR[i] = bits[i] == 0 ? ((BGR[i] >> 1) << 1) : (BGR[i] | 1);

	return SUCCESS;

ERROR_OCCURED:
	return FAILURE;
}


//----------------------------------FILE EXTRACTION----------------------------------------

int extract_from_bmp(const char* const input_file_name, const char* const output_file_name)
{
	FILE *composite_file = fopen(input_file_name, "rb"), *extracted_file = fopen(output_file_name, "wb");
	if (!composite_file || !extracted_file)
	{
		perror("Error - extracting from bmp failed\n opening files");
		goto ERROR_OCCURED;
	}

	// BMP Header's last argument is dword offset to pixel array
	if (fseek(composite_file, VBMP_BMP_HEADER_SIZE - 4, SEEK_SET) != 0)
	{
		perror("Error - extracting from bmp failed\n fetching pixel array offset");
		goto ERROR_OCCURED;
	}
	uint32_t pixel_array_offset;
	if (fread(&pixel_array_offset, 4, 1, composite_file) != 1)
	{
		perror("Error - extracting from bmp failed\n fetching pixel array offset");
		goto ERROR_OCCURED;
	}

	// DIB Header's second argument (1st is 4 bytes) is always image width, than image height
	if (fseek(composite_file, VBMP_BMP_HEADER_SIZE + 4, SEEK_SET) != 0)
	{
		perror("Error - creating bmp failed\n fetching rows and cols");
		goto ERROR_OCCURED;
	}
	uint16_t rows, columns;
	uint16_t dimensions[2];
	if (fread(dimensions, 2, 2, composite_file) != 2)
	{
		perror("Error - creating BMP failed while fetching rows and cols");
		goto ERROR_OCCURED;
	}
	rows = dimensions[0];
	columns = dimensions[1];

	// move the file pointer to pixel array where the data is stored in lsb's
	if (fseek(composite_file, pixel_array_offset, SEEK_SET) != 0)
	{
		perror("Error - creating bmp failed\n opening files");
		goto ERROR_OCCURED;
	}

	if(extract_data(composite_file, rows, columns, extracted_file) == FAILURE)
		goto ERROR_OCCURED;

	fclose(composite_file);
	fclose(extracted_file);

	return 0;


ERROR_OCCURED:
	if (composite_file)
		fclose(composite_file);
	if (extracted_file)
		fclose(extracted_file);
	return FAILURE;
}


int extract_data(FILE* composite_file, uint16_t rows, uint16_t columns, FILE* extracted_file)
{
	uint8_t bit_counter = 0, padding = padding_bytes(columns);
	char byte = 0;

	for (uint16_t i = 0; i < rows; i++)
	{
		for (uint16_t j = 0; j < columns; j++)
		{
			// there are 3 bytes of color to a pixel (RGB spectre)
			for(uint8_t color_spectre = 0; color_spectre < 3; color_spectre++)
			{
				uint8_t color = fgetc(composite_file);
				if (color == EOF)
					goto ERROR_OCCURED;

				// shift left to make space for new bit, and or it with LSB of color byte
				byte = (byte << 1) | (color & 1);

				if (++bit_counter == 8)
				{
					fputc(byte, extracted_file);
					bit_counter = 0;
				}
			}
		}

		fseek(composite_file, padding, SEEK_CUR);
	}

	return SUCCESS;

ERROR_OCCURED:
	return FAILURE;
}