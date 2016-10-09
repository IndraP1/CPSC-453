// ==========================================================================
// Image File Reading Support Functions
//  - requires the Magick++ development libraries: http://www.imagemagick.org
//  - or the FreeImage library: http://freeimage.sourceforge.net
//
// You may use this code (or not) however you see fit for your work.
//
// Add this header and the accompanying .cpp source file to your project, and
// ensure that you have one of the above libraries installed and the paths to
// it are set correctly.
//
// Authors: Sonny Chan, Alex Brown
//          University of Calgary
// Date:    January-March 2016
// ==========================================================================

#ifndef __IMAGE_READER_H__
#define __IAMGE_READER_H__

#include <string>

// --------------------------------------------------------------------------
// A structure to hold an allocated OpenGL texture along with its dimensions.

struct MyTexture
{
    // OpenGL names for array buffer objects, vertex array object
    GLuint  textureName;
    
    // dimensions of the image stored in this texture
    GLuint  width, height;
    
    // initialize object names to zero (OpenGL reserved value)
    MyTexture() : textureName(0), width(0), height(0)
    {}
};

// --------------------------------------------------------------------------
// Prototype for a function that loads an image with a given file name into
// an instance of the above structure. Returns true if successful.

bool InitializeTexture(MyTexture *texture, const std::string &imageFileName);

// --------------------------------------------------------------------------
#endif
