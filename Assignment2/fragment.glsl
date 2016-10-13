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

float intensity(in vec4 pixel) 
{
	return sqrt((pixel.x*pixel.x)+(pixel.y*pixel.y)+(pixel.z*pixel.z));
}

float threshold(in float thr1, in float thr2 , in float val) {
	if (val < thr1) {return 0.0;}
	if (val > thr2) {return 1.0;}
	return val;
}

vec3 detect_edge(vec2 coords)
{
	float d = 1.0/512.0;

	float tleft = intensity(texture2DRect(tex,vec2(-d,d)));
	float cleft = intensity(texture2DRect(tex,vec2(-d,0)));
	float bleft = intensity(texture2DRect(tex,vec2(-d,-d)));
	float tcenter = intensity(texture2DRect(tex,vec2(0,d)));
	float bcenter = intensity(texture2DRect(tex,vec2(0,-d)));
	float tright = intensity(texture2DRect(tex,vec2(d,d)));
	float cright = intensity(texture2DRect(tex,vec2(d,0)));
	float bright = intensity(texture2DRect(tex,vec2(d,-d)));

	float x = tleft + 2.0*cleft + bleft - tright - 2.0*cright - bright;
	float y = -tleft - 2.0*tcenter - tright + bleft + 2.0*bcenter + bright;
	
	float color = sqrt((x*x) + (y*y));
	if (color > 1.0)
		return vec3(0.0,0.0,0.0);

	return vec3(1.0,1.0,1.0);
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

	colour.rgb = detect_edge(textureCoords.xy);

    FragmentColour = colour;
}
