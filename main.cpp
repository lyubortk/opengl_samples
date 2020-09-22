#pragma optimize("", off)

#include <iostream>
#include <vector>
#include <chrono>
#include <string>

#include <fmt/format.h>

#include <GL/glew.h>

// Imgui + bindings
#include "imgui.h"
#include "bindings/imgui_impl_glfw.h"
#include "bindings/imgui_impl_opengl3.h"

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// STB, load images
#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

#include "opengl_shader.h"

static void glfw_error_callback(int error, const char *description) {
    std::cerr << fmt::format("Glfw Error {}: {}\n", error, description);
}

static float shift[] = {0, 0};
static float zoom = 1;

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

    int display_w, display_h, display_min;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    display_min = std::min(display_w, display_h);

    shift[0] += (prev_x - cur_x) / display_min * 2.0 * zoom;
    shift[1] += (cur_y - prev_y) / display_min * 2.0 * zoom;
    prev_x = cur_x;
    prev_y = cur_y;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    double x;
    double y;
    glfwGetCursorPos(window, &x, &y);

    int display_w, display_h, display_min;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    display_min = std::min(display_w, display_h);

    float new_zoom = zoom;
    if (yoffset > 0) {
        new_zoom *= 1.1;
    } else if (yoffset < 0) {
        new_zoom /= 1.1;
    }

    double x_normalized = (x * 2 - display_w) / display_min;
    double y_normalized = (y * 2 - display_h) / display_min;

    shift[0] += x_normalized * (zoom - new_zoom);
    shift[1] += y_normalized * (new_zoom - zoom);

    zoom = new_zoom;
}

void create_triangles(GLuint &vbo, GLuint &vao, GLuint &ebo) {
    float triangle_vertices[] = {
            -1.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, -1.0f,
            -1.0f, -1.0f,
    };
    unsigned int triangle_indices[] = {0, 1, 3, 1, 2, 3};

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle_indices), triangle_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void load_image(GLuint &texture, const std::string &name) {
    int width, height, channels;
    unsigned char *image = stbi_load(name.c_str(), &width, &height, &channels, STBI_rgb);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_1D, texture);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB8, width, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_1D);

    stbi_image_free(image);
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

    GLuint texture_red;
    GLuint texture_blue;
    load_image(texture_red, "texture_red.png");
    load_image(texture_blue, "texture_blue.png");

    // create our geometries
    GLuint vbo, vao, ebo;
    create_triangles(vbo, vao, ebo);

    // init shader
    shader_t shader("simple-shader.vs", "simple-shader.fs");

    // Setup GUI context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();

    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetScrollCallback(window, scroll_callback);

    auto const start_time = std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Get windows size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Set viewport to fill the whole window area
        glViewport(0, 0, display_w, display_h);

        // Fill background with solid color
        glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Gui start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // GUI
        ImGui::Begin("Settings");
        static int iterations = 50.0;
        ImGui::SliderInt("Iterations", &iterations, 10, 1000);
        static float seed[] = {-0.4, -0.6};
        ImGui::DragFloat2("Seed (c)", seed, 0.003, -1.0, 1.0);
        static int color = 0;
        ImGui::RadioButton("Red", &color, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Blue", &color, 1);
        ImGui::End();

        // Pass the parameters to the shader as uniforms
        shader.set_uniform("u_iter", iterations);
        shader.set_uniform("u_c", seed[0], seed[1]);
        shader.set_uniform("u_shift", shift[0], shift[1]);
        shader.set_uniform("u_display_w", display_w);
        shader.set_uniform("u_display_h", display_h);
        shader.set_uniform("u_display_min", std::min(display_h, display_w));
        shader.set_uniform("u_zoom", zoom);
        shader.set_uniform("u_texture", int(0));

        // Bind triangle shader
        shader.use();
        glActiveTexture(GL_TEXTURE0);
        if (color == 0) {
            glBindTexture(GL_TEXTURE_1D, texture_red);
        } else {
            glBindTexture(GL_TEXTURE_1D, texture_blue);
        }
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

        // Bind vertex array = buffers + indices
        glBindVertexArray(vao);
        // Execute draw call
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindTexture(GL_TEXTURE_1D, 0);
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
