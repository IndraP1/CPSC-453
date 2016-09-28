// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2basepos
// ==========================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>

#include <vector>
#include <cmath>

// specify that we want the OpenGL core profile before including GLFW headers
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

using namespace std;

// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);


// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering
struct Coordinates {
	float x;
	float y;
};

struct Triangle {
	Coordinates a;
	Coordinates b;
	Coordinates c;
	
	float color;
	float width;
};


struct MyShader
{
	// OpenGL names for vertex and fragment shaders, shader program
	GLuint  vertex;
	GLuint  fragment;
	GLuint  program;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0), program(0)
	{}
};

enum Shape 
{
	SQUARES_DIAMONDS,
	SPIRAL,
	TRIANGLES
};

struct CurrentState
{
	enum Shape shape;
	int layer;
};

CurrentState currentstate;

// Custom function prototypes
void Sierpinski(vector<float>& vertices, vector<float>& colours, Triangle& triangle_prev, int recursions);

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader)
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// link shader program
	shader->program = LinkProgram(shader->vertex, shader->fragment);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader->program);
	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;
	GLenum renderMode;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0), renderMode(0)
	{}
};

MyGeometry globalGeo;

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry, vector<float>& vertices, vector<float>& colours)
{
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

void GenerateSquaresDiamonds(MyGeometry *geometry, vector<float>& vertices, vector<float>& colours, int layer) 
{
	float basepos = 0.9f;
	float baseneg = -0.9f;

	for (int i=0; i<layer; i++) {
		// SQUARES
		vertices.push_back(baseneg/(pow(2,i)));
		vertices.push_back(baseneg/(pow(2,i)));
		vertices.push_back(basepos/(pow(2,i)));
		vertices.push_back(baseneg/(pow(2,i)));

		vertices.push_back(basepos/(pow(2,i)));
		vertices.push_back(baseneg/(pow(2,i)));
		vertices.push_back(basepos/(pow(2,i)));
		vertices.push_back(basepos/(pow(2,i)));

		vertices.push_back(basepos/(pow(2,i)));
		vertices.push_back(basepos/(pow(2,i)));
		vertices.push_back(baseneg/(pow(2,i)));
		vertices.push_back(basepos/(pow(2,i)));

		vertices.push_back(baseneg/(pow(2,i)));
		vertices.push_back(basepos/(pow(2,i)));
		vertices.push_back(baseneg/(pow(2,i)));
		vertices.push_back(baseneg/(pow(2,i)));
	}

	for (int i=0; i<layer; i++) {
		for (int j=0; j<8; j++) {
			colours.push_back(1.0-(i*0.2));
			colours.push_back(0.0);
			colours.push_back(0.0);
		}
	}


	// DIAMONDS
	for (int i=0; i<layer; i++) {
		vertices.push_back(0.0);
		vertices.push_back(baseneg/(pow(2,i)));
		vertices.push_back(basepos/(pow(2,i)));
		vertices.push_back(0.0);

		vertices.push_back(basepos/(pow(2,i)));
		vertices.push_back(0.0);
		vertices.push_back(0.0);
		vertices.push_back(basepos/(pow(2,i)));

		vertices.push_back(0.0);
		vertices.push_back(basepos/(pow(2,i)));
		vertices.push_back(baseneg/(pow(2,i)));
		vertices.push_back(0.0);

		vertices.push_back(baseneg/(pow(2,i)));
		vertices.push_back(0.0);
		vertices.push_back(0.0);
		vertices.push_back(baseneg/(pow(2,i)));
	}

	for (int i=0; i<layer; i++) {
		for (int j=0; j<8; j++) {
			colours.push_back(0.0);
			colours.push_back(1.0-(i*0.2));
			colours.push_back(0.0);
		}
	}

	geometry->elementCount = layer*16;
	geometry->renderMode = GL_LINES;
}

void GenerateSpiral(MyGeometry *geometry, vector<float>& vertices, vector<float>& colours, int layer) 
{
	// code for colour taken from tutorial
	float red = 1.f;
	float green = 0.f;
	float blue = 0.f;

	float maxradius = 360*currentstate.layer;
	for(int i=0; i<=maxradius; ++i)
	{
		float rad = (float)layer*i*2*M_PI/maxradius;
		vertices.push_back((i/maxradius) * (float) cos(rad));
		vertices.push_back((i/maxradius) * (float) sin(rad));

		colours.push_back(red);
		colours.push_back(green);
		colours.push_back(blue);

		red -= 1.f/360.f;
		green += 1.f/360.f;
		blue += .5f/360.f;
	}
	geometry->elementCount = vertices.size() / 2;
	geometry->renderMode = GL_LINE_STRIP;
}


void Sierpinski(vector<float>& vertices, vector<float>& colours, Triangle& triangle_prev, int recursions) {

	// Push vertices and colours to buffer
	if (recursions == 1) {
		vertices.push_back(triangle_prev.a.x);
		vertices.push_back(triangle_prev.a.y);

		vertices.push_back(triangle_prev.b.x);
		vertices.push_back(triangle_prev.b.y);

		vertices.push_back(triangle_prev.c.x);
		vertices.push_back(triangle_prev.c.y);

		colours.push_back(1);
		colours.push_back(0);
		colours.push_back(0);

		colours.push_back(1);
		colours.push_back(0);
		colours.push_back(0);

		colours.push_back(1);
		colours.push_back(0);
		colours.push_back(0);
		return;
	}

	Triangle triangle_a;
	Triangle triangle_b;
	Triangle triangle_c;
	Triangle triangle_center;

	// Determine points for helper center triangle
	triangle_center.a.x = triangle_prev.a.x+(triangle_prev.width/2);	
	triangle_center.a.y = triangle_prev.a.y;

	triangle_center.b.x = triangle_prev.a.x+(triangle_prev.width*0.75f);
	triangle_center.b.y = triangle_prev.a.y+(sqrt(3)*triangle_prev.width/4);

	triangle_center.c.x = triangle_prev.a.x+triangle_prev.width*0.25f;
	triangle_center.c.y = triangle_prev.a.y+(sqrt(3)*triangle_prev.width/4);

	// Perform next level of recursion
	triangle_a.a = triangle_prev.a;	
	triangle_a.b = triangle_center.a;
	triangle_a.c = triangle_center.c;
	triangle_a.width = triangle_prev.width/2;
	
	triangle_b.a = triangle_center.a;	
	triangle_b.b = triangle_prev.b;
	triangle_b.c = triangle_center.b;
	triangle_b.width = triangle_prev.width/2;
	
	triangle_c.a = triangle_center.c;	
	triangle_c.b = triangle_center.b;
	triangle_c.c = triangle_prev.c;
	triangle_c.width = triangle_prev.width/2;

	Sierpinski(vertices, colours, triangle_a, recursions-1);
	Sierpinski(vertices, colours, triangle_b, recursions-1);
	Sierpinski(vertices, colours, triangle_c, recursions-1);
}

void GenerateTriangles(MyGeometry *geometry, vector<float>& vertices, vector<float>& colours, int layer) 
{
	Triangle triangle;

	triangle.a.x = -0.9f;
	triangle.a.y = -0.9f;

	triangle.b.x = 0.9f;
	triangle.b.y = -0.9f;

	triangle.c.x = 0;
	triangle.c.y = -0.9f+(1.8f*sqrt(3)/2);

	triangle.width = 1.8f;

	Sierpinski(vertices, colours, triangle, layer);

	geometry->elementCount = vertices.size() / 2;
	geometry->renderMode = GL_TRIANGLES;
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

void RenderScene(MyGeometry *geometry, MyShader *shader)
{
	// clear screen to a dark grey colour
	glClearColor(0.2, 0.2, 0.2, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(geometry->renderMode, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
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

void UpdateDisplay() {
	vector<float> vertices;
	vector<float> colours;

	switch(currentstate.shape) {
		case SQUARES_DIAMONDS:
			GenerateSquaresDiamonds(&globalGeo, vertices, colours, currentstate.layer);
			break;
		case SPIRAL:
			GenerateSpiral(&globalGeo, vertices, colours, currentstate.layer);
			break;
		case TRIANGLES:
			GenerateTriangles(&globalGeo, vertices, colours, currentstate.layer);
			break;
	}

	if (!InitializeGeometry(&globalGeo, vertices, colours))
		cout << "Program failed to intialize geometry!" << endl;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	bool updateDisplay = false;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	} else if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		if (currentstate.layer > 1) {
			currentstate.layer--;
			updateDisplay = true;
		}
	} else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		if (currentstate.layer < 7) {
			currentstate.layer++;
			updateDisplay = true;
		}
	} else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		currentstate.shape = SQUARES_DIAMONDS;
		currentstate.layer = 1;
		updateDisplay = true;
	} else if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		currentstate.shape = SPIRAL;
		currentstate.layer = 1;
		updateDisplay = true;
	} else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		currentstate.shape = TRIANGLES;
		currentstate.layer = 1;
		updateDisplay = true;
	}

	if(updateDisplay) {
		UpdateDisplay();
	}
}
// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{   
	currentstate.layer = 1;
	currentstate.shape = SQUARES_DIAMONDS;
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initilize, TERMINATING" << endl;
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

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	MyShader shader;
	if (!InitializeShaders(&shader)) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}

	// call function to create and fill buffers with geometry data
	/* MyGeometry geometry; */
	/* if (!InitializeGeometry(&geometry, 1)) */
	/*     cout << "Program failed to intialize geometry!" << endl; */

	// run an event-triggered main loop
	UpdateDisplay();

	while (!glfwWindowShouldClose(window))
	{
		// call function to draw our scene
		RenderScene(&globalGeo, &shader);
		/* RenderScene(&geometry2, &shader); */
		/* RenderScene(&geometry2, &shader); */

		// scene is rendered to the back buffer, so swap to front for display
		glfwSwapBuffers(window);

		// sleep until next event before drawing again
		glfwWaitEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&globalGeo);
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
	string version  = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver  = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
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
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
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


// ==========================================================================
