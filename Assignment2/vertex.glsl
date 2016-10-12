// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// location indices for these attributes correspond to those specified in the
// InitializeGeometry() function of the main program
layout(location = 0) in vec2 VertexPosition;
layout(location = 1) in vec3 VertexColour;
layout(location = 2) in vec2 VertexTexture;

// output to be interpolated between vertices and passed to the fragment stage
out vec3 Colour;
out vec2 textureCoords;
out vec2 newVertexPosition;

uniform vec2 image_offset;
uniform float magnification;
uniform float rotation;

void main()
{
	newVertexPosition.x = (VertexPosition.x * magnification) + image_offset.x/512;
	newVertexPosition.y = (VertexPosition.y * magnification) + image_offset.y/512;

	float tmpx = newVertexPosition.x;
	float tmpy = newVertexPosition.y;

	newVertexPosition.x = cos(rotation)*tmpx - sin(rotation)*tmpy;
	newVertexPosition.y = sin(rotation)*tmpx + cos(rotation)*tmpy;

    // assign vertex position without modification
    gl_Position = vec4(newVertexPosition, 0.0, 1.0);

    // assign output colour to be interpolated
    Colour = VertexColour;
    textureCoords = VertexTexture;
}
