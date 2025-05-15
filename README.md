The project is about image stegonography. The main goal is write the binary data of the file into least significant bits (LSBs) of the images - effectively making it not visible to the naked eye.

The project is written in pure C, with data about file headers, and structure coming from my own research (Wikipedia, Google, lots of hexdumps).

For now the working option is to embed your file in a random-generated .bmp image.

Next: Embed files in chosen .bmp files

Future: Embed files in chosen or randomly-generated .bmp .jpg .png files
