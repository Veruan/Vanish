#include "vbmp.h"

int main(int argc, char* argv[])
{
	embed_file_in_bmp("idk", "idkEmbed");
	extract_from_bmp("idkEmbed", "idk2");
	return 0;
}