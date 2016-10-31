// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include "GlyphExtractor.h"
#include <vector>

// Specify that we want the OpenGL core profile before including GLFW headers
#ifndef LAB_LINUX
	#include <glad/glad.h>
#else
	#define GLFW_INCLUDE_GLCOREARB
	#define GL_GLEXT_PROTOTYPES
#endif
#include <GLFW/glfw3.h>

//STB
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
using namespace std;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint TCSshader, GLuint TESshader, GLuint fragmentShader); 

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

enum Scene
{
	QUAD,
	CUBIC,
	FONT1,
	FONT2,
	FONT3,
	SCROLL1,
	SCROLL2,
	SCROLL3,
};

struct Coordinate
{
	double x;
	double y;
};

struct CurrentState
{
	enum Scene scene;
	int speed;
};

CurrentState current_state;
GlyphExtractor extractor;

float bezier_deg;
float global_deg;

struct MyShader
{
	// OpenGL names for vertex and fragment shaders, shader program
	GLuint  vertex;
	GLuint  TCS; 
	GLuint  TES; 
	GLuint  fragment;
	GLuint  program;
	GLuint  program2;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0), program(0)
	{}
};

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader)
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");
	string TCSSource = LoadSource("tessControl.glsl");
	string TESSource = LoadSource("tessEval.glsl"); 
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource); 
	shader->TCS = CompileShader(GL_TESS_CONTROL_SHADER, TCSSource);
	shader->TES = CompileShader(GL_TESS_EVALUATION_SHADER, TESSource);

	// link shader program
	shader->program = LinkProgram(shader->vertex, shader->TCS, shader->TES, shader->fragment);
	shader->program2 = LinkProgram(shader->vertex, 0, 0, shader->fragment);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader->program);
	glDeleteProgram(shader->program2);
	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);
	glDeleteShader(shader->TCS); 
	glDeleteShader(shader->TES); 
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing textures

struct MyTexture
{
	GLuint textureID;
	GLuint target;
	int width;
	int height;

	// initialize object names to zero (OpenGL reserved value)
	MyTexture() : textureID(0), target(0), width(0), height(0)
	{}
};

bool InitializeTexture(MyTexture* texture, const char* filename, GLuint target = GL_TEXTURE_2D)
{
	int numComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename, &texture->width, &texture->height, &numComponents, 0);
	if (data != nullptr)
	{
		texture->target = target;
		glGenTextures(1, &texture->textureID);
		glBindTexture(texture->target, texture->textureID);
		GLuint format = numComponents == 3 ? GL_RGB : GL_RGBA;
		glTexImage2D(texture->target, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);

		// Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
		// GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Clean up
		glBindTexture(texture->target, 0);
		stbi_image_free(data);
		return !CheckGLErrors();
	}
	return true; //error
}

// deallocate texture-related objects
void DestroyTexture(MyTexture *texture)
{
	glBindTexture(texture->target, 0);
	glDeleteTextures(1, &texture->textureID);
}

