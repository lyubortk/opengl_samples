#pragma optimize("", off)

#include <iostream>
#include <vector>
#include <chrono>

#include <fmt/format.h>

#include <GL/glew.h>

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// Imgui + bindings
#include "imgui.h"
#include "bindings/imgui_impl_glfw.h"
#include "bindings/imgui_impl_opengl3.h"

// STB, load images
#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

// Math constant and routines for OpenGL interop
#include <glm/gtc/constants.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc

#include <tiny_obj_loader.h>

#include "opengl_shader.h"

static void glfw_error_callback(int error, const char *description) {
    std::cerr << fmt::format("Glfw Error {}: {}\n", error, description);
}

static float radius = 15;
static float theta = -glm::pi<float>() / 2;
static float phi = 0;

unsigned char *height_data;

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    if (yoffset > 0) {
        radius *= 1.1;
    } else if (yoffset < 0) {
        radius /= 1.1;
    }

    radius = std::min(radius, 15.0f);
    radius = std::max(radius, 3.0f);
}

float a = 0;
float b = 0;
float rot = 0;

// TODO: use seconds
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        b += 0.005 * cos(rot);
        a += 0.02 * sin(rot);
    }
    if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        b -= 0.005 * cos(rot);
        a -= 0.02 * sin(rot);
    }
    if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        rot -= 0.05;
    }
    if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        rot += 0.05;
    }

    a = glm::mod(a, 1.0f);
    b = glm::mod(b, 1.0f);
    rot = glm::mod(rot, glm::pi<float>() * 2);
}

void create_environment(GLuint &vbo, GLuint &vao) {
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

int create_object(GLuint &vbo, GLuint &vao) {
    std::string inputfile = "spaceship/Space.obj";
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str(), "spaceship/");
    if (!ret) {
        exit(1);
    }

    std::vector<float> buffer_data;

// Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                float data[] = {vx, vy, vz, nx, ny, nz};
                buffer_data.insert(buffer_data.end(), data, data + 6);
            }
            index_offset += fv;
        }
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, buffer_data.size() * sizeof(float), &(buffer_data[0]), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) (3 * sizeof(float)));

    return buffer_data.size() / 6;
}

float r = 2.5;
float c = 10;

// TODO: rename variables
float get_height(float u, float v) {
    int heightmap_u = (int) (u * 512);
    int heightmap_v = (int) (v * 2048);
    heightmap_u %= 512;
    if (heightmap_u < 0) heightmap_u += 512;
    heightmap_v %= 2048;
    if (heightmap_v < 0) heightmap_v += 2048;
    int height = (int) height_data[(512 * heightmap_v + heightmap_u)];
    return (float) height / 100;
}

glm::vec3 get_torus_normal(float u, float v) {
    float TAU = glm::pi<float>() * 2;
    float nx = cos(u * TAU) * cos(v * TAU);
    float ny = cos(u * TAU) * sin(v * TAU);
    float nz = sin(u * TAU);
    return glm::vec3(nx, ny, nz);
}


glm::vec4 get_point(float u, float v) {
    float TAU = glm::pi<float>() * 2;

    float x = (c + r * cos(u * TAU)) * cos(v * TAU);
    float y = (c + r * cos(u * TAU)) * sin(v * TAU);
    float z = r * sin(u * TAU);

    glm::vec3 torus_normals = get_torus_normal(u, v);
    float height = get_height(u + 0.5f, v);
    return glm::vec4(x + torus_normals.x * height, y + torus_normals.y * height, z + torus_normals.z * height, height);
}

glm::vec3 get_surface_normal(float u, float v) {
    glm::vec3 u_plus = get_point(u + 1.0 / 512, v);
    glm::vec3 u_minus = get_point(u - 1.0 / 512, v);
    glm::vec3 v_plus = get_point(u, v + 1.0 / 2048);
    glm::vec3 v_minus = get_point(u, v - 1.0 / 2048);
    glm::vec3 u_diff = glm::normalize(u_minus - u_plus);
    glm::vec3 v_diff = glm::normalize(v_plus - v_minus);
    return glm::normalize(glm::cross(u_diff, v_diff));
}

int create_torus(GLuint &vbo, GLuint &vao) {
    std::vector<float> buffer_data;

    int rSeg = 128;
    int cSeg = 64;

    for (int i = 0; i < rSeg; i++) {
        for (int j = 0; j <= cSeg; j++) {
            for (int k = 0; k <= 1; k++) {
                float u = (i + k) / (float) rSeg;
                float v = j / (float) cSeg;

                glm::vec4 point = get_point(u, v);
                glm::vec3 normal = get_surface_normal(u, v);

                float texture_u = u + 0.5;
                if (texture_u > 1.0) texture_u -= 1.0;

                float data[] = {point.x, point.y, point.z, normal.x, normal.y, normal.z, texture_u, v, point.w};
                buffer_data.insert(buffer_data.end(), data, data + 9);

            }
        }
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, buffer_data.size() * sizeof(float), &(buffer_data[0]), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) (6 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) (8 * sizeof(float)));

    return buffer_data.size() / 9;
}

