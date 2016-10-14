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
uniform uint gaussize;

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

vec4 gaussian() {
	float half;
	float sigma;

	if (gaussize == 3) {
		half = 1;
		sigma = 0.66;
	} else if (gaussize == 5) {
		half = 2;
		sigma = 0.997;
	} else if (gaussize == 7) {
		half = 3;
		sigma = 1.45;
	}
	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);
	
	for (int i = 0; i < gaussize; i++) {
		for (int j = 0; j < gaussize; j++) {
			vec4 tmp = texture(tex, textureCoords + i - half + j - half);
			float gaus = exp(-1 * (pow(i - half, 2) + pow(j - half, 2)) / (2 * pow(sigma, 2)))
				/(2 * 3.14159265 * pow(sigma, 2));

			result.r += tmp.r * gaus;
			result.g += tmp.g * gaus;
			result.b += tmp.b * gaus;
		}
	}
	
	return result;
}

// code inspired from http://stackoverflow.com/questions/2797549/block-filters-using-fragment-shaders
vec4 detect_edge()
{
	float sobel_hor[9] = float[9](1.0, 0.0, -1.0, 2.0, 0.0, -2.0, 1.0, -2.0, -1.0);
	float sobel_ver[9] = float[9](-1.0, -2.0, -1.0, 0.0, 0.0, 0.0, 1.0, 2.0, 1.0);
	float sobel_uns[9] = float[9](0.0, -1.0, 0.0, -1.0, 5.0, -1.0, -0.0, -1.0, 0.0);

	vec4 sum = vec4(0.0, 0.0, 0.0, 1.0);
	vec2 offset[9];

	offset[0] = vec2(-1, -1); 
	offset[1] = vec2(0.0, -1);
	offset[2] = vec2(1, -1); 

	offset[3] = vec2(-1, 0.0);
	offset[4] = vec2(0.0, 0.0);
	offset[5] = vec2(1, 0.0); 

	offset[6] = vec2(-1, 1); 
	offset[7] = vec2(0.0, 1);
	offset[8] = vec2(1, 1); 

	for( int i = 0; i < 9; i++ )
	{
		vec4 tmp = texture(tex, textureCoords + offset[i]);
		if (sobel == 1) {
			sum += tmp * sobel_hor[i];
		} else if (sobel == 2) {
			sum += tmp * sobel_ver[i];
		} else if (sobel == 3) {
			sum += tmp * sobel_uns[i];
		}
	}

	return sum;
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
		colour = detect_edge();
	}

	if (gaussize > 0) {
		colour = gaussian();
	}

    FragmentColour = colour;
}