void SaveImage(const char* filename, int width, int height, unsigned char *data, int numComponents = 3, int stride = 0)
{
	if (!stbi_write_png(filename, width, height, numComponents, data, stride))
		cout << "Unable to save image: " << filename << endl;
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

MyGeometry global_geo;

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry, vector<float>& vertices, vector<float>& colours) {
	// three vertex positions and assocated colours of a triangle

	geometry->elementCount = vertices.size()/2; 

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertices.size(), &vertices[0], GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*colours.size(), &colours[0], GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
	glDeleteBuffers(1, &geometry->colourBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void create_vertices_colours(vector<float>& vertices, vector<float>& vertices2, vector<float>& colours, vector<float>& colours2, vector<float>& colours3) {
	if (bezier_deg == 3) {
		// Curve 1
		vertices.push_back(0.4);
		vertices.push_back(0.4);
		vertices.push_back(0.8);
		vertices.push_back(-0.4);
		vertices.push_back(0.0);
		vertices.push_back(-0.4);

		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);

		vertices2.push_back(0.4);
		vertices2.push_back(0.4);
		vertices2.push_back(0.8);
		vertices2.push_back(-0.4);
		vertices2.push_back(0.8);
		vertices2.push_back(-0.4);
		vertices2.push_back(0.0);
		vertices2.push_back(-0.4);
		// Curve 2
		vertices.push_back(0.0);
		vertices.push_back(-0.4);
		vertices.push_back(-0.8);
		vertices.push_back(-0.4);
		vertices.push_back(-0.4);
		vertices.push_back(0.4);

		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);

		vertices2.push_back(0.0);
		vertices2.push_back(-0.4);
		vertices2.push_back(-0.8);
		vertices2.push_back(-0.4);
		vertices2.push_back(-0.8);
		vertices2.push_back(-0.4);
		vertices2.push_back(-0.4);
		vertices2.push_back(0.4);
		// Curve 3
		vertices.push_back(-0.4);
		vertices.push_back(0.4);
		vertices.push_back(0.0);
		vertices.push_back(0.4);
		vertices.push_back(0.4);
		vertices.push_back(0.4);

		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);

		vertices2.push_back(-0.4);
		vertices2.push_back(0.4);
		vertices2.push_back(0.0);
		vertices2.push_back(0.4);
		vertices2.push_back(0.0);
		vertices2.push_back(0.4);
		vertices2.push_back(0.4);
		vertices2.push_back(0.4);
		// Curve 4
		vertices.push_back(0.48);
		vertices.push_back(0.2);
		vertices.push_back(1.0);
		vertices.push_back(0.4);
		vertices.push_back(0.52);
		vertices.push_back(0.16);

		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);

		vertices2.push_back(0.48);
		vertices2.push_back(0.2);
		vertices2.push_back(1.0);
		vertices2.push_back(0.4);
		vertices2.push_back(1.0);
		vertices2.push_back(0.4);
		vertices2.push_back(0.52);
		vertices2.push_back(0.16);

		for (uint i = 0; i <= vertices.size(); i++) {
			colours.push_back(0);
			colours.push_back(1);
			colours.push_back(0);
		}
		for (uint i = 0; i <= vertices2.size(); i++) {
			colours2.push_back(0.5);
			colours2.push_back(0.5);
			colours2.push_back(0.5);
		}
	} else if (bezier_deg == 4) {
		float f = 0.1;
		// Curve 1
		vertices.push_back(1.0*f-0.5);
		vertices.push_back(1.0*f-0.5);
		vertices.push_back(4.0*f-0.5);
		vertices.push_back(0.0-0.8);
		vertices.push_back(6.0*f-0.5);
		vertices.push_back(2.0*f-0.5);
		vertices.push_back(9.0*f-0.5);
		vertices.push_back(1.0*f-0.5);

		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);

		vertices2.push_back(1.0*f-0.5);
		vertices2.push_back(1.0*f-0.5);
		vertices2.push_back(4.0*f-0.5);
		vertices2.push_back(0.0-0.8);
		vertices2.push_back(4.0*f-0.5);
		vertices2.push_back(0.0-0.8);
		vertices2.push_back(6.0*f-0.5);
		vertices2.push_back(2.0*f-0.5);
		vertices2.push_back(6.0*f-0.5);
		vertices2.push_back(2.0*f-0.5);
		vertices2.push_back(9.0*f-0.5);
		vertices2.push_back(1.0*f-0.5);

		// Curve 2
		vertices.push_back(8.0*f-0.5);
		vertices.push_back(2.0*f-0.5);
		vertices.push_back(0.0*f-0.5);
		vertices.push_back(8.0*f-0.5);
		vertices.push_back(0.0*f-0.5);
		vertices.push_back(-2.0*f-0.5);
		vertices.push_back(8.0*f-0.5);
		vertices.push_back(4.0*f-0.5);

		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);

		vertices2.push_back(8.0*f-0.5);
		vertices2.push_back(2.0*f-0.5);
		vertices2.push_back(0.0*f-0.5);
		vertices2.push_back(8.0*f-0.5);
		vertices2.push_back(0.0*f-0.5);
		vertices2.push_back(8.0*f-0.5);
		vertices2.push_back(0.0*f-0.5);
		vertices2.push_back(-2.0*f-0.5);
		vertices2.push_back(0.0*f-0.5);
		vertices2.push_back(-2.0*f-0.5);
		vertices2.push_back(8.0*f-0.5);
		vertices2.push_back(4.0*f-0.5);
		// Curve 3
		vertices.push_back(5.0*f-0.5);
		vertices.push_back(3.0*f-0.5);
		vertices.push_back(3.0*f-0.5);
		vertices.push_back(2.0*f-0.5);
		vertices.push_back(3.0*f-0.5);
		vertices.push_back(3.0*f-0.5);
		vertices.push_back(5.0*f-0.5);
		vertices.push_back(2.0*f-0.5);

		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);

		vertices2.push_back(5.0*f-0.5);
		vertices2.push_back(3.0*f-0.5);
		vertices2.push_back(3.0*f-0.5);
		vertices2.push_back(2.0*f-0.5);
		vertices2.push_back(3.0*f-0.5);
		vertices2.push_back(2.0*f-0.5);
		vertices2.push_back(3.0*f-0.5);
		vertices2.push_back(3.0*f-0.5);
		vertices2.push_back(3.0*f-0.5);
		vertices2.push_back(3.0*f-0.5);
		vertices2.push_back(5.0*f-0.5);
		vertices2.push_back(2.0*f-0.5);
		// Curve 4
		vertices.push_back(3.0*f-0.5);
		vertices.push_back(2.2*f-0.5);
		vertices.push_back(3.5*f-0.5);
		vertices.push_back(2.7*f-0.5);
		vertices.push_back(3.5*f-0.5);
		vertices.push_back(3.3*f-0.5);
		vertices.push_back(3.0*f-0.5);
		vertices.push_back(3.8*f-0.5);

		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);

		vertices2.push_back(3.0*f-0.5);
		vertices2.push_back(2.2*f-0.5);
		vertices2.push_back(3.5*f-0.5);
		vertices2.push_back(2.7*f-0.5);
		vertices2.push_back(3.5*f-0.5);
		vertices2.push_back(2.7*f-0.5);
		vertices2.push_back(3.5*f-0.5);
		vertices2.push_back(3.3*f-0.5);
		vertices2.push_back(3.5*f-0.5);
		vertices2.push_back(3.3*f-0.5);
		vertices2.push_back(3.0*f-0.5);
		vertices2.push_back(3.8*f-0.5);
		// Curve 5
		vertices.push_back(2.8*f-0.5);
		vertices.push_back(3.5*f-0.5);
		vertices.push_back(2.4*f-0.5);
		vertices.push_back(3.8*f-0.5);
		vertices.push_back(2.4*f-0.5);
		vertices.push_back(3.2*f-0.5);
		vertices.push_back(2.8*f-0.5);
		vertices.push_back(3.5*f-0.5);

		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);
		colours3.push_back(1);
		colours3.push_back(1);
		colours3.push_back(0);
		colours3.push_back(0);

		vertices2.push_back(2.8*f-0.5);
		vertices2.push_back(3.5*f-0.5);
		vertices2.push_back(2.4*f-0.5);
		vertices2.push_back(3.8*f-0.5);
		vertices2.push_back(2.4*f-0.5);
		vertices2.push_back(3.8*f-0.5);
		vertices2.push_back(2.4*f-0.5);
		vertices2.push_back(3.2*f-0.5);
		vertices2.push_back(2.4*f-0.5);
		vertices2.push_back(3.2*f-0.5);
		vertices2.push_back(2.8*f-0.5);
		vertices2.push_back(3.5*f-0.5);

		for (uint i = 0; i <= vertices.size(); i++) {
			colours.push_back(0);
			colours.push_back(1);
			colours.push_back(0);
		}
		for (uint i = 0; i <= vertices2.size(); i++) {
			colours2.push_back(0.5);
			colours2.push_back(0.5);
			colours2.push_back(0.5);
		}
	}
}

