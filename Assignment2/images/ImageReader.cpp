// ==========================================================================
// Image File Reading Support Functions
//  - requires the Magick++ development libraries: http://www.imagemagick.org
//  - or the FreeImage library: http://freeimage.sourceforge.net
//
// You may use this code (or not) however you see fit for your work.
//
// Set the #defines below to choose the image library you have installed on
// your system, that you will be using for your assignment. Then compile and
// link this source file with your project.
//
// Authors: Sonny Chan, Alex Brown
//          University of Calgary
// Date:    January-March 2016
// ==========================================================================

#include <string>
#include <iostream>

#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#include "ImageReader.h"

using namespace std;

// --------------------------------------------------------------------------
// Set these defines to choose which image library to use for loading image
// files from disk. Obviously, you shouldn't set both!

#define USE_IMAGEMAGICK
// #define USE_FREEIMAGE

// --------------------------------------------------------------------------
// ImageMagick implementation of the InitializeTexture() function
#ifdef USE_IMAGEMAGICK

#include <Magick++.h>

bool InitializeTexture(MyTexture *texture, const string &imageFileName)
{
    Magick::Image myImage;
    
    // try to read the provided image file
    try {
        myImage.read(imageFileName);
    }
    catch (Magick::Error &error) {
        cout << "Magick++ failed to read image " << imageFileName << endl;
        cout << "ERROR: " << error.what() << endl;
        return false;
    }
    
    // store the image width and height into the texture structure
    texture->width = myImage.columns();
    texture->height = myImage.rows();
    
    // create a Magick++ pixel cache from the image for direct access to data
    Magick::Pixels pixelCache(myImage);
    Magick::PixelPacket *pixels;
    pixels = pixelCache.get(0, 0, texture->width, texture->height);
    
    // determine the number of stored bytes per pixel channel in the cache
    GLenum channelDataType;
    switch (sizeof(Magick::Quantum)) {
        case 4:     channelDataType = GL_UNSIGNED_INT;      break;
        case 2:     channelDataType = GL_UNSIGNED_SHORT;    break;
        default:    channelDataType = GL_UNSIGNED_BYTE;
    }
    
    // create a texture name to associate our image data with
    if (!texture->textureName)
        glGenTextures(1, &texture->textureName);
    
    // bind the texture as a "rectangle" to access using image pixel coordinates
    glBindTexture(GL_TEXTURE_RECTANGLE, texture->textureName);

    // allocate the GPU texture memory for an image of the given size and format
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB, texture->width,
                 texture->height, 0, GL_BGRA, channelDataType, 0);
    
    // send image pixel data to OpenGL texture memory, reversing the vertical axis
    for (int row = 0; row < texture->height; ++row)
    {
        int offset = (texture->height - row - 1) * texture->width;
        glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, row, texture->width,
                        1, GL_BGRA, channelDataType, pixels + offset);
    }
    
    // unbind this texture
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    
    return true;
}

#endif
// --------------------------------------------------------------------------
// FreeImage implementation of the InitializeTexture() function
#ifdef USE_FREEIMAGE

#include <FreeImage.h>

bool InitializeTexture(MyTexture *mytex, const string &imageFileName)
{
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	FIBITMAP* dib = nullptr;
	BYTE* bits = nullptr;

	fif = FreeImage_GetFileType(imageFileName.c_str(), 0);
	if (fif == FIF_UNKNOWN)
		fif = FreeImage_GetFIFFromFilename(imageFileName.c_str());
	if (fif == FIF_UNKNOWN)
		return false;

	if (FreeImage_FIFSupportsReading(fif))
		dib = FreeImage_Load(fif, imageFileName.c_str());
	if (!dib)
		return false;

	bits = FreeImage_GetBits(dib);
	mytex->width = FreeImage_GetWidth(dib);
	mytex->height = FreeImage_GetHeight(dib);
	if ((bits == 0) || (mytex->width == 0) || (mytex->height == 0))
		return false;

	glGenTextures(1, &mytex->textureName);
	glBindTexture(GL_TEXTURE_RECTANGLE, mytex->textureName);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB, mytex->width, mytex->height,
		0, GL_BGR, GL_UNSIGNED_BYTE, bits);

	FreeImage_Unload(dib);

	return true;
}

#endif
// --------------------------------------------------------------------------
