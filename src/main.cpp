#define GL_SILENCE_DEPRECATION

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#include <OpenGL/gl3.h>
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef __EMSCRIPTEN__
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#endif

#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#endif

struct Star {
    glm::vec3 position;
    float rest_wavelength;
    float rest_intensity;
};

struct Simulation {
    glm::vec3 camera_front{0.0f, 0.0f, -1.0f};
    float yaw = -90.0f;
    float pitch = 0.0f;
    float last_x = 640.0f;
    float last_y = 360.0f;
    bool first_mouse = true;

    glm::vec3 ship_position{0.0f};
    float beta = 0.0f;
    float target_beta = 0.0f;
    float acceleration = 0.35f;
    float time_scale = 1.0f;
    float fov = 45.0f;
    float exposure = 1.0f;
    float point_size = 4.0f;
    float fog_start = 800.0f;
    float fog_end = 1000.0f;
    bool paused = false;
    bool show_panel = true;
    double coordinate_time = 0.0;
    double proper_time = 0.0;
};

Simulation sim;
GLFWwindow* window = nullptr;
GLuint shader_program = 0;
GLuint vao = 0;
GLuint vbo = 0;
GLint view_projection_location = -1;
GLint ship_position_location = -1;
GLint beta_location = -1;
GLint fov_location = -1;
GLint exposure_location = -1;
GLint point_size_location = -1;
GLint fog_start_location = -1;
GLint fog_end_location = -1;
constexpr int NUM_STARS = 100000;

std::string read_text_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Could not open file: " + path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string shader_source(const std::string& path, const std::string& version) {
    return version + "\n" + read_text_file(path);
}

GLuint compile_shader(GLenum type, const std::string& source, const std::string& path) {
    GLuint shader = glCreateShader(type);
    const char* ptr = source.c_str();
    glShaderSource(shader, 1, &ptr, nullptr);
    glCompileShader(shader);
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<size_t>(len), '\0');
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Shader compilation failed (" + path + "):\n" + log);
    }
    return shader;
}

GLuint create_shader_program() {
#ifdef __EMSCRIPTEN__
    const std::string version = "#version 300 es\nprecision highp float;";
#else
    const std::string version = "#version 410 core";
#endif
    GLuint vs = compile_shader(GL_VERTEX_SHADER, shader_source("shaders/star.vert", version), "shaders/star.vert");
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, shader_source("shaders/star.frag", version), "shaders/star.frag");
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<size_t>(len), '\0');
        glGetProgramInfoLog(program, len, nullptr, log.data());
        glDeleteProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        throw std::runtime_error("Shader program linking failed:\n" + log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

std::vector<Star> generate_universe(int count) {
    std::vector<Star> stars;
    stars.reserve(static_cast<size_t>(count));
    std::mt19937 generator(42); // deterministic so native/web look identical
    std::uniform_real_distribution<float> pos(-1000.0f, 1000.0f);
    std::uniform_real_distribution<float> wavelength(300.0f, 750.0f);
    std::uniform_real_distribution<float> intensity(0.5f, 1.5f);
    for (int i = 0; i < count; ++i) {
        stars.push_back({glm::vec3(pos(generator), pos(generator), pos(generator)), wavelength(generator), intensity(generator)});
    }
    return stars;
}

void reset_simulation() {
    sim.ship_position = glm::vec3(0.0f);
    sim.beta = 0.0f;
    sim.target_beta = 0.0f;
    sim.yaw = -90.0f;
    sim.pitch = 0.0f;
    sim.camera_front = glm::vec3(0.0f, 0.0f, -1.0f);
    sim.paused = false;
    sim.coordinate_time = 0.0;
    sim.proper_time = 0.0;
}

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (sim.show_panel && !glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)) return;
    if (sim.first_mouse) {
        sim.last_x = static_cast<float>(xpos);
        sim.last_y = static_cast<float>(ypos);
        sim.first_mouse = false;
    }
    float xoffset = static_cast<float>(xpos) - sim.last_x;
    float yoffset = sim.last_y - static_cast<float>(ypos);
    sim.last_x = static_cast<float>(xpos);
    sim.last_y = static_cast<float>(ypos);
    constexpr float sensitivity = 0.1f;
    sim.yaw += xoffset * sensitivity;
    sim.pitch = glm::clamp(sim.pitch + yoffset * sensitivity, -89.0f, 89.0f);
    glm::vec3 front;
    front.x = std::cos(glm::radians(sim.yaw)) * std::cos(glm::radians(sim.pitch));
    front.y = std::sin(glm::radians(sim.pitch));
    front.z = std::sin(glm::radians(sim.yaw)) * std::cos(glm::radians(sim.pitch));
    sim.camera_front = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow*, int width, int height) { glViewport(0, 0, width, height); }

