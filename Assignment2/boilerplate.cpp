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
#include <vector>

// specify that we want the OpenGL core profile before including GLFW headers
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

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
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering
struct ImageResolution 
{
	float width;
	float height;
};

ImageResolution image_resolutions[6];

enum SampleImages
{
	IMAGE1,
	IMAGE2,
	IMAGE3,
	IMAGE4,
	IMAGE5,
	IMAGE6,
};

struct CurrentState
{
    enum SampleImages image;
	int layer;
	float rotation;
};

CurrentState current_state;

struct Coordinate
{
	double x;
	double y;
};

struct MouseStatus 
{
	bool button_pressed;
	Coordinate image_offset;
	Coordinate prev_image_offset;
	Coordinate mouse_press;
};

MouseStatus mouse_status;

struct Luminance
{
	float r;
	float g;
	float b;
};

Luminance luminance;
float enable_sepia;
float sobel;
float gaus;

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

MyTexture global_tex;

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
		cout << numComponents << endl;
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
void insert_vertices(vector<float>& vertices) {
	float ratio = image_resolutions[current_state.image].height / image_resolutions[current_state.image].width;
	float basepos = 1.f;
	float baseneg = -1.f;

	if(ratio > 1) {
		vertices.push_back(-1*ratio/2); //Top left
		vertices.push_back(basepos);

		vertices.push_back(ratio/2); // Top right
		vertices.push_back(basepos);

		vertices.push_back(-1*ratio/2); // Bottom left
		vertices.push_back(baseneg);

		vertices.push_back(-1*ratio/2); // Bottom left
		vertices.push_back(baseneg);

		vertices.push_back(ratio/2); // Bottom right
		vertices.push_back(baseneg);

		vertices.push_back(ratio/2); // Top right
		vertices.push_back(basepos);
	} else if(ratio < 1) {
		vertices.push_back(baseneg); // Top left
		vertices.push_back(ratio); 

		vertices.push_back(basepos); // top right
		vertices.push_back(ratio); 

		vertices.push_back(baseneg); // bottom left
		vertices.push_back(-1*ratio); 

		vertices.push_back(baseneg); // bottom left
		vertices.push_back(-1*ratio); 

		vertices.push_back(basepos); // Bottom right
		vertices.push_back(-1*ratio); 

		vertices.push_back(basepos); // Top right
		vertices.push_back(ratio); 
	} else {
		vertices.push_back(baseneg); //Top left
		vertices.push_back(basepos);

		vertices.push_back(basepos); // Top right
		vertices.push_back(basepos);

		vertices.push_back(baseneg); // Bottom left
		vertices.push_back(baseneg);

		vertices.push_back(baseneg); // Bottom left
		vertices.push_back(baseneg);

		vertices.push_back(basepos); // Bottom right
		vertices.push_back(baseneg);

		vertices.push_back(basepos); // Top right
		vertices.push_back(basepos);
	}
}

