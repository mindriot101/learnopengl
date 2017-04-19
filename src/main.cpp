#include <iostream>
#include <cmath>
#include <cassert>
#include <memory>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;

/* Globals */
bool KEY_PRESSED[1024];
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

GLenum glCheckError_(const char *file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__) 


void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

void clear_screen();

static GLint success;
static GLchar infoLog[512];

/*
GLfloat vertices[] = {
    0.5f,  0.5f, 0.0f,  // Top Right
    0.5f, -0.5f, 0.0f,  // Bottom Right
    -0.5f, -0.5f, 0.0f,  // Bottom Left
    -0.5f,  0.5f, 0.0f   // Top Left 
};
GLuint indices[] = {  // Note that we start from 0!
    0, 1, 3,   // First Triangle
    1, 2, 3    // Second Triangle
};  
*/
GLfloat vertices[] = {
    -0.5f, -0.5f, 0.0f,
    0.0f, 0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
};

GLuint indices[] = {
    0, 1, 2,
};

enum class ShaderType {
    VERTEX,
    FRAGMENT,
};

struct Shader {
    ShaderType shader_type;
    GLuint id;
};

Shader create_shader(ShaderType type, const char *src) {
    Shader shader;
    switch (type) {
        case ShaderType::VERTEX:
            shader.id = glCreateShader(GL_VERTEX_SHADER);
            glCheckError();
            break;
        case ShaderType::FRAGMENT:
            shader.id = glCreateShader(GL_FRAGMENT_SHADER);
            glCheckError();
            break;
        default:
            /* Unreachable */
            std::cerr << "Invalid shader type passed\n";
    };

    glShaderSource(shader.id, 1, &src, nullptr);
    glCheckError();
    glCompileShader(shader.id);
    glCheckError();

    glGetShaderiv(shader.id, GL_COMPILE_STATUS, &success);
    glCheckError();

    if (!success) {
        glGetShaderInfoLog(shader.id, 512, nullptr, infoLog);
        glCheckError();
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

struct ShaderProgram {
    Shader vertex, fragment;
    GLuint id;

};

shared_ptr<ShaderProgram> create_shader_program(const char *vertex_src, const char *fragment_src) {
    shared_ptr<ShaderProgram> program = make_shared<ShaderProgram>();
    Shader vertex_shader = create_shader(ShaderType::VERTEX, vertex_src);
    Shader fragment_shader = create_shader(ShaderType::FRAGMENT, fragment_src);

    program->id = glCreateProgram();
    glCheckError();
    glAttachShader(program->id, vertex_shader.id);
    glCheckError();
    glAttachShader(program->id, fragment_shader.id);
    glCheckError();
    glLinkProgram(program->id);
    glCheckError();

    glGetProgramiv(program->id, GL_LINK_STATUS, &success);
    glCheckError();
    if (!success) {
        glGetProgramInfoLog(program->id, 512, nullptr, infoLog);
        glCheckError();
        std::cerr << "ERROR::SHADER::PROGRAM::LINK_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex_shader.id);
    glCheckError();
    glDeleteShader(fragment_shader.id);
    glCheckError();

    return program;
}

void use_shader(const shared_ptr<ShaderProgram> &program) {
    glUseProgram(program->id);
    glCheckError();
}

struct Mesh {
    GLuint VBO;
    GLuint VAO;
    GLuint EBO;
    GLuint draw_mode = GL_FILL;
    int n_indices;
    shared_ptr<ShaderProgram> shader;
};

Mesh create_mesh(int n_vertices, GLfloat *vertices,
        int n_indices, GLuint *indices,
        const shared_ptr<ShaderProgram> &shader) {

    Mesh mesh;

    const int vertices_size = n_vertices * sizeof(GLfloat) * 3;

    glGenVertexArrays(1, &mesh.VAO);
    glCheckError();
    glGenBuffers(1, &mesh.VBO);
    glCheckError();
    glGenBuffers(1, &mesh.EBO);
    glCheckError();

    glBindVertexArray(mesh.VAO);
    glCheckError();

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glCheckError();
    glBufferData(GL_ARRAY_BUFFER, vertices_size,
            vertices, GL_STATIC_DRAW);
    glCheckError();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glCheckError();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, n_indices * sizeof(GLuint),
            indices, GL_STATIC_DRAW);
    glCheckError();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glCheckError();
    glEnableVertexAttribArray(0);
    glCheckError();

    glBindVertexArray(0);
    glCheckError();

    mesh.shader = shader;
    mesh.n_indices = n_indices;

    return mesh;
}

Mesh create_mesh(int n_vertices, GLfloat *vertices,
        int n_indices, GLuint *indices,
        const char *vertex_shader_src,
        const char *fragment_shader_src) {

    shared_ptr<ShaderProgram> shader = create_shader_program(vertex_shader_src, fragment_shader_src);

    return create_mesh(n_vertices, vertices, n_indices, indices, shader);
}

void draw_mesh(const Mesh &mesh) {
    glBindVertexArray(mesh.VAO);
    glCheckError();
    glPolygonMode(GL_FRONT_AND_BACK, mesh.draw_mode);
    glCheckError();
    glDrawElements(GL_TRIANGLES, mesh.n_indices, GL_UNSIGNED_INT, 0);
    glCheckError();
    glBindVertexArray(0);
    glCheckError();
}

void draw_mesh_with_bound_shader(const Mesh &mesh) {
    use_shader(mesh.shader);
    draw_mesh(mesh);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    /* Initialize glew */
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    glfwSetKeyCallback(window, key_callback);

    const char *vertex_shader_src = "#version 330 core\n"
        "layout (location = 0) in vec3 position;\n"
        "void main()\n"
        "{\n"
        "gl_Position = vec4(position.x, position.y, position.z, 1.0);\n"
        "}\n";

    const char *fragment_shader_src = "#version 330 core\n"
        "out vec4 color;\n"
        "uniform vec4 ourColor;\n"
        "void main()\n"
        "{\n"
        "color = ourColor;\n"
        "}\n";
    shared_ptr<ShaderProgram> shader = create_shader_program(vertex_shader_src, fragment_shader_src);

    Mesh triangle = create_mesh(3, &vertices[0], 3, &indices[0], shader);

    GLfloat greenValue = 0;
    GLfloat redValue = 0;
    GLfloat blueValue = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (KEY_PRESSED[GLFW_KEY_W]) {
            greenValue += deltaTime;
        } else if (KEY_PRESSED[GLFW_KEY_S]) {
            greenValue -= deltaTime;
        }
        greenValue = fmin(fmax(greenValue, 0.0), 1.0);

        if (KEY_PRESSED[GLFW_KEY_D]) {
            redValue += deltaTime;
        } else if (KEY_PRESSED[GLFW_KEY_A]) {
            redValue -= deltaTime;
        }
        redValue = fmin(fmax(redValue, 0.0), 1.0);

        if (KEY_PRESSED[GLFW_KEY_E]) {
            blueValue += deltaTime;
        } else if (KEY_PRESSED[GLFW_KEY_Q]) {
            blueValue -= deltaTime;
        }
        blueValue = fmin(fmax(blueValue, 0.0), 1.0);

        clear_screen();

        GLint vertexColorLocation = glGetUniformLocation(shader->id, "ourColor");
        if (vertexColorLocation == -1) {
            std::cerr << "Cannot find uniform location" << std::endl;
            return 1;
        }
        use_shader(shader);
        glUniform4f(vertexColorLocation, redValue, greenValue, blueValue, 1.0f);

        draw_mesh(triangle);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    /* Handle single events first */
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    /* Now normal keys */
    if (action == GLFW_PRESS) {
        KEY_PRESSED[key] = true;
    } else if (action == GLFW_RELEASE) {
        KEY_PRESSED[key] = false;
    }
}

void clear_screen() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glCheckError();
    glClear(GL_COLOR_BUFFER_BIT);
    glCheckError();
}
