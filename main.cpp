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

    radius = std::min(radius, 100.0f);
    radius = std::max(radius, 0.5f);
}

float a = 0;
float b = 0;
float rot = 0;

// TODO: use seconds
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        b += 0.01 * cos(rot);
        a += 0.04 * sin(rot);
    }
    if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        b -= 0.01 * cos(rot);
        a -= 0.04 * sin(rot);
    }
    if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        rot -= 0.05;
    }
    if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        rot += 0.05;
    }
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

int create_torus(GLuint &vbo, GLuint &vao) {
    std::vector<float> buffer_data;

    int rSeg = 128;
    int cSeg = 64;

    const double PI = 3.1415926535897932384626433832795;
    const double TAU = 2 * PI;

    for (int i = 0; i < rSeg; i++) {
        for (int j = 0; j <= cSeg; j++) {
            for (int k = 0; k <= 1; k++) {
                float s = (i + k) % rSeg + 0.5f;

                float x = (c + r * cos(s * TAU / rSeg)) * cos(j * TAU / cSeg);
                float y = (c + r * cos(s * TAU / rSeg)) * sin(j * TAU / cSeg);
                float z = r * sin(s * TAU / rSeg);

                float u = (i + k) / (float) rSeg + 0.5;
                float v = j / (float) cSeg;
                if (u > 1.0) u -= 1;

                float nx = cos(s * TAU / rSeg) * cos(j * TAU / cSeg);
                float ny = cos(s * TAU / rSeg) * sin(j * TAU / cSeg);
                float nz = sin(s * TAU / rSeg);

                int heightmap_u = (int) (u * 512) % 512;
                int heightmap_v = (int) (v * 2048) % 2048;
                int height = (int) height_data[(512 * heightmap_v + heightmap_u)];
                float height_f = (float) height / 100;

                float data[] = {x + nx * height_f, y + ny * height_f, z + nz * height_f, nx, ny, nz, u, v,
                                (float) height};
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

//glm::mat4 compute_spaceship_model() {
//
//}

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

    glfwSetCursorPosCallback(window, cursor_callback);
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

        static float summer_threshold = 65;

        // Gui start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings");
        ImGui::SliderFloat("Summer threshold", &summer_threshold, 0, 255);
        ImGui::End();

//        auto camera_position = glm::vec3(
//                glm::cos(theta) * glm::cos(phi) * radius,
//                glm::sin(phi) * radius,
//                glm::sin(theta) * glm::cos(phi) * radius
//        );
//        auto camera_position = glm::vec3(0, 0, -1 * radius);
        auto projection = glm::perspective<float>(glm::radians(90.0f), display_w * 1.0 / display_h, 0.1, 100);

        auto object_model = glm::identity<glm::mat4>();

//        auto spaceship_model = compute_spaceship_model();
// // // // // // // // /// // // // // // // // //
        float TAU = glm::pi<float>() * 2;
        float x = (c + r * cos(a * TAU)) * cos(b * TAU);
        float y = (c + r * cos(a * TAU)) * sin(b * TAU);
        float z = r * sin(a * TAU);

        // TODO: add real normals
        float nx = cos(a * TAU) * cos(b * TAU);
        float ny = cos(a * TAU) * sin(b * TAU);
        float nz = sin(a * TAU);

        auto temp = glm::cross(glm::vec3(0, 0, -1), glm::vec3(nx, ny, nz));
        auto angle = glm::angle(glm::vec3(0, 0, -1), glm::vec3(nx, ny, nz));

        auto scale = glm::scale(glm::vec3(0.3, 0.3, 0.3));
        auto rotate_d = glm::rotate(glm::pi<float>() + rot - (b * TAU), glm::vec3(0, 0, -1)) *
                glm::rotate(-glm::pi<float>() / 2, glm::vec3(1, 0, 0));
        auto rotate_l = glm::rotate(angle, temp);
        auto translate = glm::translate(glm::vec3(x, y, z));
        auto spaceship_model =  translate * rotate_l * rotate_d * scale;
// // /// /// // // // // // // /// // // /// /// ///

        glm::vec4 temp3 = (rotate_l * rotate_d * glm::vec4(0, 0, -1, 0));
        glm::vec3 up = glm::vec3(temp3.x, temp3.y, temp3.z);
        std::cout << temp3.x << ' ' << temp3.y << ' ' << temp3.z << ' ' << temp3.w << std::endl;
        auto camera_position = glm::vec3(x, y, z) + radius * (0.7f * glm::vec3(nx, ny, nz) - 0.3f * up);
        auto object_view = glm::lookAt<float>(camera_position, glm::vec3(x, y, z), up);
        auto object_view_projection = projection * object_view;

        auto environment_view = glm::lookAt<float>(glm::vec3(0, 0, 0), glm::vec3(x, y, z) - camera_position, up);
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
