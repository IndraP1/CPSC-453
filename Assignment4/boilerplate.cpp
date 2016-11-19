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
#include <math.h>
#include <glm/glm.hpp>
#include "ImageBuffer.h"

// Specify that we want the OpenGL core profile before including GLFW headers
#ifndef LAB_LINUX
	#include <glad/glad.h>
#else
	#define GLFW_INCLUDE_GLCOREARB
	#define GL_GLEXT_PROTOTYPES
#endif
#include <GLFW/glfw3.h>

using namespace glm;
using namespace std;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();
ImageBuffer img;

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

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

struct coord3D {
	float x;
	float y;
	float z;
};

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

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry)
{
	// three vertex positions and assocated colours of a triangle
	const GLfloat vertices[][2] = {
		{ -.6f, -.4f },
                { 0, .6f },
		{ .6f, -.4f }
	};

	const GLfloat colours[][3] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f }
	};
	geometry->elementCount = 3;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

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

void RenderScene(MyGeometry *geometry, MyShader *shader)
{
	// clear screen to a dark grey colour
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_TRIANGLES, 0, geometry->elementCount);

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
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

struct Plane {
	vec3 p;
	vec3 n;

	vec3 intersect;
	float intmag;
};

struct Light {
	vec3 p;
	vec3 r;
	float intensity;
};

struct Sphere {
	vec3 c;
	float r;

	vec3 n;
	vec3 intersect;
	float intmag;
};

struct Triangle {
	Plane pl;
	vec3 p0;
	vec3 p1;
	vec3 p2;
	vec3 px;

	float a;
	float a0;
	float a1;
	float a2;

	float u;
	float v;
	float w;

	vec3 intersect;
	float intmag;
};

bool intersectSphere(Sphere& cr, vec3& d, vec3& o) {
	float A = dot(d, d);
	float B = 2*dot(d, (o-cr.c));
	float C = (dot((o-cr.c), (o-cr.c))-pow(cr.r,2));
	
	float quad = pow(B,2)-(4*A*C);
	if (quad < 0) {
		return false;
	}
	float t = (-B + sqrt(quad))/(2*A);
	cr.intersect = o + t*d;
	cr.n = cr.intersect = cr.c;
	cr.intmag = length(cr.intersect);
	
	return true;
}

void initializeTrianglePlane(Triangle& t) {
	vec3 u(t.p1 - t.p0);
	vec3 v(t.p2 - t.p0);

	t.pl.n.x = dot(u.y, v.z)-dot(u.z, v.y);
	t.pl.n.y = dot(u.z, v.x)-dot(u.x, v.z);
	t.pl.n.z = dot(u.x, v.y)-dot(u.y, v.x);
	t.pl.p = t.p0;
}

float intersectPlane(Plane& pl, vec3& d, vec3& o) {
	float t1 = dot((pl.p-o), pl.n);
	float t2 = dot(d, pl.n);

	if (t2 == 0)
		return 0;

	pl.intersect = o + (t1/t2)*d;
	pl.intmag = length(pl.intersect);
	return t1/t2;
}

void initializeTriangle(Triangle& t) {
	vec3 p01(t.p1-t.p0);
	vec3 p02(t.p2-t.p0);
	vec3 p0x(t.px-t.p0);
	vec3 p12(t.p2-t.p1);
	vec3 p1x(t.px-t.p1);
	vec3 p20(t.p0-t.p2);
	vec3 p2x(t.px-t.p2);
	
	vec3 tmp(cross(p01, p02));
	t.a = tmp.x+tmp.y+tmp.z;

	vec3 tmp2(cross(p12, p1x));
	t.a0 = tmp2.x+tmp2.y+tmp2.z;

	vec3 tmp3(cross(p20, p2x));
	t.a1 = tmp3.x+tmp3.y+tmp3.z;

	vec3 tmp4(cross(p01, p0x));
	t.a2 = tmp4.x+tmp4.y+tmp4.z;

	t.v = t.a2/t.a;
	t.u = t.a1/t.a;
	t.w = t.a0/t.a;
}


