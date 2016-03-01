#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <cstring>
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include "IMAGE.h"


//Open a BMP and find out what type it is
bool IMAGE::LoadBMP(const char * filename)
{
	FILE * file;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;

	//Open file for reading
	file = fopen(filename, "rb");
	if (!file)
	{
		printf("Unable to open %s\n", filename);
		return false;
	}

	//read the file header
	fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);

	//Check this is a bitmap
	if (fileHeader.bfType != bitmapID)
	{
		fclose(file);
		printf("%s is not a legal .bmp\n", filename);
		return false;
	}

	//Read in the information header
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

	//close the file
	fclose(file);

	//Find out the bpp and go to correct loading function
	if (infoHeader.biBitCount == 8)
		return Load8BitBMP(filename);
	if (infoHeader.biBitCount == 24)
		return Load24BitBMP(filename);

	printf("%s has an unsupported bpp\n", filename);

	return false;
}

//Load an compressed true color TGA (24 or 32 bit)
bool IMAGE::LoadCompressedTrueColorTGA(const char * filename)
{
	unsigned char compTGAHeader[12] = { 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned char TGAcompare[12];
	unsigned char header[6];

	printf("Loading %s in LoadCompressedTGA\n", filename);

	FILE * file = fopen(filename, "rb");

	if (!file)
	{
		printf("Unable to open %s\n", filename);
		return false;
	}

	//read the "compare" header
	fread(TGAcompare, 1, sizeof(TGAcompare), file);
	if (memcmp(compTGAHeader, TGAcompare, sizeof(compTGAHeader)) != 0)
	{
		printf("%s is not an uncompressed true color TGA\n", filename);
		return false;
	}

	//read in header
	fread(header, 1, sizeof(header), file);

	//Save data into class member variables
	width = header[1] * 256 + header[0];
	height = header[3] * 256 + header[2];
	bpp = header[4];

	if (width <= 0 || height <= 0 || (bpp != 24 && bpp != 32))
	{
		fclose(file);
		printf("%s's height or width is less than zero, or the TGA is not 24/32 bpp\n", filename);
		return false;
	}

	//Set format
	if (bpp == 24)
		format = GL_RGB;
	if (bpp == 32)
		format = GL_RGBA;

	//allocate space for temporary data storage
	GLubyte * tempData = new GLubyte[width*height*bpp / 8];
	if (!tempData)
	{
		printf("unable to allocate memory for temporary image data\n");
		fclose(file);
		return false;
	}

	//read in the image data
	int pixelCount = width*height;
	int currentPixel = 0;
	int currentByte = 0;
	unsigned char * colorBuffer = new unsigned char[bpp / 8];

	do
	{
		unsigned char chunkHeader = 0;

		if (fread(&chunkHeader, sizeof(unsigned char), 1, file) == 0)
		{
			printf("could not read RLE chunk header\n");
			fclose(file);
			return false;
		}

		if (chunkHeader<128)		//read raw colour values
		{
			++chunkHeader;

			for (int counter = 0; counter<chunkHeader; ++counter)
			{
				if (fread(colorBuffer, 1, bpp / 8, file) != bpp / 8)
				{
					printf("unable to read image data\n");
					fclose(file);
					return false;
				}

				//transfer pixel colour to data(swapping R and B values)
				tempData[currentByte + 0] = colorBuffer[2];
				tempData[currentByte + 1] = colorBuffer[1];
				tempData[currentByte + 2] = colorBuffer[0];

				if (bpp / 8 == 4)
					tempData[currentByte + 3] = colorBuffer[3];

				currentByte += bpp / 8;
				++currentPixel;

				if (currentPixel>pixelCount)
				{
					printf("too many pixels read\n");
					fclose(file);
					return false;
				}
			}
		}
		else	//chunkHeader>=128
		{
			chunkHeader -= 127;

			if (fread(colorBuffer, 1, bpp / 8, file) != bpp / 8)
			{
				printf("unable to read image data\n");
				fclose(file);
				return false;
			}

			for (int counter = 0; counter<chunkHeader; ++counter)
			{
				//transfer pixel colour to data(swapping R and B values)
				tempData[currentByte + 0] = colorBuffer[2];
				tempData[currentByte + 1] = colorBuffer[1];
				tempData[currentByte + 2] = colorBuffer[0];

				if (bpp / 8 == 4)
					tempData[currentByte + 3] = colorBuffer[3];

				currentByte += bpp / 8;
				++currentPixel;

				if (currentPixel>pixelCount)
				{
					printf("too many pixels read\n");
					fclose(file);
					return false;
				}
			}
		}
	} while (currentPixel<pixelCount);

	//Calculate the stride in bytes for each row (allow for 4-byte padding)
	stride = CalculateStride();

	//Allocate space for the image data
	data = new GLubyte[stride*height];
	if (!data)
	{
		printf("Unable to allocate data for %s of size %d x %d\n", filename,
			stride, height);
		return false;
	}

	//copy in the data a line at a time, hence making it 32-byte aligned
	for (unsigned int row = 0; row<height; ++row)
	{
		memcpy(&data[(row*stride)], &tempData[row*width*bpp / 8], width*bpp / 8);
	}

	//delete the temporary data
	if (tempData)
		delete[] tempData;
	tempData = NULL;

	if (colorBuffer)
		delete[] colorBuffer;
	colorBuffer = NULL;
	printf("Loaded %s Correctly!\n", filename);
	return true;
}

//Open a TGA and find out what type it is
bool IMAGE::LoadTGA(const char * filename)
{
	unsigned char uncompTGAHeader[12] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned char compTGAHeader[12] = { 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned char uncomp8Bit24TGAHeader[12] = { 0, 1, 1, 0, 0, 0, 1, 24, 0, 0, 0, 0 };
	unsigned char uncomp8Bit32TGAHeader[12] = { 0, 1, 1, 0, 0, 0, 1, 32, 0, 0, 0, 0 };

	unsigned char TGAcompare[12];

	FILE * file;

	//Open file for reading
	file = fopen(filename, "rb");
	if (!file)
	{
		printf("Unable to open %s\n", filename);
		return false;
	}

	//read the file header
	fread(TGAcompare, sizeof(TGAcompare), 1, file);

	//close the file
	fclose(file);

	//Find out the type and go to correct loading function
	if (memcmp(uncompTGAHeader, TGAcompare, sizeof(uncompTGAHeader)) == 0)
		return LoadUncompressedTrueColorTGA(filename);

	if (memcmp(compTGAHeader, TGAcompare, sizeof(compTGAHeader)) == 0)
		return LoadCompressedTrueColorTGA(filename);

	if (memcmp(uncomp8Bit24TGAHeader, TGAcompare, sizeof(uncomp8Bit24TGAHeader)) == 0 ||
		memcmp(uncomp8Bit32TGAHeader, TGAcompare, sizeof(uncomp8Bit32TGAHeader)) == 0)
	{
		return LoadUncompressed8BitTGA(filename);
	}


	printf("%s has an unsupported bpp\n", filename);

	return false;
}

//Load an uncompressed true color TGA (24 or 32 bit)
bool IMAGE::LoadUncompressedTrueColorTGA(const char * filename)
{
	unsigned char uncompTGAHeader[12] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned char TGAcompare[12];
	unsigned char header[6];

	printf("Loading %s in LoadUncompressedTGA\n", filename);

	FILE * file = fopen(filename, "rb");

	if (!file)
	{
		printf("Unable to open %s\n", filename);
		return false;
	}

	//read the "compare" header
	fread(TGAcompare, 1, sizeof(TGAcompare), file);
	if (memcmp(uncompTGAHeader, TGAcompare, sizeof(uncompTGAHeader)) != 0)
	{
		printf("%s is not an uncompressed true color TGA\n", filename);
		return false;
	}

	//read in header
	fread(header, 1, sizeof(header), file);

	//Save data into class member variables
	width = header[1] * 256 + header[0];
	height = header[3] * 256 + header[2];
	bpp = header[4];

	if (width <= 0 || height <= 0 || (bpp != 24 && bpp != 32))
	{
		fclose(file);
		printf("%s's height or width is less than zero, or the TGA is not 24/32 bpp\n", filename);
		return false;
	}

	//Set format
	if (bpp == 24)
		format = GL_RGB;
	if (bpp == 32)
		format = GL_RGBA;

	//Calculate the stride in bytes for each row (allow for 4-byte padding)
	stride = CalculateStride();

	//Allocate space for the image data
	data = new GLubyte[stride*height];
	if (!data)
	{
		fclose(file);
		printf("Unable to allocate data for %s of size %d x %d\n", filename,
			stride, height);
		return false;
	}

	//read in the data a line at a time, and save it into the array,
	//hence making it 32-byte aligned
	for (unsigned int row = 0; row<height; ++row)
	{
		fread(&data[row*stride], 1, width*(bpp / 8), file);
	}

	fclose(file);

	//Data is in BGR format
	//swap b and r
	for (unsigned int row = 0; row<height; ++row)
	{
		for (unsigned int i = 0; i<width; ++i)
		{
			//Repeated XOR to swap bytes 0 and 2
			data[(row*stride) + i*(bpp / 8)] ^= data[(row*stride) + i*(bpp / 8) + 2] ^=
				data[(row*stride) + i*(bpp / 8)] ^= data[(row*stride) + i*(bpp / 8) + 2];
		}
	}

	printf("Loaded %s Correctly!\n", filename);
	return true;
}

//Load an image from a file
bool IMAGE::Load(const char * filename)
{
	//Clear the member variables if already used
	bpp = 0;
	width = height = 0;
	stride = 0;
	format = 0;

	if (data)
		delete[]  data;
	data = NULL;

	paletted = false;
	paletteBpp = 0;
	paletteFormat = 0;
	if (palette)
		delete[] palette;
	palette = NULL;

	//Call the correct loading function based on the filename
	int filenameLength = strlen(filename);

	if (strncmp(filename + filenameLength - 3, "BMP", 3) == 0 ||
		strncmp(filename + filenameLength - 3, "bmp", 3) == 0)
		return LoadBMP(filename);

	if (strncmp(filename + filenameLength - 3, "TGA", 3) == 0 ||
		strncmp(filename + filenameLength - 3, "tga", 3) == 0)
		return LoadTGA(filename);

	printf("%s does not end with \"bmp\" or \"tga\"\n", filename);
	return false;
}

//Load an image from a file
bool IMAGE::LoadAlpha(const IMAGE & alphaImage)
{
	//Cannot load alpha channel into a paletted image
	if (paletted)
	{
		printf("Cannot load alpha channel into a paletted image\n");
		return false;
	}

	//Check alpha image is paletted
	if (!alphaImage.paletted)
	{
		printf("Alpha image is non-paletted\n");
		return false;
	}

	//Check the images are the same size
	if (alphaImage.width != width || alphaImage.height != height)
	{
		printf("Alpha image is not the same size as color image\n");
		return false;
	}

	//Create space for new data
	int newStride = CalculateStride(32);

	GLubyte * tempData = new GLubyte[newStride*height];
	if (!tempData)
	{
		printf("Unable to allocate memory for temporary image data\n");
		return false;
	}

	//Fill in the temporary data
	for (unsigned int row = 0; row<height; ++row)
	{
		for (unsigned int i = 0; i<width; ++i)
		{
			tempData[(row*newStride) + i * 4 + 0] = data[(row*stride) + i*bpp / 8 + 0];
			tempData[(row*newStride) + i * 4 + 1] = data[(row*stride) + i*bpp / 8 + 1];
			tempData[(row*newStride) + i * 4 + 2] = data[(row*stride) + i*bpp / 8 + 2];

			tempData[(row*newStride) + i * 4 + 3] = alphaImage.data[(row*alphaImage.stride) + i];
		}
	}

	//delete the old data and swap pointers
	if (data)
		delete[] data;
	data = tempData;

	//update bpp and format
	bpp = 32;
	format = GL_RGBA;

	return true;
}

//Load an uncompressed 8 Bit TGA (24 or 32 bit)
bool IMAGE::LoadUncompressed8BitTGA(const char * filename)
{
	unsigned char uncomp8Bit24TGAHeader[12] = { 0, 1, 1, 0, 0, 0, 1, 24, 0, 0, 0, 0 };
	unsigned char uncomp8Bit32TGAHeader[12] = { 0, 1, 1, 0, 0, 0, 1, 32, 0, 0, 0, 0 };
	unsigned char TGAcompare[12];
	unsigned char header[6];

	printf("Loading %s in LoadUncompressed8BitTGA\n", filename);

	FILE * file = fopen(filename, "rb");

	if (!file)
	{
		printf("Unable to open %s\n", filename);
		return false;
	}

	//read the "compare" header
	fread(TGAcompare, 1, sizeof(TGAcompare), file);
	if (memcmp(uncomp8Bit24TGAHeader, TGAcompare, sizeof(uncomp8Bit24TGAHeader)) != 0 &&
		memcmp(uncomp8Bit32TGAHeader, TGAcompare, sizeof(uncomp8Bit32TGAHeader)) != 0)
	{
		printf("%s is not an uncompressed 8Bit TGA\n", filename);
		return false;
	}

	//read in header
	fread(header, 1, sizeof(header), file);

	//Save data into class member variables
	width = header[1] * 256 + header[0];
	height = header[3] * 256 + header[2];
	bpp = header[4];

	if (width <= 0 || height <= 0 || bpp != 8)
	{
		fclose(file);
		printf("%s's height or width is less than zero, or the TGA is not 8 bpp\n", filename);
		return false;
	}

	paletted = true;
	paletteBpp = TGAcompare[7];
	if (paletteBpp == 24)
		paletteFormat = GL_RGB;
	if (paletteBpp == 32)
		paletteFormat = GL_RGBA;

	//make space for palette
	palette = new GLubyte[256 * paletteBpp / 8];
	if (!palette)
	{
		fclose(file);
		printf("unable to allocate memory for palette\n");
		return false;
	}

	//load in the palette
	fread(palette, 256 * paletteBpp / 8, 1, file);

	//Palette is in BGR format
	//swap b and r
	for (int i = 0; i<256; ++i)
	{
		//Repeated XOR to swap bytes 0 and 2
		palette[i*(paletteBpp / 8)] ^= palette[i*(paletteBpp / 8) + 2] ^=
			palette[i*(paletteBpp / 8)] ^= palette[i*(paletteBpp / 8) + 2];
	}

	//Set format
	format = GL_COLOR_INDEX;

	//Calculate the stride in bytes for each row (allow for 4-byte padding)
	stride = CalculateStride();

	//Allocate space for the image data
	data = new GLubyte[stride*height];
	if (!data)
	{
		fclose(file);
		printf("Unable to allocate data for %s of size %d x %d\n", filename,
			stride, height);
		return false;
	}

	//read in the data a line at a time, and save it into the array,
	//hence making it 32-byte aligned
	for (unsigned int row = 0; row<height; ++row)
	{
		fread(&data[row*stride], 1, width, file);
	}

	fclose(file);

	printf("Loaded %s Correctly!\n", filename);
	return true;
}

//Load an 8 bit .bmp file
bool IMAGE::Load8BitBMP(const char * filename)
{
	printf("Loading %s in Load8BitBMP()\n", filename);

	FILE * file;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;

	//Open file for reading
	file = fopen(filename, "rb");
	if (!file)
	{
		printf("Unable to open %s\n", filename);
		return false;
	}

	//read the file header
	fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);

	//Check this is a bitmap
	if (fileHeader.bfType != bitmapID)
	{
		fclose(file);
		printf("%s is not a legal .bmp\n", filename);
		return false;
	}

	//Read in the information header
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

	if (infoHeader.biBitCount != 8)
	{
		fclose(file);
		printf("%s is not an 8 bit .bmp\n", filename);
		return false;
	}

	//Set class variables
	bpp = 8;
	format = GL_COLOR_INDEX;
	width = infoHeader.biWidth;
	height = infoHeader.biHeight;

	paletted = true;
	paletteBpp = 24;
	paletteFormat = GL_RGB;

	//Calculate the stride in bytes for each row (allow for 4-byte padding)
	stride = CalculateStride();

	//Create space for a temporary palette
	GLubyte * tempPalette = new GLubyte[256 * 4];
	if (!tempPalette)
	{
		fclose(file);
		printf("Unable to allocate space for palette\n");
		return false;
	}

	//Load the palette
	fread(tempPalette, 256 * 4, 1, file);

	//Create space for palette (disregard needless 4th entry)
	palette = new GLubyte[256 * 3];
	if (!palette)
	{
		fclose(file);
		printf("Unable to allocate space for palette\n");
		return false;
	}

	//Copy the palette entries from tempPalette to palette
	//swap bytes 0 and 2 to go from BGR to RGB
	for (int i = 0; i<256; ++i)
	{
		palette[i * 3 + 0] = tempPalette[i * 4 + 2];
		palette[i * 3 + 1] = tempPalette[i * 4 + 1];
		palette[i * 3 + 2] = tempPalette[i * 4 + 0];
	}

	//Clear "tempPalette" data
	if (tempPalette)
		delete[] tempPalette;
	tempPalette = NULL;

	//Point "file" to the beginning of the data
	fseek(file, fileHeader.bfOffBits, SEEK_SET);

	//Allocate space for the image data
	data = new GLubyte[stride*height];
	if (!data)
	{
		fclose(file);
		printf("Unable to allocate data for %s of size %d x %d\n", filename,
			stride, height);
		return false;
	}

	//read in the data
	fread(data, 1, stride*height, file);

	fclose(file);

	printf("Loaded %s Correctly!\n", filename);
	return true;
}

//Load a 24 bit .bmp file
bool IMAGE::Load24BitBMP(const char * filename)
{
	printf("Loading %s in Load24BitBMP()\n", filename);

	FILE * file;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;

	//Open file for reading
	file = fopen(filename, "rb");
	if (!file)
	{
		printf("Unable to open %s\n", filename);
		return false;
	}

	//read the file header
	fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);

	//Check this is a bitmap
	if (fileHeader.bfType != bitmapID)
	{
		fclose(file);
		printf("%s is not a legal .bmp\n", filename);
		return false;
	}

	//Read in the information header
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

	if (infoHeader.biBitCount != 24)
	{
		fclose(file);
		printf("%s is not a 24 bit .bmp\n", filename);
		return false;
	}

	//Set class variables
	bpp = 24;
	format = GL_RGB;
	width = infoHeader.biWidth;
	height = infoHeader.biHeight;

	//Calculate the stride in bytes for each row (allow for 4-byte padding)
	stride = CalculateStride();

	//Point "file" to the beginning of the data
	fseek(file, fileHeader.bfOffBits, SEEK_SET);

	//Allocate space for the image data
	data = new GLubyte[stride*height];
	if (!data)
	{
		fclose(file);
		printf("Unable to allocate data for %s of size %d x %d\n", filename,
			stride, height);
		return false;
	}

	//read in the data
	fread(data, 1, stride*height, file);

	fclose(file);

	//Data is in BGR format
	//swap b and r
	for (unsigned int row = 0; row<height; ++row)
	{
		for (unsigned int i = 0; i<width; ++i)
		{
			//Repeated XOR to swap bytes 0 and 2
			data[(row*stride) + i * 3] ^= data[(row*stride) + i * 3 + 2] ^=
				data[(row*stride) + i * 3] ^= data[(row*stride) + i * 3 + 2];
		}
	}

	printf("Loaded %s Correctly!\n", filename);
	return true;
}