#ifdef __EMSCRIPTEN__
extern "C" {
EMSCRIPTEN_KEEPALIVE void set_target_beta(float value) { sim.target_beta = glm::clamp(value, 0.0f, 0.999999f); }
EMSCRIPTEN_KEEPALIVE void set_acceleration(float value) { sim.acceleration = glm::clamp(value, 0.01f, 2.0f); }
EMSCRIPTEN_KEEPALIVE void set_time_scale(float value) { sim.time_scale = glm::clamp(value, 0.0f, 10.0f); }
EMSCRIPTEN_KEEPALIVE void set_fov(float value) { sim.fov = glm::clamp(value, 25.0f, 100.0f); }
EMSCRIPTEN_KEEPALIVE void set_exposure(float value) { sim.exposure = glm::clamp(value, 0.1f, 4.0f); }
EMSCRIPTEN_KEEPALIVE void set_point_size(float value) { sim.point_size = glm::clamp(value, 1.0f, 10.0f); }
EMSCRIPTEN_KEEPALIVE void set_fog_distance(float value) { sim.fog_end = glm::clamp(value, 500.0f, 1400.0f); sim.fog_start = sim.fog_end * 0.8f; }
EMSCRIPTEN_KEEPALIVE void toggle_pause() { sim.paused = !sim.paused; }
EMSCRIPTEN_KEEPALIVE void reset_from_web() { reset_simulation(); }
EMSCRIPTEN_KEEPALIVE float get_beta() { return sim.beta; }
EMSCRIPTEN_KEEPALIVE float get_target_beta() { return sim.target_beta; }
EMSCRIPTEN_KEEPALIVE float get_ship_speed_c() { return sim.beta; }
EMSCRIPTEN_KEEPALIVE float get_ship_distance() { return glm::length(sim.ship_position); }
EMSCRIPTEN_KEEPALIVE float get_coordinate_time() { return static_cast<float>(sim.coordinate_time); }
EMSCRIPTEN_KEEPALIVE float get_proper_time() { return static_cast<float>(sim.proper_time); }
EMSCRIPTEN_KEEPALIVE float get_forward_doppler() {
    double b = sim.beta;
    double gamma = 1.0 / std::sqrt(std::max(1e-12, 1.0 - b * b));
    return static_cast<float>(gamma * (1.0 - b));
}
EMSCRIPTEN_KEEPALIVE float get_backward_doppler() {
    double b = sim.beta;
    double gamma = 1.0 / std::sqrt(std::max(1e-12, 1.0 - b * b));
    return static_cast<float>(gamma * (1.0 + b));
}
EMSCRIPTEN_KEEPALIVE float get_forward_beaming() {
    double d = get_forward_doppler();
    return static_cast<float>(1.0 / std::pow(std::max(d, 1e-12), 4.0));
}
EMSCRIPTEN_KEEPALIVE int get_paused() { return sim.paused ? 1 : 0; }
}
#endif

