// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// interpolated colour received from vertex stage
in vec3 Colour;
in vec2 textureCoords;

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

uniform sampler2DRect tex;
uniform vec3 luminance;

void main(void)
{
    int res = 1;
    vec2 newCoords;
    newCoords.x = res * (int(textureCoords.x)/res);
    newCoords.y = res * (int(textureCoords.y)/res);

    // write colour output without modification
    vec4 colour = texture(tex, newCoords);

	if (luminance.r < 1) {
		float lum = colour.r * luminance.r + colour.g * luminance.g + colour.b * luminance.b;

		colour.r = lum;
		colour.g = lum;
		colour.b = lum;
	}

    FragmentColour = colour;
}
