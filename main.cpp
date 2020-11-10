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

static const float MAX_CAMERA_DISTANCE = 15;
static const float MIN_CAMERA_DISTANCE = 3;

static const float TORUS_SMALL_RADIUS = 2.5;
static const float TORUS_BIG_RADIUS = 10;

static const float OBJECT_FLIGHT_SPEED = 0.1;
static const float OBJECT_ROTATION_SPEED = 0.025;

static const unsigned int SHADOW_WIDTH = 4096;
static const unsigned int SHADOW_HEIGHT = 4096;

static const glm::vec3 UP = glm::vec3(0, 1, 0);
static const glm::vec3 INITIAL_NOSE_DIR = glm::vec3(0, 0, -1);
static const glm::vec3 CENTER = glm::vec3(0, 0, 0);
static const glm::vec3 SUN_DIR = glm::normalize(glm::vec3(-1, 0.6, -1));
static const glm::vec3 SUN_COLOR = glm::vec3(0.75, 0.75, 0.75);
static const glm::vec3 AMBIENT_COLOR = glm::vec3(1, 1, 1);
static const glm::vec3 SPACESHIP_COLOR = glm::vec3(0.19, 0.3, 0.3);
static const glm::vec3 HEADLIGHT_COLOR = glm::vec3(1, 1, 1);

static const float AMBIENT_STRENGTH = 0.4;
static const float SHADOWMAP_BIAS = 0.003;

static const float OBJECT_ELEVATION = 0.2;
static const float OBJECT_SCALE = 0.2;

static float camera_distance = 15;


static void glfw_error_callback(int error, const char *description) {
    std::cerr << fmt::format("Glfw Error {}: {}\n", error, description);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    if (yoffset > 0) {
        camera_distance *= 1.1;
    } else if (yoffset < 0) {
        camera_distance /= 1.1;
    }

    camera_distance = std::min(camera_distance, MAX_CAMERA_DISTANCE);
    camera_distance = std::max(camera_distance, MIN_CAMERA_DISTANCE);
}

static float object_position_small_angle = 0;
static float object_position_big_angle = 0;
static float object_rotation = 0;

