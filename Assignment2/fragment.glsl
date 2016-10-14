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
uniform float sepia;
uniform uint sobel;

mat3 sobel_hor = mat3( 
	1.0, 2.0, 1.0, 
	0.0, 0.0, 0.0, 
	-1.0, -2.0, -1.0 
);

mat3 sobel_ver = mat3( 
	1.0, 0.0, -1.0, 
	2.0, 0.0, -2.0, 
	1.0, 0.0, -1.0 
);

mat3 sobel_uns = mat3( 
	0.0, -1.0, 0.0, 
	-1.0, 5.0, -1.0, 
	0.0, -1.0, 0.0 
);

// Code taken from http://computergraphics.stackexchange.com/questions/3646/opengl-glsl-sobel-edge-detection-filter
vec3 detect_edge(vec2 coords)
{
	mat3 I;
	float filtered;
	for (int i=0; i<3; i++) {
		for (int j=0; j<3; j++) {
			vec3 samp = texelFetch(tex, ivec2(textureCoords) + ivec2(i-1,j-1)).rgb;
			I[i][j] = length(samp); 
		}
	}

	if (sobel == 1) {
		filtered = dot(sobel_hor[0], I[0]) + dot(sobel_hor[1], I[1]) + dot(sobel_hor[2], I[2]); 
	} else if (sobel == 2) {
		filtered = dot(sobel_ver[0], I[0]) + dot(sobel_ver[1], I[1]) + dot(sobel_ver[2], I[2]);
	} else if (sobel == 3) {
		filtered = dot(sobel_uns[0], I[0]) + dot(sobel_uns[1], I[1]) + dot(sobel_uns[2], I[2]);
	}

	return vec3(filtered);
}

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
	if (sepia > 0) {
		float inputRed = colour.r;
		float inputBlue = colour.g;
		float inputGreen = colour.b;

		colour.r = (inputRed * .393) + (inputGreen *.769) + (inputBlue * .189);
		colour.g = (inputRed * .349) + (inputGreen *.686) + (inputBlue * .168);
		colour.b = (inputRed * .272) + (inputGreen *.534) + (inputBlue * .131);
	}

	if (sobel > 0) {
		colour.rgb = detect_edge(textureCoords.xy);
	}

    FragmentColour = colour;
}
