#include "vbmp.h"

int main(int argc, char* argv[])
{
	embed_file_in_bmp("test", "idkEmbed");
	extract_from_bmp("idkEmbed", NULL);
	return 0;
}