//Convert a paletted image into a non-paletted one
void IMAGE::ExpandPalette(void)
{
	//Do not expand non-paletted images, or those with no data
	if (!paletted || !data)
		return;

	//Calculate the stride of the expanded image
	unsigned int newStride = CalculateStride(paletteBpp);

	//Create space for the expanded image data
	GLubyte * newData = new GLubyte[newStride*height];
	if (!newData)
	{
		printf("Unable to create memory for expanded data\n");
		return;
	}

	//Loop through and fill in the unpaletted data
	for (unsigned int row = 0; row<height; ++row)
	{
		for (unsigned int i = 0; i<width; ++i)
		{
			unsigned int currentOldPixel = (row*stride) + i;
			unsigned int currentNewPixel = (row*newStride) + i*(paletteBpp / 8);
			GLubyte currentPaletteEntry = data[((row*stride) + i)];

			for (unsigned int j = 0; j<paletteBpp / 8; ++j)
			{
				newData[currentNewPixel + j] =
					palette[currentPaletteEntry*(paletteBpp / 8) + j];
			}
		}
	}

	//Update class member variables
	paletted = false;

	bpp = paletteBpp;
	format = paletteFormat;
	stride = newStride;

	if (data)
		delete[] data;
	data = newData;
}

//Calculate the number of bytes in an image row, including padding bytes
//default parameters (-1, -1). If a parameter is -1, use class varibles
unsigned int IMAGE::CalculateStride(unsigned int testBpp, unsigned int testWidth)
{
	//See if we should use the class' own variables
	if (testWidth == -1)
		testWidth = width;

	if (testBpp == -1)
		testBpp = bpp;

	//Calculate the number of bits per line
	unsigned int bitsPerLine = testWidth*testBpp;

	//Find how many to add on to make 32-bit aligned
	unsigned int bitsToAdd = 0;

	if ((bitsPerLine % 32) != 0)
		bitsToAdd = 32 - (bitsPerLine % 32);

	//return stride
	return (bitsPerLine + bitsToAdd) / 8;
}