bool intersectTriangle(Triangle& tr, vec3&d, vec3& o) {
	initializeTrianglePlane(tr);
	float t = intersectPlane(tr.pl, d, o);
	if (t == 0)
		return false;
	tr.px = {(o.x + t*d.x), (o.y + t*d.y), (o.z + t*d.z)};
	initializeTriangle(tr);
	if (tr.u*tr.v < 0 || tr.u*tr.w < 0 || tr.v*tr.w < 0)
		return false;

	tr.intersect = o + t*d; 
	tr.intmag = length(tr.intersect);
	return true;
}

void renderShapes() {
	coord3D ray;
	vec3 origin(0, 0, 0);

	Sphere cr;
	cr.c = {0.9, -1.925, -6.69};
	cr.r = 0.825;

	// Light 
	Light l;
	l.p = {0, 2.5, -7.75};

	// Plane 
	Plane pl;
	pl.p = {0, 0, -10.5};
	pl.n = {0, 0, 1};

	// Blue triangle
	Triangle bt1;
	bt1.p0 = {-0.4, -2.75, -9.55};
	bt1.p1 = {-0.93, 0.55, -8.51};
	bt1.p2 = {0.11, -2.75, -7.98};

	Triangle bt2;
	bt2.p0 = { 0.11, -2.75, -7.98};
	bt2.p1 = {-0.93, 0.55, -8.51};
	bt2.p2 = {-1.46, -2.75, -7.47};

	Triangle bt3;
	bt3.p0 = {-1.46, -2.75, -7.47};
	bt3.p1 = {-0.93, 0.55, -8.51};
	bt3.p2 = {-1.97, -2.75, -9.04};

	Triangle bt4;
	bt4.p0 = {-1.97, -2.75, -9.04};
	bt4.p1 = {-0.93, 0.55, -8.51};
	bt4.p2 = {-0.4, -2.75, -9.55};
	Triangle bluepyramid[4] = {bt1, bt2, bt3, bt4};

	// Ceiling
	Triangle c1;
	c1.p0 = { 2.75, 2.75, -10.5};
	c1.p1 = { 2.75, 2.75, -5};
	c1.p2 = { -2.75, 2.75, -5};

	Triangle c2;
	c2.p0 = { -2.75, 2.75, -10.5};
	c2.p1 = { 2.75, 2.75, -10.5};
	c2.p2 = { -2.75, 2.75, -5};
	Triangle ceiling[2] = {c1, c2};

	/* Green wall on right */ 
	Triangle gw1;
	gw1.p0 = {2.75, 2.75, -5};
	gw1.p1 = {2.75, 2.75, -10.5};
	gw1.p2 = {2.75, -2.75, -10.5};

	Triangle gw2;
	gw2.p0 = {2.75, -2.75, -5};
	gw2.p1 = {2.75, 2.75, -5};
	gw2.p2 = {2.75, -2.75, -10.5};
	Triangle greenwall[2] = {gw1, gw2};

	/* Red wall on left */
	Triangle rw1;
	rw1.p0 = {-2.75, -2.75, -5};
	rw1.p1 = {-2.75, -2.75, -10.5};
	rw1.p2 = {-2.75, 2.75, -10.5};

	Triangle rw2;
	rw2.p0 = {-2.75, 2.75, -5};
	rw2.p1 = {-2.75, -2.75, -5};
	rw2.p2 = {-2.75, 2.75, -10.5};
	Triangle redwall[2] = {rw1, rw2};

	/* Floor */
	Triangle f1;
	f1.p0 = {2.75, -2.75, -5};
	f1.p1 = {2.75, -2.75, -10.5};
	f1.p2 = {-2.75, -2.75, -10.5};

	Triangle f2;
	f2.p0 = {-2.75, -2.75, -5};
	f2.p1 = {2.75, -2.75, -5};
	f2.p2 = {-2.75, -2.75, -10.5};

	Triangle floor[2] = {f1, f2};
	ray.z = -950;// tan(120)*1024

	for (int x = 0; x < img.Width(); x++) {
		for (int y = 0; y < img.Height(); y++) {
			float closest_mag = 200;
			ray.x = -1*(img.Width()/2.f - 0.5f)+x;
			ray.y = -1*(img.Height()/2.f - 0.5f)+y;
			vec3 colour(0, 0, 0);
			vec3 d(ray.x, ray.y, ray.z);

			if (intersectPlane(pl, d, origin) > 0.f) {
				if(pl.intmag < closest_mag) {
					closest_mag = pl.intmag;
					l.intensity = dot(normalize(pl.n), (normalize(l.p-pl.intersect)));
					colour.x = 1*l.intensity;
					colour.y = 1*l.intensity;
					colour.z = 1*l.intensity;
				}
			}
			for (uint i = 0; i < sizeof(bluepyramid)/sizeof(Triangle); i++) {
				if (intersectTriangle(bluepyramid[i], d, origin) > 0.f) {
					if(bluepyramid[i].intmag < closest_mag) {
						closest_mag = bluepyramid[i].intmag;
						l.intensity = dot(normalize(bluepyramid[i].pl.n), (normalize(l.p-bluepyramid[i].intersect)));
						colour.x = 0;
						colour.y = 0;
						colour.z = 1*l.intensity;
					}
				}
			}
			for (uint i = 0; i < sizeof(floor)/sizeof(Triangle); i++) {
				if (intersectTriangle(floor[i], d, origin) > 0.f) {
					if(floor[i].intmag < closest_mag) {
						closest_mag = floor[i].intmag;
						l.intensity = dot(normalize(floor[i].pl.n), (normalize(l.p-floor[i].intersect)));
						colour.x = 0.3*l.intensity;
						colour.y = 0.3*l.intensity;
						colour.z = 0.3*l.intensity;
					}
				}
			}

			for (uint i = 0; i < sizeof(ceiling)/sizeof(Triangle); i++) {
				if (intersectTriangle(ceiling[i], d, origin) > 0.f) {
					if(ceiling[i].intmag < closest_mag) {
						closest_mag = ceiling[i].intmag;
						l.intensity = dot(normalize(ceiling[i].pl.n), (normalize(l.p-ceiling[i].intersect)));
						colour.x = 0.3*l.intensity;
						colour.y = 0.3*l.intensity;
						colour.z = 0.3*l.intensity;
					}
				}
			}

			for (uint i = 0; i < sizeof(greenwall)/sizeof(Triangle); i++) {
				if (intersectTriangle(greenwall[i], d, origin)) {
					if(greenwall[i].intmag < closest_mag) {
						closest_mag = greenwall[i].intmag;
						l.intensity = dot(normalize(greenwall[i].pl.n), (normalize(l.p-greenwall[i].intersect)));
						colour.x = 0;
						colour.y = 1*l.intensity;
						colour.z = 0;
					}
				}
			}

			for (uint i = 0; i < sizeof(redwall)/sizeof(Triangle); i++) {
				if (intersectTriangle(redwall[i], d, origin) > 0.f) {
					if(redwall[i].intmag < closest_mag) {
						closest_mag = redwall[i].intmag;
						l.intensity = dot(normalize(redwall[i].pl.n), (normalize(l.p-redwall[i].intersect)));
						colour.x = 1*l.intensity;
						colour.y = 0;
						colour.z = 0;
					}
				}
			}
			if (intersectSphere(cr, d, origin)) {
				if(cr.intmag < closest_mag) {
					closest_mag = cr.intmag;
					l.intensity = dot(normalize(cr.n), (normalize(l.p-cr.intersect)));
					colour.x = 0.5*l.intensity;
					colour.y = 0.5*l.intensity;
					colour.z = 0.5*l.intensity;
					cout << l.intensity << endl;
				}
			}
			img.SetPixel(x, y, colour);
		}
	}
}
// print a vec3
void printVec3(const char* prefix, const vec3& v)
{
    cout << prefix << " (" << v.x << ", " << v.y << ", " << v.z << ")" << endl;
}

// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
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
	window = glfwCreateWindow(1024, 1024, "CPSC 453 OpenGL Boilerplate", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwMakeContextCurrent(window);

	//Intialize GLAD
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
	if (!img.Initialize()) {
		cout << "ImageBuffer could not be initialized, TERMINATING" << endl;
		return -1;
	}

	// call function to create and fill buffers with geometry data
	MyGeometry geometry;
	if (!InitializeGeometry(&geometry))
		cout << "Program failed to intialize geometry!" << endl;
	renderShapes();
	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		img.Render();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&geometry);
	DestroyShaders(&shader);
	img.Destroy();
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