void RenderScene(MyGeometry *geometry, MyShader *shader)
{
	// clear screen to a dark grey colour
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	if (bezier_deg != 0) {
		vector<float> vertices;
		vector<float> vertices2;
		vector<float> colours;
		vector<float> colours2;
		vector<float> colours3;
		create_vertices_colours(vertices, vertices2, colours, colours2, colours3);

		glPointSize(4);
		if (!InitializeGeometry(&global_geo, vertices, colours3))
			cout << "Program failed to intialize geometry!" << endl;
		glUseProgram(shader->program2);
		glBindVertexArray(geometry->vertexArray);
		glDrawArrays(GL_POINTS, 0, geometry->elementCount);

		if (!InitializeGeometry(&global_geo, vertices2, colours2))
			cout << "Program failed to intialize geometry!" << endl;
		glUseProgram(shader->program2);
		glBindVertexArray(geometry->vertexArray);
		glDrawArrays(GL_LINES, 0, geometry->elementCount);

		if (!InitializeGeometry(&global_geo, vertices, colours))
			cout << "Program failed to intialize geometry!" << endl;
		glUseProgram(shader->program);
		glBindVertexArray(geometry->vertexArray);
		glDrawArrays(GL_PATCHES, 0, geometry->elementCount);
	}

	glUseProgram(shader->program);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_PATCHES, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

float insert_char(vector<float>& vertices, char c, float advance, float scale, float shift) {
	MyGlyph glyph = extractor.ExtractGlyph(c);

	for (unsigned int contour_index = 0; contour_index < glyph.contours.size(); contour_index++) {   
		MyContour contour = glyph.contours[contour_index];
		for (unsigned int segment_index = 0; segment_index < contour.size(); segment_index++) {
			MySegment segment = contour[segment_index];
			for (unsigned i = 0; i <= segment.degree; i++) {
				if (segment.degree == 1 && global_deg == 4) {
					vertices.push_back((segment.x[i] + advance - shift) * scale);	
					vertices.push_back((segment.y[i]) * scale);	
					vertices.push_back((segment.x[i] + advance - shift) * scale);	
					vertices.push_back((segment.y[i]) * scale);	
				} else if (segment.degree == 1 && global_deg == 3) {
					vertices.push_back((segment.x[i] + advance - shift) * scale);	
					vertices.push_back((segment.y[i]) * scale);	
					if (i == 0) {
						vertices.push_back((segment.x[i] + advance - shift) * scale);	
						vertices.push_back((segment.y[i]) * scale);	
					}
				} else {
					vertices.push_back((segment.x[i] + advance - shift) * scale);	
					vertices.push_back((segment.y[i]) * scale);	
				}
			}
		}
	}
	return glyph.advance;
}

void update_display() {
	vector<float> vertices;
	vector<float> colours;

	switch(current_state.scene) {
		case QUAD:
			bezier_deg = 3;
			global_deg = 3;
			break;
		case CUBIC:
			bezier_deg = 4;
			global_deg = 4;
			break;
		case FONT1:
			{
				bezier_deg = 0;
				global_deg = 3;
				extractor.LoadFontFile("fonts/Pacifico.ttf");
				float adv = insert_char(vertices, 'I', 0, 0.6, 1.5);
				adv += insert_char(vertices, 'n', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'd', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'r', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'a', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'P', adv, 0.6, 1.5);

				for (uint i = 0; i <= vertices.size(); i++) {
					colours.push_back(1);
					colours.push_back(1);
					colours.push_back(1);
				}
				if (!InitializeGeometry(&global_geo, vertices, colours))
					cout << "Program failed to intialize geometry!" << endl;
				break;
			}
		case FONT2:
			{
				bezier_deg = 0;
				global_deg = 3;
				extractor.LoadFontFile("fonts/Lora-Regular.ttf");
				float adv = insert_char(vertices, 'I', 0, 0.6, 1.5);
				adv += insert_char(vertices, 'n', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'd', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'r', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'a', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'P', adv, 0.6, 1.5);

				for (uint i = 0; i <= vertices.size(); i++) {
					colours.push_back(1);
					colours.push_back(1);
					colours.push_back(1);
				}
				if (!InitializeGeometry(&global_geo, vertices, colours))
					cout << "Program failed to intialize geometry!" << endl;
				break;
			}
		case FONT3:
			{
				bezier_deg = 0;
				global_deg = 4;
				extractor.LoadFontFile("fonts/SourceSansPro-Regular.otf");
				float adv = insert_char(vertices, 'I', 0, 0.6, 1.5);
				adv += insert_char(vertices, 'n', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'd', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'r', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'a', adv, 0.6, 1.5);
				adv += insert_char(vertices, 'P', adv, 0.6, 1.5);

				for (uint i = 0; i <= vertices.size(); i++) {
					colours.push_back(1);
					colours.push_back(1);
					colours.push_back(1);
				}
				if (!InitializeGeometry(&global_geo, vertices, colours))
					cout << "Program failed to intialize geometry!" << endl;
				break;
			}
		case SCROLL1:
			extractor.LoadFontFile("fonts/Inconsolata.otf");
			break;
		case SCROLL2:
			extractor.LoadFontFile("fonts/AlexBrush-Regular.ttf");
			break;
		case SCROLL3:
			extractor.LoadFontFile("fonts/Pacifico.ttf");
			break;
	}
}

float shift = -1.8;
void scroll() {
	vector<float> vertices;
	vector<float> colours;
	bezier_deg = 0;
	
	shift += (current_state.speed*1.0/200);
	if (current_state.scene == SCROLL1 && shift > 24) {	
		shift = -1.8;
	} else if (current_state.scene == SCROLL2 && shift > 20) {	
		shift = -1.8;
	} else if (current_state.scene == SCROLL3 && shift > 23) {	
		shift = -1.8;
	}
	float adv = insert_char(vertices, 'T', 0, 0.6, shift);
	adv += insert_char(vertices, 'h', adv, 0.6, shift);
	adv += insert_char(vertices, 'e', adv, 0.6, shift);
	adv += insert_char(vertices, ' ', adv, 0.6, shift);
	adv += insert_char(vertices, 'Q', adv, 0.6, shift);
	adv += insert_char(vertices, 'u', adv, 0.6, shift);
	adv += insert_char(vertices, 'i', adv, 0.6, shift);
	adv += insert_char(vertices, 'c', adv, 0.6, shift);
	adv += insert_char(vertices, 'k', adv, 0.6, shift);
	adv += insert_char(vertices, ' ', adv, 0.6, shift);
	adv += insert_char(vertices, 'B', adv, 0.6, shift);
	adv += insert_char(vertices, 'r', adv, 0.6, shift);
	adv += insert_char(vertices, 'o', adv, 0.6, shift);
	adv += insert_char(vertices, 'w', adv, 0.6, shift);
	adv += insert_char(vertices, 'n', adv, 0.6, shift);
	adv += insert_char(vertices, ' ', adv, 0.6, shift);
	adv += insert_char(vertices, 'F', adv, 0.6, shift);
	adv += insert_char(vertices, 'o', adv, 0.6, shift);
	adv += insert_char(vertices, 'x', adv, 0.6, shift);
	adv += insert_char(vertices, ' ', adv, 0.6, shift);
	adv += insert_char(vertices, 'J', adv, 0.6, shift);
	adv += insert_char(vertices, 'u', adv, 0.6, shift);
	adv += insert_char(vertices, 'm', adv, 0.6, shift);
	adv += insert_char(vertices, 'p', adv, 0.6, shift);
	adv += insert_char(vertices, 's', adv, 0.6, shift);
	adv += insert_char(vertices, ' ', adv, 0.6, shift);
	adv += insert_char(vertices, 'O', adv, 0.6, shift);
	adv += insert_char(vertices, 'v', adv, 0.6, shift);
	adv += insert_char(vertices, 'e', adv, 0.6, shift);
	adv += insert_char(vertices, 'r', adv, 0.6, shift);
	adv += insert_char(vertices, ' ', adv, 0.6, shift);
	adv += insert_char(vertices, 'T', adv, 0.6, shift);
	adv += insert_char(vertices, 'h', adv, 0.6, shift);
	adv += insert_char(vertices, 'e', adv, 0.6, shift);
	adv += insert_char(vertices, ' ', adv, 0.6, shift);
	adv += insert_char(vertices, 'L', adv, 0.6, shift);
	adv += insert_char(vertices, 'a', adv, 0.6, shift);
	adv += insert_char(vertices, 'z', adv, 0.6, shift);
	adv += insert_char(vertices, 'y', adv, 0.6, shift);
	adv += insert_char(vertices, ' ', adv, 0.6, shift);
	adv += insert_char(vertices, 'D', adv, 0.6, shift);
	adv += insert_char(vertices, 'o', adv, 0.6, shift);
	adv += insert_char(vertices, 'g', adv, 0.6, shift);
	adv += insert_char(vertices, '.', adv, 0.6, shift);

	for (uint i = 0; i <= vertices.size(); i++) {
		colours.push_back(1);
		colours.push_back(1);
		colours.push_back(1);
	}

	if (!InitializeGeometry(&global_geo, vertices, colours))
		cout << "Program failed to intialize geometry!" << endl;
}

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	bool should_update_display = false;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	} else if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		current_state.scene = QUAD;
		should_update_display = true;
	} else if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		current_state.scene = CUBIC;
		should_update_display = true;
	} else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		current_state.scene = FONT1;
		bezier_deg = 0;
		should_update_display = true;
	} else if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		current_state.scene = FONT2;
		bezier_deg = 0;
		should_update_display = true;
	} else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		current_state.scene = FONT3;
		bezier_deg = 0;
		should_update_display = true;
	} else if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		global_deg = 4;
		current_state.scene = SCROLL1;
		bezier_deg = 0;
		should_update_display = true;
	} else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		global_deg = 3;
		current_state.scene = SCROLL2;
		bezier_deg = 0;
		should_update_display = true;
	} else if (key == GLFW_KEY_D && action == GLFW_PRESS) {
		global_deg = 3;
		current_state.scene = SCROLL3;
		bezier_deg = 0;
		should_update_display = true;
	} else if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		if(current_state.speed + 1 < 8)
			current_state.speed += 1;
		should_update_display = true;
	} else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		if(current_state.speed - 1 > 0)
			current_state.speed -= 1;
		should_update_display = true;
	}

	if (should_update_display)
		update_display();
}

// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	current_state.scene = QUAD;
	current_state.speed = 1;
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(512, 512, "CPSC 453 OpenGL Boilerplate", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwMakeContextCurrent(window);
	
	//Intialize GLAD if not lab linux
	#ifndef LAB_LINUX
	if (!gladLoadGL())
	{
		cout << "GLAD init failed" << endl;
		return -1;
	}
	#endif
	
	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	MyShader shader;
	if (!InitializeShaders(&shader)) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}


	update_display();

	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		glUseProgram(shader.program);
		glPatchParameteri(GL_PATCH_VERTICES, global_deg);

		GLint bezier = glGetUniformLocation(shader.program, "degree");
		glUniform1ui(bezier, global_deg);
		// call function to draw our scene
		if (current_state.scene == SCROLL1 || current_state.scene == SCROLL2 || current_state.scene == SCROLL3)
			scroll();
		RenderScene(&global_geo, &shader); //render scene with texture
								
		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&global_geo);
	DestroyShaders(&shader);
	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint TCSshader, GLuint TESshader, GLuint fragmentShader) 
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (TCSshader) glAttachShader(programObject, TCSshader); 
	if (TESshader) glAttachShader(programObject, TESshader); 
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}