void check_keyboard_keys(GLFWwindow *window) {
    static bool w_was_pressed = false;
    static bool s_was_pressed = false;
    static bool d_was_pressed = false;
    static bool a_was_pressed = false;

    bool w_pressed = glfwGetKey(window, GLFW_KEY_W);
    bool s_pressed = glfwGetKey(window, GLFW_KEY_S);
    bool d_pressed = glfwGetKey(window, GLFW_KEY_D);
    bool a_pressed = glfwGetKey(window, GLFW_KEY_A);

    if (w_pressed && w_was_pressed) {
        object_position_big_angle += OBJECT_FLIGHT_SPEED / TORUS_BIG_RADIUS * cos(object_rotation);
        object_position_small_angle += OBJECT_FLIGHT_SPEED / TORUS_SMALL_RADIUS * sin(object_rotation);
    }
    if (s_pressed && s_was_pressed) {
        object_position_big_angle -= OBJECT_FLIGHT_SPEED / TORUS_BIG_RADIUS * cos(object_rotation);
        object_position_small_angle -= OBJECT_FLIGHT_SPEED / TORUS_SMALL_RADIUS * sin(object_rotation);
    }
    if (d_pressed && d_was_pressed) {
        object_rotation -= OBJECT_ROTATION_SPEED;
    }
    if (a_pressed && a_was_pressed) {
        object_rotation += OBJECT_ROTATION_SPEED;
    }

    w_was_pressed = w_pressed;
    s_was_pressed = s_pressed;
    d_was_pressed = d_pressed;
    a_was_pressed = a_pressed;

    object_position_small_angle = glm::mod(object_position_small_angle, glm::two_pi<float>());
    object_position_big_angle = glm::mod(object_position_big_angle, glm::two_pi<float>());
    object_rotation = glm::mod(object_rotation, glm::two_pi<float>());
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

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];

            for (size_t v = 0; v < fv; v++) {
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

static unsigned char *height_data;
static int heightmap_h = 0;
static int heightmap_w = 0;

// TODO: rename variables
float get_height(float small_angle, float big_angle) {
    float u = small_angle / glm::two_pi<float>() + 0.5;
    float v = big_angle / glm::two_pi<float>();
    if (u > 1.0) u -= 1.0;

    int heightmap_u = (int) (u * heightmap_w);
    int heightmap_v = (int) (v * heightmap_h);
    heightmap_u %= heightmap_w;
    if (heightmap_u < 0) heightmap_u += heightmap_w;
    heightmap_v %= heightmap_h;
    if (heightmap_v < 0) heightmap_v += heightmap_h;
    int height = (int) height_data[(heightmap_w * heightmap_v + heightmap_u)];
    return (float) height / 100;
}

glm::vec3 get_torus_normal(float small_angle, float big_angle) {
    float nx = cos(small_angle) * cos(big_angle);
    float ny = cos(small_angle) * sin(big_angle);
    float nz = sin(small_angle);
    return glm::vec3(nx, ny, nz);
}

glm::vec4 get_surface_point(float small_angle, float big_angle) {
    float x = (TORUS_BIG_RADIUS + TORUS_SMALL_RADIUS * cos(small_angle)) * cos(big_angle);
    float y = (TORUS_BIG_RADIUS + TORUS_SMALL_RADIUS * cos(small_angle)) * sin(big_angle);
    float z = TORUS_SMALL_RADIUS * sin(small_angle);

    glm::vec3 torus_normals = get_torus_normal(small_angle, big_angle);
    float height = get_height(small_angle, big_angle);
    return glm::vec4(x + torus_normals.x * height, y + torus_normals.y * height, z + torus_normals.z * height, height);
}

glm::vec3 get_surface_normal(float small_angle, float big_angle) {
    glm::vec3 u_plus = get_surface_point(small_angle + 2.0 / heightmap_w * glm::two_pi<float>(), big_angle);
    glm::vec3 u_minus = get_surface_point(small_angle - 2.0 / heightmap_w * glm::two_pi<float>(), big_angle);
    glm::vec3 v_plus = get_surface_point(small_angle, big_angle + 2.0 / heightmap_h * glm::two_pi<float>());
    glm::vec3 v_minus = get_surface_point(small_angle, big_angle - 2.0 / heightmap_h * glm::two_pi<float>());
    glm::vec3 u_diff = glm::normalize(u_minus - u_plus);
    glm::vec3 v_diff = glm::normalize(v_plus - v_minus);
    return glm::normalize(glm::cross(u_diff, v_diff));
}

static const int SMALL_CIRCLE_SEGMENTS = 128;
static const int BIG_CIRCLE_SEGMENTS = 64;

int create_torus(GLuint &vbo, GLuint &vao) {
    std::vector<float> buffer_data;
    for (int i = 0; i < SMALL_CIRCLE_SEGMENTS; i++) {
        for (int j = 0; j <= BIG_CIRCLE_SEGMENTS; j++) {
            for (int k = 0; k <= 1; k++) {
                float u = (i + k) / (float) SMALL_CIRCLE_SEGMENTS;
                float v = j / (float) BIG_CIRCLE_SEGMENTS;

                float small_angle = u * glm::two_pi<float>();
                float big_angle = v * glm::two_pi<float>();

                glm::vec4 point = get_surface_point(small_angle, big_angle);
                glm::vec3 normal = get_surface_normal(small_angle, big_angle);

                float tex_u = glm::mod(u + 0.5f, 1.0f);
                float data[] = {point.x, point.y, point.z, normal.x, normal.y, normal.z, tex_u, v, point.w};
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

void load_torus_texture(std::string name, GLuint &texture) {
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

    height_data = stbi_load("torus/height.png", &heightmap_w, &heightmap_h, NULL, 0);

    // create our geometries
    GLuint torus_vbo, torus_vao;
    int torus_points = create_torus(torus_vbo, torus_vao);

    GLuint spaceship_vbo, spaceship_vao;
    int spaceship_points = create_object(spaceship_vbo, spaceship_vao);

    GLuint environment_vbo, environment_vao;
    create_environment(environment_vbo, environment_vao);

    GLuint environment_texture;
    load_environment_cubemap(environment_texture);

    // prepare shadowmap
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    GLuint summer_texture;
    load_torus_texture("torus/summer.png", summer_texture);
    GLuint winter_texture;
    load_torus_texture("torus/winter.png", winter_texture);

    // init shaders
    shader_t torus_shader("shaders/torus-shader.vs", "shaders/torus-shader.fs");
    shader_t environment_shader("shaders/environment-shader.vs", "shaders/environment-shader.fs");
    shader_t object_shader("shaders/object-shader.vs", "shaders/object-shader.fs");
    shader_t shadowmap_shader("shaders/shadowmap-shader.vs", "shaders/shadowmap-shader.fs");

    // Setup GUI context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();

    glfwSetScrollCallback(window, scroll_callback);

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        check_keyboard_keys(window);

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

        auto torus_model = glm::identity<glm::mat4>();

        glm::vec4 point = get_surface_point(object_position_small_angle, object_position_big_angle);
        glm::vec3 surface_normal = get_surface_normal(object_position_small_angle, object_position_big_angle);
        glm::vec3 torus_normal = get_torus_normal(object_position_small_angle, object_position_big_angle);

        glm::mat4 rotate_to_surface_normal = glm::identity<glm::mat4>();
        if (glm::abs(glm::dot(UP, surface_normal) - 1.0) > glm::epsilon<float>()) {
            rotate_to_surface_normal = glm::rotate(glm::angle(UP, surface_normal), glm::cross(UP, surface_normal));
        }

        glm::vec3 current_nose_direction = rotate_to_surface_normal * glm::vec4(INITIAL_NOSE_DIR, 0);
        glm::vec3 expected_nose_direction = glm::rotate(object_rotation, torus_normal) *
                                            glm::rotate(-object_position_big_angle, INITIAL_NOSE_DIR) *
                                            glm::vec4(UP, 0);

        float angle_to_rotate_nose = glm::orientedAngle(
                current_nose_direction,
                glm::normalize(glm::cross(surface_normal, expected_nose_direction)),
                surface_normal) - glm::half_pi<float>();

        auto rotate_nose = glm::rotate(angle_to_rotate_nose, surface_normal);
        auto translate = glm::translate(glm::vec3(point));
        auto elevate = glm::translate(glm::vec3(0, OBJECT_ELEVATION, 0));
        auto scale = glm::scale(glm::vec3(OBJECT_SCALE, OBJECT_SCALE, OBJECT_SCALE));

        auto spaceship_model = translate * rotate_nose * rotate_to_surface_normal * elevate * scale;

        glm::vec3 up = expected_nose_direction;
        auto camera_position = glm::vec3(point) + camera_distance * (0.7f * glm::vec3(torus_normal) - 0.3f * up);

        auto scene_view = glm::lookAt<float>(camera_position, glm::vec3(point), up);
        auto scene_view_projection = projection * scene_view;

        auto environment_view = glm::lookAt<float>(glm::vec3(0, 0, 0), glm::vec3(point) - camera_position, up);
        auto environment_view_projection = projection * environment_view;

        glDepthMask(GL_TRUE);
        glColorMask(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw cubemap
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

        // render shadowmap
        shadowmap_shader.use();
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glColorMask(1, 1, 1, 1);
        glClear(GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

        glm::mat4 light_view = glm::lookAt(SUN_DIR * glm::vec3(10, 10, 10), CENTER, UP);
        glm::mat4 light_projection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 0.1f, 40.0f);
        glm::mat4 light_space_mat = light_projection * light_view;

        shadowmap_shader.set_uniform("u_light_space_matrix", glm::value_ptr(light_space_mat));
        shadowmap_shader.set_uniform("u_model", glm::value_ptr(torus_model));
        glBindVertexArray(torus_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, torus_points);

        shadowmap_shader.set_uniform("u_model", glm::value_ptr(spaceship_model));
        glBindVertexArray(spaceship_vao);
        glDrawArrays(GL_TRIANGLES, 0, spaceship_points);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_DEPTH_BUFFER_BIT);

        //draw torus
        torus_shader.use();
        glm::vec4 headlight_pos = spaceship_model * glm::vec4(INITIAL_NOSE_DIR, 1);
        glm::vec3 headlight_dir = glm::normalize(
                rotate_nose * rotate_to_surface_normal * elevate * glm::vec4(INITIAL_NOSE_DIR, 0)
        );

        torus_shader.set_uniform("u_light_space_mat", glm::value_ptr(light_space_mat));
        torus_shader.set_uniform("u_headlight_pos",
                                 headlight_pos.x / headlight_pos.w,
                                 headlight_pos.y / headlight_pos.w,
                                 headlight_pos.z / headlight_pos.w);
        torus_shader.set_uniform("u_headlight_dir", headlight_dir.x, headlight_dir.y, headlight_dir.z);
        torus_shader.set_uniform("u_headlight_inner_cutoff", glm::cos(glm::radians(20.0f)));
        torus_shader.set_uniform("u_headlight_outer_cutoff", glm::cos(glm::radians(40.0f)));
        torus_shader.set_uniform("u_view_projection", glm::value_ptr(scene_view_projection));
        torus_shader.set_uniform("u_model", glm::value_ptr(torus_model));
        torus_shader.set_uniform("u_camera_pos", camera_position.x, camera_position.y, camera_position.z);
        torus_shader.set_uniform("u_environment", int(0));
        torus_shader.set_uniform("u_summer", int(1));
        torus_shader.set_uniform("u_winter", int(2));
        torus_shader.set_uniform("u_shadow_map", int(3));
        torus_shader.set_uniform("u_summer_threshold", summer_threshold);
        torus_shader.set_uniform("u_sun_dir", SUN_DIR.x, SUN_DIR.y, SUN_DIR.z);
        torus_shader.set_uniform("u_sun_color", SUN_COLOR.x, SUN_COLOR.y, SUN_COLOR.z);
        torus_shader.set_uniform("u_object_color", SPACESHIP_COLOR.x, SPACESHIP_COLOR.y, SPACESHIP_COLOR.z);
        torus_shader.set_uniform("u_ambient_color", AMBIENT_COLOR.x, AMBIENT_COLOR.y, AMBIENT_COLOR.z);
        torus_shader.set_uniform("u_ambient_strength", AMBIENT_STRENGTH);
        torus_shader.set_uniform("u_shadow_map_bias", SHADOWMAP_BIAS);
        torus_shader.set_uniform("u_headlight_color", HEADLIGHT_COLOR.x, HEADLIGHT_COLOR.y, HEADLIGHT_COLOR.z);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, summer_texture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, winter_texture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glBindVertexArray(torus_vao);

        // Draw object
        glDepthMask(GL_TRUE);
        glColorMask(1, 1, 1, 1);
        glDepthFunc(GL_LEQUAL);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, torus_points);
        object_shader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        object_shader.set_uniform("u_view_projection", glm::value_ptr(scene_view_projection));
        object_shader.set_uniform("u_model", glm::value_ptr(spaceship_model));
        object_shader.set_uniform("u_light_space_mat", glm::value_ptr(light_space_mat));
        object_shader.set_uniform("u_shadow_map", int(0));
        object_shader.set_uniform("u_sun_dir", SUN_DIR.x, SUN_DIR.y, SUN_DIR.z);
        object_shader.set_uniform("u_sun_color", SUN_COLOR.x, SUN_COLOR.y, SUN_COLOR.z);
        object_shader.set_uniform("u_object_color", SPACESHIP_COLOR.x, SPACESHIP_COLOR.y, SPACESHIP_COLOR.z);
        object_shader.set_uniform("u_ambient_color", AMBIENT_COLOR.x, AMBIENT_COLOR.y, AMBIENT_COLOR.z);
        object_shader.set_uniform("u_ambient_strength", AMBIENT_STRENGTH);
        object_shader.set_uniform("u_shadow_map_bias", SHADOWMAP_BIAS);

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
