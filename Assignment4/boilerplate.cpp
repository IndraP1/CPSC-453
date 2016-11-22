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
	vec3 colour;

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
	vec3 colour;

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
	vec3 colour;

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
	cr.n = cr.intersect - cr.c;
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


bool intersectTriangle(Triangle& tr, vec3& d, vec3& o) {
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

// print a vec3
void printVec3(const char* prefix, const vec3& v)
{
    cout << prefix << " (" << v.x << ", " << v.y << ", " << v.z << ")" << endl;
}

void shading(vec3& colour, vec3& n, Light& lightpoint, vec3& intersect, vec3& d) {
	float p = 256;
	float cl = 1;
	float ca = 0.2;
	vec3 l(lightpoint.p - intersect);
	vec3 origin(0, 0, 0);
	lightpoint.intensity = ca + cl*dot(normalize(n), (normalize(l)));

	vec3 d_hat(normalize(d));
	vec3 n_hat(normalize(n));
	vec3 l_hat(normalize(l));
	vec3 ref(reflect(l_hat, n_hat));
	/* printVec3("h", h); */
	
	vec3 cp(colour);
	colour.x *= lightpoint.intensity;  
	if(dot(d_hat, ref) < 0) {
		colour.x += cl*cp.x*pow(dot(d_hat,ref), p);
	}
	colour.y *= lightpoint.intensity;
	if(dot(d_hat, ref) < 0) {
		colour.y += cl*cp.y*pow(dot(d_hat,ref), p);
	}
	colour.z *= lightpoint.intensity;
	if(dot(d_hat, ref) < 0) {
		colour.z += cl*cp.z*pow(dot(d_hat,ref), p);
	}
}

void checkShadow1(vec3& intercept, vec3& colours, Light& lightpoint, Triangle bluepyramid[], Sphere& sphere) {
	vec3 l(lightpoint.p - intercept);

	for (uint i = 0; i <= 4; i++) {
		if (intersectTriangle(bluepyramid[i], l, intercept)) {
			colours.x = 0.1;
			colours.y = 0.1;
			colours.z = 0.1;
		}
	}	

	if (intersectSphere(sphere, l, intercept)) {
		colours.x = 0.1;
		colours.y = 0.1;
		colours.z = 0.1;
	}
}

void checkShadow2(vec3& intercept, vec3& colours, Light& lightpoint, Triangle greencone[], Triangle rediso[], Sphere& yellow_sp, Sphere& grey_sp,
		Sphere& purp_sp) {
	vec3 l(lightpoint.p - intercept);

	for (uint i = 0; i <= 12; i++) {
		if (intersectTriangle(greencone[i], l, intercept)) {
			colours.x = 0.1;
			colours.y = 0.1;
			colours.z = 0.1;
		}
	}	

	for (uint i = 0; i <= 20; i++) {
		if (intersectTriangle(rediso[i], l, intercept)) {
			colours.x = 0.1;
			colours.y = 0.1;
			colours.z = 0.1;
		}
	}	

	if (intersectSphere(grey_sp, l, intercept)) {
		colours.x = 0.1;
		colours.y = 0.1;
		colours.z = 0.1;
	}
	if (intersectSphere(yellow_sp, l, intercept)) {
		colours.x = 0.1;
		colours.y = 0.1;
		colours.z = 0.1;
	}
	if (intersectSphere(purp_sp, l, intercept)) {
		colours.x = 0.1;
		colours.y = 0.1;
		colours.z = 0.1;
	}
}

void scene_reflect1(int obj, vec3& d, vec3& n, vec3& intersect, vec3& colour, Sphere& cr, Plane& pl, Triangle bluepyramid[], Triangle ceiling[],
		Triangle redwall[], Triangle greenwall[], Triangle floor[]) {
	vec3 r(reflect(d, n));	
	for (uint i = 0; i <= 2; i++) {
		if (intersectTriangle(greenwall[i], r, intersect)) {
			colour.x = greenwall[i].colour.x;
			colour.y = greenwall[i].colour.y;
			colour.z = greenwall[i].colour.z;
		}
	}	
	for (uint i = 0; i <= 2; i++) {
		if (intersectTriangle(redwall[i], r, intersect)) {
			colour.x = redwall[i].colour.x;
			colour.y = redwall[i].colour.y;
			colour.z = redwall[i].colour.z;
		}
	}	
	for (uint i = 0; i <= 2; i++) {
		if (intersectTriangle(floor[i], r, intersect)) {
			colour.x = floor[i].colour.x;
			colour.y = floor[i].colour.y;
			colour.z = floor[i].colour.z;
		}
	}	
	
	if(obj == 2) {
		for (uint i = 0; i <= 4; i++) {
			if (intersectTriangle(bluepyramid[i], r, intersect)) {
				colour.x = bluepyramid[i].colour.x;
				colour.y = bluepyramid[i].colour.y;
				colour.z = bluepyramid[i].colour.z;
			}
		}	
	}

	if (obj == 1) {
		if (intersectSphere(cr, r, intersect)) {
			colour.x = cr.colour.x;
			colour.y = cr.colour.y;
			colour.z = cr.colour.z;
		}
	}
}

void scene_reflect2(int obj, vec3& d, vec3& n, vec3& intersect, vec3& colour, Sphere& yellow_sp, Sphere& purple_sp, Sphere& grey_sp, Triangle greencone[], Triangle rediso[]) {
	vec3 r(reflect(d, n));	

	for (uint i = 0; i <= 12; i++) {
		if (intersectTriangle(greencone[i], r, intersect)) {
			colour.x = greencone[i].colour.x;
			colour.y = greencone[i].colour.y;
			colour.z = greencone[i].colour.z;
		}
	}	

	if (intersectSphere(yellow_sp, r, intersect)) {
		colour.x = yellow_sp.colour.x;
		colour.y = yellow_sp.colour.y;
		colour.z = yellow_sp.colour.z;
	}

	if (intersectSphere(yellow_sp, r, intersect)) {
		colour.x = yellow_sp.colour.x;
		colour.y = yellow_sp.colour.y;
		colour.z = yellow_sp.colour.z;
	}

	if (obj != 1) {
		if (intersectSphere(purple_sp, r, intersect)) {
			colour.x = purple_sp.colour.x;
			colour.y = purple_sp.colour.y;
			colour.z = purple_sp.colour.z;
		}
	}
	if (obj != 2) {
		if (intersectSphere(grey_sp, r, intersect)) {
			colour.x = grey_sp.colour.x;
			colour.y = grey_sp.colour.y;
			colour.z = grey_sp.colour.z;
		}
	}
	if(obj != 3) {
		for (uint i = 0; i <= 20; i++) {
			if (intersectTriangle(rediso[i], r, intersect)) {
				colour.x = rediso[i].colour.x;
				colour.y = rediso[i].colour.y;
				colour.z = rediso[i].colour.z;
			}
		}	
	}
}

void renderShapes() {
	coord3D ray;
	vec3 origin(0, 0, 0);

	Sphere cr;
	cr.c = {0.9, -1.925, -6.69};
	cr.r = 0.825;
	cr.colour = {0.5, 0.5, 0.5};

	// Light 
	Light l;
	l.p = {0, 2.5, -7.75};

	// Plane 
	Plane pl;
	pl.p = {0, 0, -10.5};
	pl.n = {0, 0, 1};
	pl.colour = {0.5, 0.5, 0.5};

	// Blue triangle
	Triangle bt1;
	bt1.p0 = {-0.4, -2.75, -9.55};
	bt1.p1 = {-0.93, 0.55, -8.51};
	bt1.p2 = {0.11, -2.75, -7.98};
	bt1.colour = {0, 0, 0.7};

	Triangle bt2;
	bt2.p0 = { 0.11, -2.75, -7.98};
	bt2.p1 = {-0.93, 0.55, -8.51};
	bt2.p2 = {-1.46, -2.75, -7.47};
	bt2.colour = {0, 0, 0.7};

	Triangle bt3;
	bt3.p0 = {-1.46, -2.75, -7.47};
	bt3.p1 = {-0.93, 0.55, -8.51};
	bt3.p2 = {-1.97, -2.75, -9.04};
	bt3.colour = {0, 0, 0.7};

	Triangle bt4;
	bt4.p0 = {-1.97, -2.75, -9.04};
	bt4.p1 = {-0.93, 0.55, -8.51};
	bt4.p2 = {-0.4, -2.75, -9.55};
	bt4.colour = {0, 0, 0.7};
	Triangle bluepyramid[4] = {bt1, bt2, bt3, bt4};

	// Ceiling
	Triangle c1;
	c1.p0 = { 2.75, 2.75, -10.5};
	c1.p1 = { 2.75, 2.75, -5};
	c1.p2 = { -2.75, 2.75, -5};
	c1.colour = {0.3, 0.3, 0.3};

	Triangle c2;
	c2.p0 = { -2.75, 2.75, -10.5};
	c2.p1 = { 2.75, 2.75, -10.5};
	c2.p2 = { -2.75, 2.75, -5};
	c2.colour = {0.3, 0.3, 0.3};
	Triangle ceiling[2] = {c1, c2};

	/* Green wall on right */ 
	Triangle gw1;
	gw1.p0 = {2.75, 2.75, -5};
	gw1.p1 = {2.75, 2.75, -10.5};
	gw1.p2 = {2.75, -2.75, -10.5};
	gw1.colour = {0, 0.5, 0};

	Triangle gw2;
	gw2.p0 = {2.75, -2.75, -5};
	gw2.p1 = {2.75, 2.75, -5};
	gw2.p2 = {2.75, -2.75, -10.5};
	gw2.colour = {0, 0.5, 0};
	Triangle greenwall[2] = {gw1, gw2};

	/* Red wall on left */
	Triangle rw1;
	rw1.p0 = {-2.75, -2.75, -5};
	rw1.p1 = {-2.75, -2.75, -10.5};
	rw1.p2 = {-2.75, 2.75, -10.5};
	rw1.colour = {0.5, 0, 0};

	Triangle rw2;
	rw2.p0 = {-2.75, 2.75, -5};
	rw2.p1 = {-2.75, -2.75, -5};
	rw2.p2 = {-2.75, 2.75, -10.5};
	rw2.colour = {0.5, 0, 0};
	Triangle redwall[2] = {rw1, rw2};

	/* Floor */
	Triangle f1;
	f1.p0 = {2.75, -2.75, -5};
	f1.p1 = {2.75, -2.75, -10.5};
	f1.p2 = {-2.75, -2.75, -10.5};
	f1.colour = {0.3, 0.3, 0.3};

	Triangle f2;
	f2.p0 = {-2.75, -2.75, -5};
	f2.p1 = {2.75, -2.75, -5};
	f2.p2 = {-2.75, -2.75, -10.5};
	f2.colour = {0.3, 0.3, 0.3};

	Triangle floor[2] = {f1, f2};
	ray.z = -500;

	for (int x = 0; x < img.Width(); x++) {
		for (int y = 0; y < img.Height(); y++) {
			float closest_mag = 200;
			ray.x = -1*(img.Width()/2.f - 0.5f)+x;
			ray.y = -1*(img.Height()/2.f - 0.5f)+y;
			vec3 colour(0, 0, 0);
			vec3 d(ray.x, ray.y, ray.z);

			if (intersectPlane(pl, d, origin) != 0.f) {
				if(pl.intmag < closest_mag) {
					closest_mag = pl.intmag;
					colour.x = 0.5;
					colour.y = 0.5;
					colour.z = 0.5;
					shading(colour, pl.n, l, pl.intersect, d);
				}
			}
			for (uint i = 0; i < sizeof(bluepyramid)/sizeof(Triangle); i++) {
				if (intersectTriangle(bluepyramid[i], d, origin)) {
					if(bluepyramid[i].intmag < closest_mag) {
						closest_mag = bluepyramid[i].intmag;
						colour.x = 0;
						colour.y = 0;
						colour.z = 0.7;
						scene_reflect1(1, d, bluepyramid[i].pl.n, bluepyramid[i].intersect, colour, cr, pl, bluepyramid, ceiling, redwall, greenwall, floor);
						shading(colour, bluepyramid[i].pl.n, l, bluepyramid[i].intersect, d);
					}
				}
			}
			for (uint i = 0; i < sizeof(floor)/sizeof(Triangle); i++) {
				if (intersectTriangle(floor[i], d, origin)) {
					if(floor[i].intmag < closest_mag) {
						closest_mag = floor[i].intmag;
						colour.x = 0.3;
						colour.y = 0.3;
						colour.z = 0.3;
						shading(colour, floor[i].pl.n, l, floor[i].intersect, d);
						checkShadow1(floor[i].intersect, colour, l, bluepyramid, cr);
					}
				}
			}

			for (uint i = 0; i < sizeof(ceiling)/sizeof(Triangle); i++) {
				if (intersectTriangle(ceiling[i], d, origin)) {
					if(ceiling[i].intmag < closest_mag) {
						closest_mag = ceiling[i].intmag;
						colour.x = 0.3;
						colour.y = 0.3;
						colour.z = 0.3;
						shading(colour, ceiling[i].pl.n, l, ceiling[i].intersect, d);
					}
				}
			}

			for (uint i = 0; i < sizeof(greenwall)/sizeof(Triangle); i++) {
				if (intersectTriangle(greenwall[i], d, origin)) {
					if(greenwall[i].intmag < closest_mag) {
						closest_mag = greenwall[i].intmag;
						colour.x = 0;
						colour.y = 0.5;
						colour.z = 0;
						shading(colour, greenwall[i].pl.n, l, greenwall[i].intersect, d);
					}
				}
			}

			for (uint i = 0; i < sizeof(redwall)/sizeof(Triangle); i++) {
				if (intersectTriangle(redwall[i], d, origin)) {
					if(redwall[i].intmag < closest_mag) {
						closest_mag = redwall[i].intmag;
						colour.x = 0.5;
						colour.y = 0;
						colour.z = 0;
						shading(colour, redwall[i].pl.n, l, redwall[i].intersect, d);
					}
				}
			}
			if (intersectSphere(cr, d, origin)) {
				if(cr.intmag < closest_mag) {
					closest_mag = cr.intmag;
					colour.x = 0.5;
					colour.y = 0.5;
					colour.z = 0.5;
					scene_reflect1(2, d, cr.n, cr.intersect, colour, cr, pl, bluepyramid, ceiling, redwall, greenwall, floor);
					shading(colour, cr.n, l, cr.intersect, d);
				}
			}
			img.SetPixel(x, y, colour);
		}
	}
}

void renderShapes2() {
	coord3D ray;
	ray.z = -500;
	vec3 origin(0, 0, 0);

	Light l;
	l.p = {4, 6, 1};

	Plane pl;
	pl.p = {0, -1, 0};
	pl.n = {0, 1, 0};
	pl.colour = {0.5, 0.5, 0.5};

	Plane pl2;
	pl2.p = {0, 0, -12};
	pl2.n = {0, 0, 1};
	pl2.colour = {0.5, 0.3, 0};

	/* # Large yellow sphere */
	Sphere yellow_sp;
	yellow_sp.c = {1, -0.5, -3.5};
	yellow_sp.r = 0.5;
	yellow_sp.colour = {0.5, 0.5, 0};

	/* # Reflective grey sphere */
	Sphere grey_sp;
	grey_sp.c = {0, 1, -5};
	grey_sp.r = 0.4;
	grey_sp.colour = {0.5, 0.5, 0.5};

	/* # Metallic purple sphere */
	Sphere purp_sp;
	purp_sp.c = {-0.8, -0.75, -4};
	purp_sp.r = 0.25;
	purp_sp.colour = {0.5, 0, 0.5};

	/* # Green cone */
	Triangle gc1;
	gc1.p0 = {0, -1, -5.8};
	gc1.p1 = {0, 0.6, -5};
	gc1.p2 = {0.4, -1, -5.693};
	gc1.colour = {0, 0.7, 0};

	Triangle gc2;
	gc2.p0 = {0.4, -1, -5.693};
	gc2.p1 = {0, 0.6, -5};
	gc2.p2 = {0.6928, -1, -5.4};
	gc2.colour = {0, 0.7, 0};

	Triangle gc3;
	gc3.p0 = {0.6928, -1, -5.4};
	gc3.p1 = {0, 0.6, -5};
	gc3.p2 = {0.8, -1, -5};
	gc3.colour = {0, 0.7, 0};

	Triangle gc4;
	gc4.p0 = {0.8, -1, -5};
	gc4.p1 = {0, 0.6, -5};
	gc4.p2 = {0.6928, -1, -4.6};
	gc3.colour = {0, 0.7, 0};

	Triangle gc5;
	gc5.p0 = {0.6928, -1, -4.6};
	gc5.p1 = {0, 0.6, -5};
	gc5.p2 = {0.4, -1, -4.307};
	gc5.colour = {0, 0.7, 0};

	Triangle gc6;
	gc6.p0 = {0.4, -1, -4.307};
	gc6.p1 = {0, 0.6, -5};
	gc6.p2 = {0, -1, -4.2};
	gc6.colour = {0, 0.7, 0};

	Triangle gc7;
	gc7.p0 = {0, -1, -4.2};
	gc7.p1 = {0, 0.6, -5};
	gc7.p2 = {-0.4, -1, -4.307};
	gc7.colour = {0, 0.7, 0};

	Triangle gc8;
	gc8.p0 = {-0.4, -1, -4.307};
	gc8.p1 = {0, 0.6, -5};
	gc8.p2 = {-0.6928, -1, -4.6};
	gc8.colour = {0, 0.7, 0};

	Triangle gc9;
	gc9.p0 = {-0.6928, -1, -4.6};
	gc9.p1 = {0, 0.6, -5};
	gc9.p2 = {-0.8, -1, -5};
	gc9.colour = {0, 0.7, 0};

	Triangle gc10;
	gc10.p0 = {-0.8, -1, -5};
	gc10.p1 = {0, 0.6, -5};
	gc10.p2 = {-0.6928, -1, -5.4};
	gc10.colour = {0, 0.7, 0};

	Triangle gc11;
	gc11.p0 = {-0.6928, -1, -5.4};
	gc11.p1 = {0, 0.6, -5};
	gc11.p2 = {-0.4, -1, -5.693};
	gc11.colour = {0, 0.7, 0};

	Triangle gc12;
	gc12.p0 = {-0.4, -1, -5.693};
	gc12.p1 = {0, 0.6, -5};
	gc12.p2 = {0, -1, -5.8}; 
	gc12.colour = {0, 0.7, 0};

	/* Shiny red icosahedron */
	Triangle ri1;	
	ri1.p0 = {-2, -1, -7};
	ri1.p1 = {-1.276, -0.4472, -6.474};
	ri1.p2 = {-2.276, -0.4472, -6.149};
	ri1.colour = {0.7, 0, 0};

	Triangle ri2;
	ri2.p0 = {-1.276, -0.4472, -6.474};
	ri2.p1 = {-2, -1, -7};
	ri2.p2 = {-1.276, -0.4472, -7.526};
	ri2.colour = {0.7, 0, 0};

	Triangle ri3;
	ri3.p0 = {-2, -1, -7};
	ri3.p1 = {-2.276, -0.4472, -6.149};
	ri3.p2 = {-2.894, -0.4472, -7};
	ri3.colour = {0.7, 0, 0};

	Triangle ri4;
	ri4.p0 = {-2, -1, -7};
	ri4.p1 = {-2.894, -0.4472, -7};
	ri4.p2 = {-2.276, -0.4472, -7.851};
	ri4.colour = {0.7, 0, 0};

	Triangle ri5;
	ri5.p0 = {-2, -1, -7};
	ri5.p1 = {-2.276, -0.4472, -7.851};
	ri5.p2 = {-1.276, -0.4472, -7.526};
	ri5.colour = {0.7, 0, 0};

	Triangle ri6;
	ri6.p0 = {-1.276, -0.4472, -6.474};
	ri6.p1 = {-1.276, -0.4472, -7.526};
	ri6.p2 = {-1.106, 0.4472, -7};
	ri6.colour = {0.7, 0, 0};

	Triangle ri7;
	ri7.p0 = {-2.276, -0.4472, -6.149};
	ri7.p1 = {-1.276, -0.4472, -6.474};
	ri7.p2 = {-1.724, 0.4472, -6.149};
	ri7.colour = {0.7, 0, 0};

	Triangle ri8;
	ri8.p0 = {-2.894, -0.4472, -7};
	ri8.p1 = {-2.276, -0.4472, -6.149};
	ri8.p2 = {-2.724, 0.4472, -6.474};
	ri8.colour = {0.7, 0, 0};

	Triangle ri9;
	ri9.p0 = {-2.276, -0.4472, -7.851};
	ri9.p1 = {-2.894, -0.4472, -7};
	ri9.p2 = {-2.724, 0.4472, -7.526};
	ri9.colour = {0.7, 0, 0};

	Triangle ri10;
	ri10.p0 = {-1.276, -0.4472, -7.526};
	ri10.p1 = {-2.276, -0.4472, -7.851};
	ri10.p2 = {-1.724, 0.4472, -7.851};
	ri10.colour = {0.7, 0, 0};

	Triangle ri11;
	ri11.p0 = {-1.276, -0.4472, -6.474};
	ri11.p1 = {-1.106, 0.4472, -7};
	ri11.p2 = {-1.724, 0.4472, -6.149};
	ri11.colour = {0.7, 0, 0};

	Triangle ri12;
	ri12.p0 = {-2.276, -0.4472, -6.149};
	ri12.p1 = {-1.724, 0.4472, -6.149};
	ri12.p2 = {-2.724, 0.4472, -6.474};
	ri12.colour = {0.7, 0, 0};

	Triangle ri13;
	ri13.p0 = {-2.894, -0.4472, -7};
	ri13.p1 = {-2.724, 0.4472, -6.474};
	ri13.p2 = {-2.724, 0.4472, -7.526};
	ri13.colour = {0.7, 0, 0};

	Triangle ri14;
	ri14.p0 = {-2.276, -0.4472, -7.851};
	ri14.p1 = {-2.724, 0.4472, -7.526};
	ri14.p2 = {-1.724, 0.4472, -7.851};
	ri14.colour = {0.7, 0, 0};

	Triangle ri15;
	ri15.p0 = {-1.276, -0.4472, -7.526};
	ri15.p1 = {-1.724, 0.4472, -7.851};
	ri15.p2 = {-1.106, 0.4472, -7};
	ri15.colour = {0.7, 0, 0};

	Triangle ri16;
	ri16.p0 = {-1.724, 0.4472, -6.149};
	ri16.p1 = {-1.106, 0.4472, -7};
	ri16.p2 = {-2, 1, -7};
	ri16.colour = {0.7, 0, 0};

	Triangle ri17;
	ri17.p0 = {-2.724, 0.4472, -6.474};
	ri17.p1 = {-1.724, 0.4472, -6.149};
	ri17.p2 = {-2, 1, -7};
	ri17.colour = {0.7, 0, 0};

	Triangle ri18;
	ri18.p0 = {-2.724, 0.4472, -7.526};
	ri18.p1 = {-2.724, 0.4472, -6.474};
	ri18.p2 = {-2, 1, -7};
	ri18.colour = {0.7, 0, 0};

	Triangle ri19;
	ri19.p0 = {-1.724, 0.4472, -7.851};
	ri19.p1 = {-2.724, 0.4472, -7.526};
	ri19.p2 = {-2, 1, -7};
	ri19.colour = {0.7, 0, 0};

	Triangle ri20;
	ri20.p0 = {-1.106, 0.4472, -7};
	ri20.p1 = {-1.724, 0.4472, -7.851};
	ri20.p2 = {-2, 1, -7};
	ri20.colour = {0.7, 0, 0};

	Triangle gc[12] = {gc1, gc2, gc3, gc4, gc5, gc6, gc7, gc8, gc9, gc10, gc11, gc12};
	Triangle ri[20] = {ri1, ri2, ri3, ri4, ri5, ri6, ri7, ri8, ri9, ri10, ri11, ri12,
		ri13, ri14, ri15, ri16, ri17, ri18, ri19, ri20};

	for (int x = 0; x < img.Width(); x++) {
		for (int y = 0; y < img.Height(); y++) {
			float closest_mag = 200;
			ray.x = -1*(img.Width()/2.f - 0.5f)+x;
			ray.y = -1*(img.Height()/2.f - 0.5f)+y;
			vec3 colour(0, 0, 0);
			vec3 d(ray.x, ray.y, ray.z);
			
			// Floor
			if (intersectPlane(pl, d, origin) != 0.f) {
				if(pl.intmag < closest_mag && y < img.Height()/2) {
					closest_mag = pl.intmag;
					colour.x = 0.5;
					colour.y = 0.5;
					colour.z = 0.5;
					shading(colour, pl.n, l, pl.intersect, d);
					checkShadow2(pl.intersect, colour, l, gc, ri, yellow_sp, grey_sp, purp_sp);
				}
			}
			// Wall
			if (intersectPlane(pl2, d, origin) != 0.f) {
				if(pl2.intmag < closest_mag) {
					closest_mag = pl2.intmag;
					colour.x = 0;
					colour.y = 0.5;
					colour.z = 0.5;
					shading(colour, pl2.n, l, pl2.intersect, d);
				}
			}
			for (uint i = 0; i < sizeof(gc)/sizeof(Triangle); i++) {
				if (intersectTriangle(gc[i], d, origin)) {
					if(gc[i].intmag < closest_mag) {
						closest_mag = gc[i].intmag;
						colour.x = 0;
						colour.y = 0.7;
						colour.z = 0;
						shading(colour, gc[i].pl.n, l, gc[i].intersect, d);
					}
				}
			}
			for (uint i = 0; i < sizeof(ri)/sizeof(Triangle); i++) {
				if (intersectTriangle(ri[i], d, origin)) {
					if(ri[i].intmag < closest_mag) {
						closest_mag = ri[i].intmag;
						colour.x = 0.7;
						colour.y = 0;
						colour.z = 0;
						scene_reflect2(3, d, ri[i].pl.n, ri[i].intersect, colour, yellow_sp, purp_sp, grey_sp, gc, ri);
						shading(colour, ri[i].pl.n, l, ri[i].intersect, d);
					}
				}
			}
			if (intersectSphere(yellow_sp, d, origin)) {
				if(yellow_sp.intmag < closest_mag) {
					closest_mag = yellow_sp.intmag;
					colour.x = 0.5;
					colour.y = 0.5;
					colour.z = 0;
					shading(colour, yellow_sp.n, l, yellow_sp.intersect, d);
				}
			}
			if (intersectSphere(grey_sp, d, origin)) {
				if(grey_sp.intmag < closest_mag) {
					closest_mag = grey_sp.intmag;
					colour.x = 0.5;
					colour.y = 0.5;
					colour.z = 0.5;
					scene_reflect2(2, d, grey_sp.n, grey_sp.intersect, colour, yellow_sp, purp_sp, grey_sp, gc, ri);
					shading(colour, grey_sp.n, l, grey_sp.intersect, d);
				}
			}
			if (intersectSphere(purp_sp, d, origin)) {
				if(purp_sp.intmag < closest_mag) {
					closest_mag = purp_sp.intmag;
					colour.x = 0.5;
					colour.y = 0;
					colour.z = 0.5;
					scene_reflect2(1, d, purp_sp.n, purp_sp.intersect, colour, yellow_sp, purp_sp, grey_sp, gc, ri);
					shading(colour, purp_sp.n, l, purp_sp.intersect, d);
				}
			}
			img.SetPixel(x, y, colour);
		}
	}
}
void renderShapes3() {
	coord3D ray;
	ray.z = -500;
	vec3 origin(0, 0, 0);

	// Plane 
	Plane pl;
	pl.p = {0, 0, -10.5};
	pl.n = {0, 0, 1};
	pl.colour = {0.5, 0.5, 0.5};
	// Light 
	Light l;
	l.p = {1, 1, -12};

	Sphere head;
	head.c = {0.8, -0.8, -3.5};
	head.r = 1;
	head.colour = {0, 0, 0.5};

	Sphere body;
	body.c = {1.1, -3.5, -3.5};
	body.r = 2;
	body.colour = {0, 0, 0.5};

	Sphere eye1;
	eye1.c = {0.9, 0.1, -3.1};
	eye1.r = 0.2;
	eye1.colour = {0.3, 0.3, 0.3};
	
	Sphere pupil1;
	pupil1.c = {0.78, 0.15, -2.8};
	pupil1.r = 0.08;
	pupil1.colour = {0, 0, 0};
	
	Sphere pupil2;
	pupil2.c = {0.5, 0.05, -2.9};
	pupil2.r = 0.08;
	pupil2.colour = {0, 0, 0};
	
	Sphere eye2;
	eye2.c = {0.5, 0.1, -3.1};
	eye2.r = 0.2;
	eye2.colour = {0.3, 0.3, 0.3};

	Triangle mouth;
	mouth.p0 = {-0.2, -0.8, -3.1};
	mouth.p1 = {1.8, -0.8, -3.1};
	mouth.p2 = {0.7, -1.4, -3.1};
	mouth.colour = {0, 0, 0};

	Triangle mouth2;
	mouth2.p0 = {-0.2, -0.8, -3.1};
	mouth2.p1 = {1.8, -0.8, -3.1};
	mouth2.p2 = {0.7, -0.4, -3.1};
	mouth.colour = {0, 0, 0};

	for (int x = 0; x < img.Width(); x++) {
		for (int y = 0; y < img.Height(); y++) {
			float closest_mag = 200;
			ray.x = -1*(img.Width()/2.f - 0.5f)+x;
			ray.y = -1*(img.Height()/2.f - 0.5f)+y;
			vec3 colour(0, 0, 0);
			vec3 d(ray.x, ray.y, ray.z);

			if (intersectPlane(pl, d, origin) != 0.f) {
				if(pl.intmag < closest_mag) {
					closest_mag = pl.intmag;
					colour.x = 0.5;
					colour.y = 0;
					colour.z = 0.5;
				}
			}
			if (intersectSphere(head, d, origin)) {
				if(head.intmag < closest_mag) {
					closest_mag = head.intmag;
					colour.x = 0;
					colour.y = 0;
					colour.z = 0.5;
					shading(colour, head.n, l, head.intersect, d);
				}
			}
			if (intersectSphere(body, d, origin)) {
				if(body.intmag < closest_mag) {
					closest_mag = body.intmag;
					colour.x = 0;
					colour.y = 0;
					colour.z = 0.5;
					shading(colour, body.n, l, body.intersect, d);
				}
			}
			if (intersectSphere(eye1, d, origin)) {
				if(eye1.intmag < closest_mag) {
					closest_mag = eye1.intmag;
					colour.x = 0.5;
					colour.y = 0.5;
					colour.z = 0.5;
					shading(colour, eye1.n, l, eye1.intersect, d);
				}
			}
			if (intersectSphere(pupil1, d, origin)) {
				if(pupil1.intmag < closest_mag) {
					closest_mag = pupil1.intmag;
					colour.x = 0;
					colour.y = 0;
					colour.z = 0;
					shading(colour, pupil1.n, l, pupil1.intersect, d);
				}
			}
			if (intersectSphere(pupil2, d, origin)) {
				if(pupil2.intmag < closest_mag) {
					closest_mag = pupil2.intmag;
					colour.x = 0;
					colour.y = 0;
					colour.z = 0;
					shading(colour, pupil2.n, l, pupil2.intersect, d);
				}
			}
			if (intersectSphere(eye2, d, origin)) {
				if(eye2.intmag < closest_mag) {
					closest_mag = eye2.intmag;
					colour.x = 0.5;
					colour.y = 0.5;
					colour.z = 0.5;
					shading(colour, eye2.n, l, eye2.intersect, d);
				}
			}

			if (intersectTriangle(mouth, d, origin)) {
				if(mouth.intmag < closest_mag) {
					closest_mag = mouth.intmag;
					colour.x = 0;
					colour.y = 0;
					colour.z = 0;
					shading(colour, mouth.pl.n, l, mouth.intersect, d);
				}
			}
			if (intersectTriangle(mouth2, d, origin)) {
				if(mouth2.intmag < closest_mag) {
					closest_mag = mouth2.intmag;
					colour.x = 0;
					colour.y = 0;
					colour.z = 0;
					shading(colour, mouth2.pl.n, l, mouth2.intersect, d);
				}
			}
			img.SetPixel(x, y, colour);
		}
	}
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
	window = glfwCreateWindow(512, 512, "CPSC 453 OpenGL Boilerplate", 0, 0);
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
	if ( argc != 2 ) {
		cout<<"Run `./boilerplate 1` for scene 1\n";
		cout<<"Run `./boilerplate 2` for scene 2\n";
		cout<<"Run `./boilerplate 3` for scene 3\n";
		return 0;
	}

	int scene = atoi(argv[1]);
	cout << scene;
	MyGeometry geometry;
	if (!InitializeGeometry(&geometry))
		cout << "Program failed to intialize geometry!" << endl;
	if (scene == 1)
		renderShapes();
	else if (scene == 2)
		renderShapes2();
	else if (scene == 3)
		renderShapes3();
	else {
		cout<<"Run `./boilerplate 1` for scene 1\n";
		cout<<"Run `./boilerplate 2` for scene 2\n";
		cout<<"Run `./boilerplate 3` for scene 3\n";
		return 0;
	}
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
