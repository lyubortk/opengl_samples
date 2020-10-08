cmake_minimum_required(VERSION 3.0)
project(opengl-imgui-sample CXX)

set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

# CONFIG option is important so that CMake doesnt search for modules into the default modules directory
find_package(imgui CONFIG)
find_package(glfw CONFIG)
find_package(glew CONFIG)
find_package(fmt CONFIG)
find_package(glm CONFIG)
find_package(stb CONFIG)
find_package(tinyobjloader CONFIG)

add_executable( opengl-imgui-sample
                main.cpp
                opengl_shader.cpp
                opengl_shader.h
                bindings/imgui_impl_glfw.cpp
                bindings/imgui_impl_opengl3.cpp
                bindings/imgui_impl_glfw.h
                bindings/imgui_impl_opengl3.h
                assets/shaders/object-shader.vs
                assets/shaders/object-shader.fs
                assets/shaders/environment-shader.vs
                assets/shaders/environment-shader.fs)

add_custom_command(TARGET opengl-imgui-sample
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets/shaders ${PROJECT_BINARY_DIR}/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets/cubemap ${PROJECT_BINARY_DIR}/cubemap
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets/object ${PROJECT_BINARY_DIR}/object
)

target_compile_definitions(opengl-imgui-sample PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
target_link_libraries(opengl-imgui-sample imgui::imgui GLEW::glew_s glfw::glfw fmt::fmt glm::glm stb::stb tinyobjloader::tinyobjloader)