void handle_keyboard() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) sim.target_beta = glm::min(sim.target_beta + 0.01f, 0.999999f);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) sim.target_beta = glm::max(sim.target_beta - 0.01f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) reset_simulation();
    static bool space_was_down = false;
    bool space_down = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (space_down && !space_was_down) sim.paused = !sim.paused;
    space_was_down = space_down;
}

void update_simulation(float dt) {
    if (sim.paused) return;
    float response = 1.0f - std::exp(-sim.acceleration * dt);
    sim.beta += (sim.target_beta - sim.beta) * response;
    sim.coordinate_time += dt * sim.time_scale;
    double gamma = 1.0 / std::sqrt(std::max(1e-12, 1.0 - static_cast<double>(sim.beta) * sim.beta));
    sim.proper_time += (dt * sim.time_scale) / gamma;
    sim.ship_position += glm::vec3(0.0f, 0.0f, -5.0f) * sim.beta * dt * 60.0f * sim.time_scale;
}

#ifndef __EMSCRIPTEN__
void draw_native_ui() {
    if (!sim.show_panel) return;
    ImGui::SetNextWindowSize(ImVec2(390, 590), ImGuiCond_FirstUseEver);
    ImGui::Begin("Relativistic Flight Computer", &sim.show_panel);

    ImGui::TextColored(ImVec4(0.55f, 0.72f, 1.0f, 1.0f), "RFS-01 // RELATIVISTIC STARFIELD");
    ImGui::TextDisabled("Special-relativistic optical flight simulation");
    ImGui::Separator();

    ImGui::Text("VELOCITY CONTROL");
    ImGui::SliderFloat("Target beta (v/c)", &sim.target_beta, 0.0f, 0.999999f, "%.6f c");
    ImGui::SliderFloat("Acceleration response", &sim.acceleration, 0.01f, 2.0f, "%.2f");
    ImGui::SliderFloat("Time scale", &sim.time_scale, 0.0f, 10.0f, "%.1fx");

    double beta = sim.beta;
    double gamma = 1.0 / std::sqrt(std::max(1e-12, 1.0 - beta * beta));
    double speed_ms = beta * 299792458.0;
    double forward_doppler = gamma * (1.0 - beta);
    double backward_doppler = gamma * (1.0 + beta);
    double forward_beaming = 1.0 / std::pow(std::max(forward_doppler, 1e-12), 4.0);

    ImGui::Separator();
    ImGui::Text("FLIGHT TELEMETRY");
    ImGui::Text("Current velocity    %.6f c", beta);
    ImGui::Text("Speed               %.3e m/s", speed_ms);
    ImGui::Text("Speed               %.3e km/s", speed_ms / 1000.0);
    ImGui::Text("Lorentz factor      %.6f", gamma);
    ImGui::Text("Distance travelled  %.1f units", glm::length(sim.ship_position));
    ImGui::Text("Coordinate time     %.2f s", sim.coordinate_time);
    ImGui::Text("Ship proper time    %.2f s", sim.proper_time);

    ImGui::Separator();
    ImGui::Text("OPTICAL EFFECTS");
    ImGui::Text("Forward wavelength factor   %.6f", forward_doppler);
    ImGui::Text("Backward wavelength factor  %.6f", backward_doppler);
    ImGui::Text("Forward beaming factor      %.2fx", forward_beaming);
    ImGui::TextDisabled("< 1 = blueshift · > 1 = redshift");

    ImGui::Separator();
    ImGui::Text("VISUAL SYSTEMS");
    ImGui::SliderFloat("Field of view", &sim.fov, 25.0f, 100.0f, "%.1f deg");
    ImGui::SliderFloat("Exposure", &sim.exposure, 0.1f, 4.0f, "%.2fx");
    ImGui::SliderFloat("Star point size", &sim.point_size, 1.0f, 10.0f, "%.1f px");
    ImGui::SliderFloat("Fog horizon", &sim.fog_end, 500.0f, 1400.0f, "%.0f");
    sim.fog_start = sim.fog_end * 0.8f;

    if (ImGui::Button(sim.paused ? "Resume flight" : "Pause flight")) sim.paused = !sim.paused;
    ImGui::SameLine();
    if (ImGui::Button("Reset flight")) reset_simulation();
    ImGui::Separator();
    ImGui::TextWrapped("Mouse: look around  |  Up/Down: throttle  |  Space: pause  |  R: reset");
    ImGui::TextDisabled("Right mouse button enables look while this panel is open.");
    ImGui::End();
}
#endif