bool InitializeGeometry(MyGeometry *geometry, vector<float>& vertices, vector<float>& texture_coord, vector<float>& colours)
{
	insert_vertices(vertices);

	texture_coord.push_back(0);
	texture_coord.push_back(image_resolutions[current_state.image].height);

	texture_coord.push_back(image_resolutions[current_state.image].width);
	texture_coord.push_back(image_resolutions[current_state.image].height);
	
	texture_coord.push_back(0);
	texture_coord.push_back(0);

	texture_coord.push_back(0);
	texture_coord.push_back(0);

	texture_coord.push_back(image_resolutions[current_state.image].width);
	texture_coord.push_back(0);

	texture_coord.push_back(image_resolutions[current_state.image].width);
	texture_coord.push_back(image_resolutions[current_state.image].height);

	colours.push_back(1);
	colours.push_back(0);
	colours.push_back(0);
	
	colours.push_back(0);
	colours.push_back(1);
	colours.push_back(0);

	colours.push_back(0);
	colours.push_back(0);
	colours.push_back(1);

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;
	const GLuint TEXTURE_INDEX = 2;

	geometry->elementCount = vertices.size()/2;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertices.size(), &vertices[0], GL_STATIC_DRAW);

	//
	glGenBuffers(1, &geometry->textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*texture_coord.size(), &texture_coord[0], GL_STATIC_DRAW);

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

	// Tell openGL how the data is formatted
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glVertexAttribPointer(TEXTURE_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(TEXTURE_INDEX);

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

void RenderScene(MyGeometry *geometry, MyTexture* texture, MyShader *shader)
{
	// clear screen to a dark grey colour
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
	glBindVertexArray(geometry->vertexArray);
	glBindTexture(texture->target, texture->textureID);
	glDrawArrays(GL_TRIANGLES, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindTexture(texture->target, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

void update_display() {
	vector<float> colours;
	vector<float> vertices;
	vector<float> texture_coord;
	string imagepath = "images/";

	switch(current_state.image) {
		case IMAGE1:
			imagepath += "image1-mandrill.png";
			break;
		case IMAGE2:
			imagepath += "image2-uclogo.png";
			break;
		case IMAGE3:
			imagepath += "image3-aerial.jpg";
			break;
		case IMAGE4:
			imagepath += "image4-thirsk.jpg";
			break;
		case IMAGE5:
			imagepath += "image5-pattern.png";
			break;
		case IMAGE6:
			imagepath += "image6-why.jpg";
			break;
	}

	const char* p_c_str = imagepath.c_str();
	
	current_state.layer = 1;
	current_state.rotation = 0;
	enable_sepia = 0;
	luminance.r = 1;
	luminance.g = 1;
	luminance.b = 1;
	sobel = 0;
	gaus = 0;
	mouse_status.image_offset.x = 0;
	mouse_status.image_offset.y = 0;

	if(!InitializeTexture(&global_tex, p_c_str, GL_TEXTURE_RECTANGLE))
		cout << "Program failed to intialize texture!" << endl;

	if (!InitializeGeometry(&global_geo, vertices, texture_coord, colours))
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
		current_state.image = IMAGE1;
		should_update_display = true;
	} else if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		current_state.image = IMAGE2;
		should_update_display = true;
	} else if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		current_state.image = IMAGE3;
		should_update_display = true;
	} else if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
		current_state.image = IMAGE4;
		should_update_display = true;
	} else if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
		current_state.image = IMAGE5;
		should_update_display = true;
	} else if (key == GLFW_KEY_6 && action == GLFW_PRESS) {
		current_state.image = IMAGE6;
		should_update_display = true;
	} else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		current_state.rotation += M_PI/12;
	} else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		current_state.rotation -= M_PI/12;
	} else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		enable_sepia = 0;
		sobel = 0;
		gaus = 0;
		luminance.r = 0.333;
		luminance.g = 0.333;
		luminance.b = 0.333;
	} else if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		enable_sepia = 0;
		sobel = 0;
		gaus = 0;
		luminance.r = 0.333;
		luminance.r = 0.299;
		luminance.g = 0.587;
		luminance.b = 0.114;
	} else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		enable_sepia = 0;
		sobel = 0;
		gaus = 0;
		luminance.r = 0.333;
		luminance.r = 0.213;
		luminance.g = 0.715;
		luminance.b = 0.072;
	} else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		enable_sepia = 1;
		sobel = 0;
		gaus = 0;
		luminance.r = 0.333;
		luminance.r = 1;
		luminance.g = 1;
		luminance.b = 1;
	} else if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		sobel = 1;
		gaus = 0;
	} else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		sobel = 2;
		gaus = 0;
	} else if (key == GLFW_KEY_D && action == GLFW_PRESS) {
		sobel = 3;
		gaus = 0;
	} else if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		gaus = 3;
	} else if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		gaus = 5;
	} else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
		gaus = 7;
	}

	if(should_update_display) {
		update_display();
	}
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if(!((current_state.layer + yoffset) < 1))
		current_state.layer = current_state.layer + yoffset;
}


static void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouse_status.button_pressed = true;
        glfwGetCursorPos(window, &mouse_status.mouse_press.x, &mouse_status.mouse_press.y);
        mouse_status.prev_image_offset = mouse_status.image_offset;
	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		mouse_status.button_pressed = false;
    }
}

static void cursor_callback(GLFWwindow* window, double xpos, double ypos)
{
	if(mouse_status.button_pressed) {
        mouse_status.image_offset.x = mouse_status.prev_image_offset.x - (mouse_status.mouse_press.x - xpos);
        mouse_status.image_offset.y = mouse_status.prev_image_offset.y + (mouse_status.mouse_press.y - ypos);
    }
}

void StoreImageResolution()
{
	image_resolutions[0].width = 512;
	image_resolutions[0].height = 512;

	image_resolutions[1].width = 692;
	image_resolutions[1].height = 516;

	image_resolutions[2].width = 2000;
	image_resolutions[2].height = 931;

	image_resolutions[3].width = 400;
	image_resolutions[3].height = 591;

	image_resolutions[4].width = 2048;
	image_resolutions[4].height = 1536;

	image_resolutions[5].width = 3388;
	image_resolutions[5].height = 2207;
}

// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	//initialize default behavior
	current_state.layer = 1;
	current_state.rotation = 0;
	current_state.image = IMAGE1;
	StoreImageResolution();

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
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_callback);
	glfwSetCursorPosCallback(window, cursor_callback);
	glfwMakeContextCurrent(window);

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
		
        GLint new_image_location = glGetUniformLocation(shader.program, "image_offset");
        glUniform2f(new_image_location, mouse_status.image_offset.x, mouse_status.image_offset.y);

        GLint image_rotation = glGetUniformLocation(shader.program, "rotation");
        glUniform1f(image_rotation, current_state.rotation);

        GLint image_magnification = glGetUniformLocation(shader.program, "magnification");
        glUniform1f(image_magnification, current_state.layer);

        GLint image_luminance = glGetUniformLocation(shader.program, "luminance");
        glUniform3f(image_luminance, luminance.r, luminance.g, luminance.b);

        GLint image_sepia = glGetUniformLocation(shader.program, "sepia");
        glUniform1f(image_sepia, enable_sepia);

        GLint image_sobel = glGetUniformLocation(shader.program, "sobel");
        glUniform1ui(image_sobel, sobel);

        GLint image_gaus = glGetUniformLocation(shader.program, "gaussize");
        glUniform1ui(image_gaus, gaus);
		// call function to draw our scene
		RenderScene(&global_geo, &global_tex, &shader); //render scene with texture

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
