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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

// Specify that we want the OpenGL core profile before including GLFW headers
#ifndef LAB_LINUX
    #include <glad/glad.h>
#else
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_GLEXT_PROTOTYPES
#endif
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;
using namespace glm;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

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

struct Coordinate
{
	double x;
	double y;
};

struct MouseStatus 
{
	bool button_pressed;
	Coordinate location_offset;
	Coordinate prev_location_offset;
	Coordinate mouse_press;
	float zoom = 4;
};
MouseStatus mouse_status;

struct KeyboardStatus 
{
	float x = 0;
	float y = 0;
};
KeyboardStatus keyboard_status;



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

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
    // OpenGL names for array buffer objects, vertex array object
    GLuint  vertexBuffer;
    // ---
    GLuint  normalBuffer;
    GLuint  textureBuffer;
    // ---
    GLuint  colourBuffer;
    GLuint  elementBuffer;
    GLuint  vertexArray;
    GLsizei elementCount;

    // initialize object names to zero (OpenGL reserved value)
    MyGeometry() : vertexBuffer(0),
    // ---
                   normalBuffer(0),
                   textureBuffer(0),
    // ---
                   colourBuffer(0),
                   vertexArray(0),
                   elementCount(0)
    {}
};

vector<GLfloat> get_sphere_vertices(float radius, float segmentation_level)
{
    float degree_step = 2*M_PI/segmentation_level;
    float rho = radius;
    float theta, phi;
    float next_theta, next_phi;

    vector<GLfloat> vertices;

    for (int i = 0; i < segmentation_level/2; ++i)
    {
        phi = i*degree_step;
        next_phi = phi + degree_step;

        for (int j = 0; j < segmentation_level; ++j)
        {
            theta = j*degree_step;
            next_theta = theta + degree_step;

            // first triangle
            // top left, bottom left, bottom right
            float triangle1_x1 = rho*cos(theta)*sin(phi);
            float triangle1_y1 = rho*sin(theta)*sin(phi);
            float triangle1_z1 = rho*cos(phi);
            float triangle1_x2 = rho*cos(theta)*sin(next_phi);
            float triangle1_y2 = rho*sin(theta)*sin(next_phi);
            float triangle1_z2 = rho*cos(next_phi);
            float triangle1_x3 = rho*cos(next_theta)*sin(next_phi);
            float triangle1_y3 = rho*sin(next_theta)*sin(next_phi);
            float triangle1_z3 = rho*cos(next_phi);

            vertices.push_back(triangle1_x1);
            vertices.push_back(triangle1_y1);
            vertices.push_back(triangle1_z1);
            vertices.push_back(triangle1_x2);
            vertices.push_back(triangle1_y2);
            vertices.push_back(triangle1_z2);
            vertices.push_back(triangle1_x3);
            vertices.push_back(triangle1_y3);
            vertices.push_back(triangle1_z3);

            // second triangle
            // top left, bottom right, top right
            float triangle2_x1 = rho*cos(theta)*sin(phi);
            float triangle2_y1 = rho*sin(theta)*sin(phi);
            float triangle2_z1 = rho*cos(phi);
            float triangle2_x2 = rho*cos(next_theta)*sin(next_phi);
            float triangle2_y2 = rho*sin(next_theta)*sin(next_phi);
            float triangle2_z2 = rho*cos(next_phi);
            float triangle2_x3 = rho*cos(next_theta)*sin(phi);
            float triangle2_y3 = rho*sin(next_theta)*sin(phi);
            float triangle2_z3 = rho*cos(phi);

            vertices.push_back(triangle2_x1);
            vertices.push_back(triangle2_y1);
            vertices.push_back(triangle2_z1);
            vertices.push_back(triangle2_x2);
            vertices.push_back(triangle2_y2);
            vertices.push_back(triangle2_z2);
            vertices.push_back(triangle2_x3);
            vertices.push_back(triangle2_y3);
            vertices.push_back(triangle2_z3);
        }
    }

    return vertices;
}

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry)
{
    // three vertex positions and assocated colours of a triangle
    /*
    const GLfloat vertices[][3] = {
        { -1, -1, 1 },
        { -1, 1, 1 },
        {  1, 1, 1 },
        {  1, -1, 1 },

        { -1, -1, -1 },
        { -1, 1,  -1 },
        {  1, 1,  -1 },
        {  1, -1, -1 }
    };
    */

    vector<GLfloat> vertices = get_sphere_vertices(1.5, 36);

    // make it a white sphere
    GLfloat colours[vertices.size()];
    fill_n(colours, vertices.size(), 1);
    // colour the top of the sphere specially
    //colours[0] = 0;

    /*
    const unsigned indices[36] = { //elements
        0, 2, 1, //Front
        0, 3, 2,

        1, 6, 5, //Top
        1, 2, 6,

        3, 6, 2, //Right
        3, 7, 6,

        1, 4, 0, //Left
        1, 5, 4,

        0, 4, 7, //Bottom
        0, 7, 3,

        4, 6, 7, //Back
        4, 5, 6
    };
    */

    // ---
    const GLfloat normals[][3] = {
        // front facing normals
        {-0.577, -0.577, 0.577},
        {-0.577,  0.577, 0.577},
        { 0.577,  0.577, 0.577},
        { 0.577, -0.577, 0.577},
        // back facing normals
        {-0.577, -0.577, -0.577},
        {-0.577,  0.577, -0.577},
        { 0.577,  0.577, -0.577},
        { 0.577, -0.577, -0.577},
    };

    const GLfloat textureCoords[16] = {
        0.0f, 0.0f,
        0.0f, 0.333f,
        0.333f, 0.333f,
        0.333f, 0.0f,

        0.0f, 0.0f,
        0.0f, 0.666f,
        0.666f, 0.666f,
        0.666f, 0.0f,
    };
    // ---

    /*
    geometry->elementCount = 36;
    */
    geometry->elementCount = vertices.size();

    // these vertex attribute indices correspond to those specified for the
    // input variables in the vertex shader
    const GLuint VERTEX_INDEX = 0;
    const GLuint COLOUR_INDEX = 1;
    // ---
    const GLuint TEXTURE_INDEX = 2;
    const GLuint NORMAL_INDEX = 3;
    // ---

    // create an array buffer object for storing our vertices
    glGenBuffers(1, &geometry->vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

    // create another one for storing our colours
    glGenBuffers(1, &geometry->colourBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);
    
    // ---
    // create another one for storing our textures
    glGenBuffers(1, &geometry->textureBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

    // create another one for storing our normals
    glGenBuffers(1, &geometry->normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, geometry->normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
    // ---

    // create a vertex array object encapsulating all our vertex attributes
    glGenVertexArrays(1, &geometry->vertexArray);
    glBindVertexArray(geometry->vertexArray);

    // create another one for storing our indices into the vertices
    //glGenBuffers(1, &geometry->elementBuffer);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->elementBuffer);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // ---
    glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
    glVertexAttribPointer(TEXTURE_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(TEXTURE_INDEX);

    glBindBuffer(GL_ARRAY_BUFFER, geometry->normalBuffer);
    glVertexAttribPointer(NORMAL_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(NORMAL_INDEX);
    // ---

    // associate the position array with the vertex array object
    glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
    glVertexAttribPointer(VERTEX_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(VERTEX_INDEX);

    // assocaite the colour array with the vertex array object
    glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
    glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(COLOUR_INDEX);

    // unbind our buffers, resetting to default state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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
    glDeleteBuffers(1, &geometry->elementBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene(MyGeometry *geometry, MyShader *shader, MyTexture *texture)
{
    // clear screen to a black colour
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // bind our shader program and the vertex array object containing our
    // scene geometry, then tell OpenGL to draw our geometry
    glBindTexture(texture->target, texture->textureID);
    glUseProgram(shader->program);
    glBindVertexArray(geometry->vertexArray);
    //glDrawElements(GL_TRIANGLES,geometry->elementCount,GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, geometry->elementCount);

    // reset state to default (no shader or geometry bound)
    glBindTexture(texture->target, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    // check for an report any OpenGL errors
    CheckGLErrors();
}

static void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouse_status.button_pressed = true;
		glfwGetCursorPos(window, &mouse_status.mouse_press.x, &mouse_status.mouse_press.y);
		mouse_status.prev_location_offset = mouse_status.location_offset;
	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		mouse_status.button_pressed = false;
	}
	cout << "location_x:" << mouse_status.location_offset.x << endl;
	cout << "location_y:" << mouse_status.location_offset.y << endl;
}

static void cursor_callback(GLFWwindow* window, double xpos, double ypos) {
	if(mouse_status.button_pressed) {
		mouse_status.location_offset.x = mouse_status.prev_location_offset.x - ((mouse_status.mouse_press.x - xpos))/100;
		mouse_status.location_offset.y = mouse_status.prev_location_offset.y + ((mouse_status.mouse_press.y - ypos))/100;
	} 
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	mouse_status.zoom -= yoffset;
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
    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
		keyboard_status.x += 0.1;
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
		keyboard_status.x -= 0.1;
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
		keyboard_status.y += 0.1;
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
		keyboard_status.y -= 0.1;
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
    int width = 1000, height = 1000;
    window = glfwCreateWindow(width, height, "CPSC 453 OpenGL Boilerplate", 0, 0);
    if (!window) {
        cout << "Program failed to create GLFW window, TERMINATING" << endl;
        glfwTerminate();
        return -1;
    }

    // set keyboard callback function and make our context current (active)
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, mouse_callback);
	glfwSetCursorPosCallback(window, cursor_callback);
	glfwSetScrollCallback(window, scroll_callback);
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

    // call function to create and fill buffers with geometry data
    MyGeometry geometry;
    if (!InitializeGeometry(&geometry))
        cout << "Program failed to intialize geometry!" << endl;

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    float angle = 0.f, size = 1.f;
    vec3 location(0,0,0);
    vec3 axis(0,1,0);

    float camera_radius = 4;
    float camera_phi = 0;
    float camera_theta = 0;

    float aspectRatio = (float)width/ (float)height;
    float zNear = .1f, zFar = 1000.f;
    float fov = 1.0472f;

    mat4 I(1);
    glUseProgram(shader.program);
    GLint modelUniform = glGetUniformLocation(shader.program, "model");
    GLint viewUniform = glGetUniformLocation(shader.program, "view");
    GLint projUniform = glGetUniformLocation(shader.program, "proj");
    // run an event-triggered main loop
    while (!glfwWindowShouldClose(window))
    {
        glUseProgram(shader.program);
        mat4 model = translate(I, location) * rotate(I, angle, axis) * rotate(I, angle, vec3(1,0,0)) * scale(I, vec3(size, 1, 1));

		camera_phi = mouse_status.location_offset.x;
		camera_theta = mouse_status.location_offset.y;

		vec3 cameraLoc( sin(camera_phi)*sin(camera_theta), cos(camera_theta), cos(camera_phi)*sin(camera_theta));
        vec3 modified_camera_loc = mouse_status.zoom * cameraLoc;
		vec3 cameraDir = -modified_camera_loc;
		vec3 cameraUp = cross(vec3( cos(camera_phi), 0, -sin(camera_phi)), cameraDir);

        mat4 view = lookAt(modified_camera_loc, -modified_camera_loc, cameraUp);
        mat4 proj = perspective(fov, aspectRatio, zNear, zFar);
        glUniformMatrix4fv(modelUniform, 1, false, value_ptr(model));
        glUniformMatrix4fv(viewUniform, 1, false, value_ptr(view));
        glUniformMatrix4fv(projUniform, 1, false, value_ptr(proj));

        // ---
        MyTexture texture;
        InitializeTexture(&texture, "test.png");
        // ---

        // call function to draw our scene
        RenderScene(&geometry, &shader, &texture);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // clean up allocated resources before exit
    DestroyGeometry(&geometry);
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
