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

int estimate_size(FILE* hidden_file, uint16_t* rows, uint16_t* columns)
{
	if (fseek(hidden_file, 0, SEEK_END) != 0)
		return -1;

	// convert bytes to bits
	long int size = 8 * ftell(hidden_file);

	// given that for each pixel we can write 3bits we need ceiling(size/3) pixels so +1 to be sure
	long int pixels_needed = (size / 3) + 1;
	uint16_t side = (int)ceil(sqrt(pixels_needed));

	*rows = side;
	*columns = side;

	// set file pointer back to 0
	if (fseek(hidden_file, 0, SEEK_SET) != 0)
		return -1;

	return 0;
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
		return -1;
	}

	uint16_t rows, columns;
	if (estimate_size(hidden_file, &rows, &columns) == -1)
	{
		perror("Error - embeding bmp failed\n estimating size");
		fclose(hidden_file);
		return -1;
	}

	if (create_bmp_file(hidden_file, output_file_name, rows, columns))
	{
		perror("Error - embeding bmp failed\n creating bmp");
		fclose(hidden_file);
		return -1;
	}

	fclose(hidden_file);

	return 0;
}


int extract_from_bmp(const char* const input_file_name, const char* const output_file_name)
{
	FILE* composite_file = fopen(input_file_name, "rb");
	if (!composite_file)
	{
		perror("Error - extracting from bmp failed\n opening files");
		return -1;
	}

	FILE* extracted_file = fopen(output_file_name, "wb");
	if (!extracted_file)
	{
		perror("Error - extracting from bmp failed\n opening files");
		fclose(composite_file);
		return -1;
	}

	// BMP Header's last argument is dword offset to pixel array
	if (fseek(composite_file, VBMP_BMP_HEADER_SIZE - 4, SEEK_SET) != 0)
	{
		perror("Error - extracting from bmp failed\n fetching pixel array offset");
		fclose(composite_file);
		fclose(extracted_file);
		return -1;
	}
	uint32_t pixel_array_offset;
	if (fread(&pixel_array_offset, 4, 1, composite_file) != 1)
	{
		perror("Error - extracting from bmp failed\n fetching pixel array offset");
		fclose(composite_file);
		fclose(extracted_file);
		return -1;
	}

	// DIB Header's second argument (1st is 4 bytes) is always image width, than image height
	if (fseek(composite_file, VBMP_BMP_HEADER_SIZE + 4, SEEK_SET) != 0)
	{
		perror("Error - creating bmp failed\n fetching rows and cols");
		fclose(composite_file);
		fclose(extracted_file);
		return -1;
	}
	uint16_t rows, columns;
	uint16_t dimensions[2];
	if (fread(dimensions, 2, 2, composite_file) != 2)
	{
		perror("Error - creating BMP failed while fetching rows and cols");
		fclose(composite_file);
		fclose(extracted_file);
		return -1;
	}
	rows = dimensions[0];
	columns = dimensions[1];

	if (fseek(composite_file, pixel_array_offset, SEEK_SET) != 0)
	{
		perror("Error - creating bmp failed\n opening files");
		fclose(composite_file);
		fclose(extracted_file);
		return -1;
	}

	extract_data(composite_file, rows, columns, extracted_file);

	fclose(composite_file);
	fclose(extracted_file);

	return 0;
}


int create_bmp_file(FILE* hidden_file, const char* const output_file_name, uint16_t rows, uint16_t columns)
{
	FILE* file = fopen(output_file_name, "wb");
	if (!file)
	{
		perror("Error - creating bmp failed\n opening files");
		return -1;
	}

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

	if (create_pixel_array(file, rows, columns, hidden_file) == -1)
	{
		perror("Error - creating bmp file failed\n pixel array");
		fclose(file);
		return -1;
	}

	fclose(file);

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



int create_pixel_array(FILE* file, uint16_t rows, uint16_t columns, FILE* hidden_file)
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
			
			if (alter_lsb(hidden_file, BGR) == -1)
			{
				perror("Error - altering lsb\n");
				return -1;
			}

			if (fwrite(BGR, 3, 1, file) < 1)
				return -1;
		}

		if (fwrite(&padding, 1, padding_byte_count, file) < padding_byte_count)
			return -1;
	}

	return 0;
}


int alter_lsb(FILE* hidden_file, uint8_t* BGR)
{
	// pointer to the currently processed byte in the file, and pointer to a bit to read
	static long int file_pointer = 0;
	static uint8_t byte_pointer = 0;

	if (fseek(hidden_file, file_pointer, SEEK_SET) != 0)
		return -1;

	// kinda doubles the operation? to do smth with that later?
	uint8_t byte = fgetc(hidden_file);
	if (byte == EOF)
		return 0;

	uint8_t bits[3];
	for (int i = 0; i < 3; i++)
	{
		bits[i] = (byte >> ((7 - byte_pointer++)) & 1);
		if (byte_pointer > 7)
		{
			// move the byte and file pointer to the next byte, and read new content
			byte_pointer = 0;
			if (fseek(hidden_file, ++file_pointer, SEEK_SET) != 0)
				return -1;

			byte = fgetc(hidden_file);
			if (byte == EOF)
				return 0;
		}
	}

	// Alter BGR lsb's according to bit value
	for(int i = 0; i < 3; i++)
		BGR[i] = bits[i] == 0 ? ((BGR[i] >> 1) << 1) : (BGR[i] | 1);

	return 0;
}


int extract_data(FILE* composite_file, uint16_t rows, uint16_t columns, FILE* extracted_file)
{
	uint8_t bit_counter = 0, padding = padding_bytes(columns);
	char byte = 0;

	for (uint16_t i = 0; i < rows; i++)
	{
		for (uint16_t j = 0; j < columns; j++)
		{
			// there are 3 bytes of color to a pixel
			for(uint8_t color_spectre = 0; color_spectre < 3; color_spectre++)
			{
				uint8_t color = fgetc(composite_file);
				if (color == EOF)
					return -1;

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

	return 0;
}