void load_environment_cubemap(GLuint &texture) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    int width, height, nrChannels;
    unsigned char *data;
    std::string faces[] = {
            "cubemap/right.jpg",
            "cubemap/left.jpg",
            "cubemap/top.jpg",
            "cubemap/bottom.jpg",
            "cubemap/front.jpg",
            "cubemap/back.jpg"
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

void load_texture(std::string name, GLuint &texture) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    int width, height, nrChannels;
    unsigned char *data;
    data = stbi_load(name.c_str(), &width, &height, &nrChannels, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(data);
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

    int width, height, nrChannels;
    height_data = stbi_load("torus/height.png", &width, &height, &nrChannels, 0);

    // create our geometries
    GLuint torus_vbo, torus_vao;
    int torus_points = create_torus(torus_vbo, torus_vao);

    GLuint spaceship_vbo, spaceship_vao;
    int spaceship_points = create_object(spaceship_vbo, spaceship_vao);

    GLuint environment_vbo, environment_vao;
    create_environment(environment_vbo, environment_vao);

    GLuint environment_texture;
    load_environment_cubemap(environment_texture);

    GLuint summer_texture;
    load_texture("torus/summer.png", summer_texture);
    GLuint winter_texture;
    load_texture("torus/winter.png", winter_texture);

    // init shader
    shader_t torus_shader("shaders/torus-shader.vs", "shaders/torus-shader.fs");
    shader_t environment_shader("shaders/environment-shader.vs", "shaders/environment-shader.fs");
    shader_t object_shader("shaders/object-shader.vs", "shaders/object-shader.fs");

    // Setup GUI context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Get windows size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Set viewport to fill the whole window area
        glViewport(0, 0, display_w, display_h);

        static float summer_threshold = 0.75;

        // Gui start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings");
        ImGui::SliderFloat("Summer threshold", &summer_threshold, 0, 3);
        ImGui::End();

        auto projection = glm::perspective<float>(glm::radians(90.0f), display_w * 1.0 / display_h, 0.1, 100);
        auto object_model = glm::identity<glm::mat4>();

// // // // // // // // /// // // // // // // // //

        glm::vec4 point = get_point(a, b);
        glm::vec3 surface_normal = get_surface_normal(a, b);
        glm::vec3 torus_normal = get_torus_normal(a, b);

        auto scale = glm::scale(glm::vec3(0.2, 0.2, 0.2));

        auto rotate_to_surface_normal = glm::rotate(
                glm::angle(glm::vec3(0, 1, 0), surface_normal),
                glm::cross(glm::vec3(0, 1, 0), surface_normal)
        );
        glm::vec3 current_nose_direction = rotate_to_surface_normal * glm::vec4(0, 0, -1, 0);
        glm::vec3 expected_nose_direction = glm::rotate(rot, torus_normal) *
                                            glm::rotate(-b * glm::pi<float>() * 2, glm::vec3(0, 0, -1)) *
                                            glm::vec4(0, 1, 0, 0);
        float angle_to_rotate_nose = glm::orientedAngle(
                current_nose_direction,
                glm::normalize(glm::cross(surface_normal, expected_nose_direction)),
                surface_normal) - glm::pi<float>() / 2;

        auto rotate_nose = glm::rotate(angle_to_rotate_nose, surface_normal);
        auto translate = glm::translate(glm::vec3(point));
        auto elevate = glm::translate(glm::vec3(0, 0.1, 0));
        auto spaceship_model = translate * rotate_nose * rotate_to_surface_normal * elevate * scale;
// // /// /// // // // // // // /// // // /// /// ///

        glm::vec3 up = expected_nose_direction;
        auto camera_position = glm::vec3(point) + radius * (0.7f * glm::vec3(torus_normal) - 0.3f * up);
        auto object_view = glm::lookAt<float>(camera_position, glm::vec3(point), up);
        auto object_view_projection = projection * object_view;

        auto environment_view = glm::lookAt<float>(glm::vec3(0, 0, 0), glm::vec3(point) - camera_position, up);
        auto environment_view_projection = projection * environment_view;

        glDepthMask(GL_TRUE);
        glColorMask(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        environment_shader.use();
        environment_shader.set_uniform("u_view_projection", glm::value_ptr(environment_view_projection));
        environment_shader.set_uniform("u_environment", int(0));
        glBindVertexArray(environment_vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_texture);

        // draw environment cube without setting z
        glDepthMask(GL_FALSE);
        glColorMask(1, 1, 1, 1);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        torus_shader.use();
        torus_shader.set_uniform("u_view_projection", glm::value_ptr(object_view_projection));
        torus_shader.set_uniform("u_model", glm::value_ptr(object_model));
        torus_shader.set_uniform("u_camera_pos", camera_position.x, camera_position.y, camera_position.z);
        torus_shader.set_uniform("u_environment", int(0));
        torus_shader.set_uniform("u_summer", int(1));
        torus_shader.set_uniform("u_winter", int(2));
        torus_shader.set_uniform("u_summer_threshold", summer_threshold);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, summer_texture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, winter_texture);
        glBindVertexArray(torus_vao);

        // Draw object
        glDepthMask(GL_TRUE);
        glColorMask(1, 1, 1, 1);
        glDepthFunc(GL_LEQUAL);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, torus_points);

        object_shader.use();
        object_shader.set_uniform("u_view_projection", glm::value_ptr(object_view_projection));
        object_shader.set_uniform("u_model", glm::value_ptr(spaceship_model));
        glBindVertexArray(spaceship_vao);
        glDrawArrays(GL_TRIANGLES, 0, spaceship_points);

        glBindVertexArray(0);

        // Generate gui render commands
        ImGui::Render();

        // Execute gui render commands using OpenGL backend
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap the backbuffer with the frontbuffer that is used for screen display
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