void render_frame(float dt) {
    handle_keyboard();
    update_simulation(dt);

    int width = 1, height = 1;
    glfwGetFramebufferSize(window, &width, &height);
    float aspect = height > 0 ? static_cast<float>(width) / height : 16.0f / 9.0f;
    glm::mat4 projection = glm::perspective(glm::radians(sim.fov), aspect, 0.1f, 2000.0f);
    glm::mat4 view = glm::lookAt(sim.ship_position, sim.ship_position + sim.camera_front, glm::vec3(0, 1, 0));
    glm::mat4 vp = projection * view;

    glClearColor(0.005f, 0.008f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader_program);
    glUniform3fv(ship_position_location, 1, glm::value_ptr(sim.ship_position));
    glUniform1f(beta_location, sim.beta);
    glUniformMatrix4fv(view_projection_location, 1, GL_FALSE, glm::value_ptr(vp));
    glUniform1f(fov_location, sim.fov);
    glUniform1f(exposure_location, sim.exposure);
    glUniform1f(point_size_location, sim.point_size);
    glUniform1f(fog_start_location, sim.fog_start);
    glUniform1f(fog_end_location, sim.fog_end);
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, NUM_STARS);

#ifndef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    draw_native_ui();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

    glfwSwapBuffers(window);
    glfwPollEvents();
}

#ifdef __EMSCRIPTEN__
void web_loop(void*) {
    static double last = 0.0;
    double now = emscripten_get_now() * 0.001;
    float dt = last > 0.0 ? static_cast<float>(glm::min(now - last, 0.1)) : 1.0f / 60.0f;
    last = now;
    render_frame(dt);
}
#endif

int main() {
    try {
        if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");
#ifdef __EMSCRIPTEN__
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        window = glfwCreateWindow(1280, 720, "Relativistic Starfield", nullptr, nullptr);
        if (!window) throw std::runtime_error("Failed to create GLFW window");
        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
#ifndef __EMSCRIPTEN__
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSwapInterval(1);
#endif

        auto stars = generate_universe(NUM_STARS);
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(stars.size() * sizeof(Star)), stars.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Star), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Star), reinterpret_cast<void*>(offsetof(Star, rest_wavelength)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Star), reinterpret_cast<void*>(offsetof(Star, rest_intensity)));
        glEnableVertexAttribArray(2);

        shader_program = create_shader_program();
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        view_projection_location = glGetUniformLocation(shader_program, "viewProjection");
        ship_position_location = glGetUniformLocation(shader_program, "ship_position");
        beta_location = glGetUniformLocation(shader_program, "beta");
        fov_location = glGetUniformLocation(shader_program, "fov");
        exposure_location = glGetUniformLocation(shader_program, "exposure");
        point_size_location = glGetUniformLocation(shader_program, "point_size");
        fog_start_location = glGetUniformLocation(shader_program, "fog_start");
        fog_end_location = glGetUniformLocation(shader_program, "fog_end");

#ifndef __EMSCRIPTEN__
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");
#endif

#ifdef __EMSCRIPTEN__
        emscripten_set_main_loop_arg(web_loop, nullptr, 0, true);
#else
        double last_time = glfwGetTime();
        while (!glfwWindowShouldClose(window)) {
            double now = glfwGetTime();
            float dt = static_cast<float>(glm::min(now - last_time, 0.1));
            last_time = now;
            render_frame(dt);
        }
#endif

#ifndef __EMSCRIPTEN__
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
#endif
        glDeleteProgram(shader_program);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
}
