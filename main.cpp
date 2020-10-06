#pragma optimize("", off)

#include <iostream>
#include <vector>
#include <chrono>

#include <fmt/format.h>

#include <GL/glew.h>

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// STB, load images
#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

// Math constant and routines for OpenGL interop
#include <glm/gtc/constants.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "opengl_shader.h"

static void glfw_error_callback(int error, const char *description) {
    std::cerr << fmt::format("Glfw Error {}: {}\n", error, description);
}

static float radius = 1.0;
static float theta = 0;
static float phi = 0;

static void cursor_callback(GLFWwindow *window, double xpos, double ypos) {
    static int last_mouse_button = -1;
    static double prev_x = -1;
    static double prev_y = -1;

    int mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    double cur_x = -1;
    double cur_y = -1;
    glfwGetCursorPos(window, &cur_x, &cur_y);

    if (mouse_button == GLFW_PRESS && last_mouse_button != GLFW_PRESS) {
        prev_x = cur_x;
        prev_y = cur_y;
    }
    last_mouse_button = mouse_button;

    if (mouse_button != GLFW_PRESS) {
        return;
    }

    theta += (cur_x - prev_x) / 300;
    phi += (cur_y - prev_y) / 300;
    theta = glm::mod(theta, 2 * glm::pi<float>());
    if (phi >= glm::pi<float>() / 2) {
        phi = glm::pi<float>() / 2 - glm::epsilon<float>();
    }
    if (phi <= -glm::pi<float>() / 2) {
        phi = -glm::pi<float>() / 2 + glm::epsilon<float>();
    }
    prev_x = cur_x;
    prev_y = cur_y;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    if (yoffset > 0) {
        radius *= 1.1;
    } else if (yoffset < 0) {
        radius /= 1.1;
    }

    radius = std::min(radius, 10.0f);
    radius = std::max(radius, 0.5f);
}

void create_environment(GLuint &vbo, GLuint &vao, GLuint &ebo) {
    float environmentVertices[] = {
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f
    };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(environmentVertices), &environmentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
}

void create_triangle(GLuint &vbo, GLuint &vao, GLuint &ebo) {
    // create the triangle
    float triangle_vertices[] = {
            0.0f, 0.25f, 0.0f,    // position vertex 1
            1.0f, 0.0f, 0.0f,     // color vertex 1

            0.25f, -0.25f, 0.0f,  // position vertex 1
            0.0f, 1.0f, 0.0f,     // color vertex 1

            -0.25f, -0.25f, 0.0f, // position vertex 1
            0.0f, 0.0f, 1.0f,     // color vertex 1
    };

    unsigned int triangle_indices[] = {
            0, 1, 2};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle_indices), triangle_indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void load_environment_cubemap(GLuint &texture) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    int width, height, nrChannels;
    unsigned char *data;
    std::string faces[] = {
            "cubemap/posx.jpg",
            "cubemap/negx.jpg",
            "cubemap/posy.jpg",
            "cubemap/negy.jpg",
            "cubemap/posz.jpg",
            "cubemap/negz.jpg"
    };
    for (unsigned int i = 0; i < 6; i++) {
        data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
        );
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

int main(int, char **) {
    // Use GLFW to create a simple window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;


    // GL 3.3 + GLSL 330
    const char *glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Dear ImGui - Conan", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize GLEW, i.e. fill all possible function pointers for current OpenGL context
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize OpenGL loader!\n";
        return 1;
    }

    // create our geometries
    GLuint object_vbo, object_vao, object_ebo;
    create_triangle(object_vbo, object_vao, object_ebo);
    GLuint environment_vbo, environment_vao, environment_ebo;
    create_environment(environment_vbo, environment_vao, environment_ebo);

    GLuint environment_texture;
    load_environment_cubemap(environment_texture);

    // init shader
    shader_t object_shader("shaders/object-shader.vs", "shaders/object-shader.fs");
    shader_t environment_shader("shaders/environment-shader.vs", "shaders/environment-shader.fs");

    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetScrollCallback(window, scroll_callback);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Get windows size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Set viewport to fill the whole window area
        glViewport(0, 0, display_w, display_h);

        // Fill background with solid color
        glClearColor(0.30f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        auto cartesian_coordiantes = glm::vec3(
                glm::cos(theta) * glm::cos(phi) * radius,
                glm::sin(phi) * radius,
                glm::sin(theta) * glm::cos(phi) * radius
        );
        auto projection = glm::perspective<float>(glm::radians(90.0f), 1.3, 0.1, 100);

        auto object_view = glm::lookAt<float>(cartesian_coordiantes, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        auto object_mvp = projection * object_view;

        auto environment_view = glm::lookAt<float>(glm::vec3(0, 0, 0), -cartesian_coordiantes, glm::vec3(0, 1, 0));
        auto environment_mvp = projection * environment_view;

        glDepthMask(GL_FALSE);
        environment_shader.use();
        environment_shader.set_uniform("u_mvp", glm::value_ptr(environment_mvp));
        environment_shader.set_uniform("environment", int(0));
        glBindVertexArray(environment_vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_texture);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDepthMask(GL_TRUE);
        // Bind triangle shader
        object_shader.use();
        object_shader.set_uniform("u_mvp", glm::value_ptr(object_mvp));
        // Bind vertex array = buffers + indices
        glBindVertexArray(object_vao);
        // Execute draw call
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Swap the backbuffer with the frontbuffer that is used for screen display
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
