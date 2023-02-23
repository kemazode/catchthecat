#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>
#include <glm/glm.hpp>
#include <iostream>
#include <thread>

#include "util.hpp"
#include "Shader.h"
#include "map.hpp"
#include "sound.hpp"

#define FLIP_TIME 1.0f
#define DISAPPEARING_TIME 2.0f

GLuint createTexture(const char *file_name);
void mouseCallback(GLFWwindow* window, int button, int action, int mods);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void cursorCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void windowScale(GLFWwindow* window, int w, int h);
void doFlip(glm::mat4& model, GLfloat degree);
void do_movement();

float mouse_x_NDC(double x);
float mouse_y_NDC(double y);

GLfloat sin30 = 0.5f;

// GL_TRIANGLE_FAN
GLfloat hexagon[] = {
    0.0f, -1.0f, 0.0f,   0.5f, 0.0f,
   -1.0f, -0.5f, 0.0f,   0.0f, sin30*0.5f,
   -1.0f,  0.5f, 0.0f,   0.0f, sin30*0.5f + 0.5f,
    0.0f,  1.0f, 0.0f,   0.5f, 1.0f,
    1.0f,  0.5f, 0.0f,   1.0f, sin30*0.5f + 0.5f,
    1.0f, -0.5f, 0.0f,   1.0f, sin30*0.5f,
};

static Map game_map;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

GLfloat lastX = 400, lastY = 300;
GLfloat yaw = -90.0f;
GLfloat pitch = 0.0f;
GLfloat fov = 45.0f;

GLuint WIDTH = 800, HEIGHT = 600;

const GLfloat scale = 0.60f;
const GLfloat edge = 0.30f;

const GLfloat map_stride_x = 1.0f + 0.5f + edge + 0.5f;
const GLfloat map_stride_y = 1.0f + 0.5f + edge;
const GLfloat shift = 1.0f + edge / 2;

const GLfloat x_width = game_map.width() * map_stride_x * scale;
const GLfloat y_width = game_map.height() * map_stride_y * scale;
const GLfloat x_offset = (2.0f - x_width) / 2 + -1.0f;
const GLfloat y_offset = (2.0f - y_width) / 2 + -1.0f;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

const glm::vec3
#ifdef PATH_HIGHLIGHT
        way_highlight_color(1.0f, 0.0f, 1.0f),
#endif
        prohibited_color(0.2f, 0.2f, 0.2f),
        regular_color(0.0f, 1.0f, 1.0f),
        wall_color(1.0f, 0.5f, 0.0f);

class BoardNavigation {
    Position current;
    const Map &map;

    void Validate(Position new_p) {
        if (map.within(new_p))
            current = new_p;
    }

public:
    BoardNavigation(const Map &m) : current{0, 0}, map(m) {}

    Position getPosition() const
    { return current; }

    void Up()
    { Validate(Position{current.i + 1, current.j}); }

    void Down()
    { Validate(Position{current.i - 1, current.j}); }

    void Left()
    { Validate(Position{current.i, current.j - 1}); }

    void Right()
    { Validate(Position{current.i, current.j + 1}); }

} board_navigation(game_map);

class Timer {
    GLfloat begin;
    mutable GLfloat end;
    GLfloat max;

    bool locked = false;

    void Update() const {
        end = glfwGetTime();
    }
public:
    Timer() = default;

    void SetTime(GLfloat seconds) {
        max = seconds;
    }

    void Start() {
        if (not locked)
            begin = glfwGetTime();
    }

    float GetTime() const {
        Update();
        return end - begin;
    }

    bool Running() const {
        Update();
        return GetTime() < max;
    }

    void Lock() {
        locked = true;
    }

    void Unlock() {
        locked = false;
    }
};

class SourceWithLock {
    Source m_source;
    bool m_lock;

public:
    template <class... Args>
    SourceWithLock(Args&& ...args) : m_source(std::forward<Args>(args)...), m_lock(false) {}
    void lock() { m_lock = true; }
    void unlock() { m_lock = false; }
    void playAsync() {
        if (not m_lock)
            m_source.playAsync();
    }
};

Timer timer;

SourceWithLock *source_with_lock_cat = nullptr;
Source *source_wall = nullptr;
Source *source_restart = nullptr;

bool keys[1024];

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Catch The Cat", nullptr, nullptr);
    assert(window);

    glfwMakeContextCurrent(window);
    glewExperimental = true;
    assert(glewInit() == GLEW_OK);

    glfwSetMouseButtonCallback(window, mouseCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetFramebufferSizeCallback(window, windowScale);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);

    SoundSystem& sound_system = SoundSystem::getInstance();
    sound_system.init();

    Sound lose_sound(PATH_TO("lose.wav"));
    SourceWithLock source_with_lock_cat_itself(lose_sound, 0.0f, 0.0f, 0.0f);
    source_with_lock_cat = &source_with_lock_cat_itself;

    Sound wall_sound(PATH_TO("wall.wav"));
    Source source_wall_itself(wall_sound, 0.0f, 0.0f, 0.0f);
    source_wall = &source_wall_itself;

    Sound restart_sound(PATH_TO("restart.wav"));
    Source source_restart_itself(restart_sound, 0.0f, 0.0f, 0.0f);
    source_restart = &source_restart_itself;


//    glEnable(GL_CULL_FACE);
//    glCullFace(GL_FRONT);

    Program hexagon_shader(PATH_TO("vs.glsl"), PATH_TO("fs.glsl"));        

    GLuint cat_texture = createTexture(PATH_TO("cat.jpg"));

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(hexagon), hexagon, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::srand(std::time(nullptr)); // For map generation

    while (!glfwWindowShouldClose(window)) {
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        do_movement();
        glClear(GL_COLOR_BUFFER_BIT);

        hexagon_shader.use();
        glBindVertexArray(VAO);
        for (GLuint i = 0; i < game_map.height(); ++i)
            for (GLuint j = 0; j< game_map.width(); ++j) {

                glm::mat4 model(1), view(1), projection(1);
                auto &tile = game_map.at(i, j);

                view = lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

                projection = glm::perspective(glm::radians(fov), (GLfloat) WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);

                model = glm::translate(model, glm::vec3(x_offset, y_offset, 0.0f));
                model = glm::scale(model, glm::vec3(scale, scale, scale));
                model = glm::translate(model, glm::vec3(j * map_stride_x + shift * !(i & 1), i * map_stride_y, 0.0f));

#ifndef KEYBOARD_CONTROL
                // Assign appropriate coordinates
                for (int i = 0; i < 6; ++i) {
                    auto&& vec = projection * view * model * glm::vec4(hexagon[0 + i*5], hexagon[1 + i*5], hexagon[2 + i*5], 1.0f);
                    tile.v[i].x = vec.x;
                    tile.v[i].y = vec.y;

                    current = vec;
                }
#endif
#ifdef KEYBOARD_CONTROL
                auto curr = board_navigation.getPosition();
                game_map.select(curr);
#endif

                hexagon_shader.setUniform("Selected", bool(tile.opt & Option::selected));
                if (tile.type == HexType::regular) {

                    if (tile.opt & Option::prohibited)
                        hexagon_shader.setUniform("Color", prohibited_color);
#ifdef PATH_HIGHLIGHT
                    else if (tile.opt & Option::way_higlight)
                        hexagon_shader.setUniform("Color", way_highlight_color);

#endif
                    else
                        hexagon_shader.setUniform("Color", regular_color);

                } else if (tile.type == HexType::wall) {
                    hexagon_shader.setUniform("Color", wall_color);
                } else if (tile.type == HexType::cat) {

                    if (game_map.status() == Status::win) {
                        doFlip(model, 360.0f);
                    } else if (game_map.status() == Status::fail) {
                        hexagon_shader.setUniform("Color", regular_color);
                        if (timer.Running()) {
                            hexagon_shader.setUniform("DisappearingTexture", (1 - timer.GetTime()/DISAPPEARING_TIME));
                        } else {
                            timer.SetTime(DISAPPEARING_TIME);
                            timer.Start();                           
                            timer.Lock();
                            if (source_with_lock_cat) {
                                source_with_lock_cat->playAsync();
                                source_with_lock_cat->lock();
                            }

                        }
                    } else if (game_map.status() == Status::playing) {
                        hexagon_shader.setUniform("DisappearingTexture", 1.0f);
                    }

                    hexagon_shader.setUniform("TextureEnabled", true);
                    glBindTexture(GL_TEXTURE_2D, cat_texture);
                }

                hexagon_shader.setUniform("projection", projection);
                hexagon_shader.setUniform("view", view);
                hexagon_shader.setUniform("model", model);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
                glBindTexture(GL_TEXTURE_2D, 0);

                hexagon_shader.setUniform("TextureEnabled", false);
                hexagon_shader.setUniform("Selected", false);
#ifdef KEYBOARD_CONTROL
                game_map.deselect(curr);
#endif             
            }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwMakeContextCurrent(window);
    glfwTerminate();
    return 0;
}

void mouseCallback(GLFWwindow* window, int button, int action, int mods)
{
    (void) window;
    (void) button;
    (void) mods;

    if (action != GLFW_PRESS)
        return;

#ifndef KEYBOARD_CONTROL
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    game_map.clickOn(Point{mouse_x_NDC(xpos), mouse_y_NDC(ypos)});
#endif
}

void cursorCallback(GLFWwindow* window, double xpos, double ypos)
{
    (void) window;

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    GLfloat sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    front.y = sin(glm::radians(pitch));
    front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    cameraFront = glm::vec3(front);
#ifndef KEYBOARD_CONTROL
    game_map.enter(Point{mouse_x_NDC(xpos), mouse_y_NDC(ypos)});
#endif
}

GLuint createTexture(const char *file_name)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Set texture filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height;
    unsigned char* image = SOIL_load_image(file_name, &width, &height, 0, SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

float mouse_x_NDC(double x) { return 2 * x/WIDTH + -1.0f; }
float mouse_y_NDC(double y) { return 2 * (1.0f - y/HEIGHT) + -1.0f; }

void windowScale(GLFWwindow* window, int w, int h)
{
    (void) window;

    WIDTH = w;
    HEIGHT = h;
    glViewport(0, 0, w, h);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    (void) window;
    (void) scancode;
    (void) mode;
    bool success = false;

    if (action == GLFW_PRESS) {
#ifdef KEYBOARD_CONTROL
        switch (key) {
        case GLFW_KEY_UP: board_navigation.Up(); break;
        case GLFW_KEY_DOWN: board_navigation.Down(); break;
        case GLFW_KEY_LEFT: board_navigation.Left(); break;
        case GLFW_KEY_RIGHT: board_navigation.Right(); break;
        case GLFW_KEY_ENTER:
            success = game_map.setWall(board_navigation.getPosition());
            if (success && source_wall)
                source_wall->playAsync();
            break;
        case GLFW_KEY_R:
            game_map = Map();
            timer.Unlock();
            if (source_with_lock_cat)
                source_with_lock_cat->unlock();
            if (source_restart)
                source_restart->playAsync();
            break;
        }
#endif
        keys[key] = true;
    } else if (action == GLFW_RELEASE)
        keys[key] = false;

}

void do_movement() {
    GLfloat cameraSpeed = 5.0f * deltaTime;

    if(keys[GLFW_KEY_W])
      cameraPos += cameraSpeed * cameraFront;
    if(keys[GLFW_KEY_S])
      cameraPos -= cameraSpeed * cameraFront;
    if(keys[GLFW_KEY_A])
      cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if(keys[GLFW_KEY_D])
      cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void) window;
    (void) xoffset;

    if(fov >= 1.0f && fov <= 45.0f)
        fov -= yoffset;
    if(fov <= 1.0f)
        fov = 1.0f;
    if(fov >= 45.0f)
        fov = 45.0f;
}

void doFlip(glm::mat4& model, GLfloat degree) {
    if (timer.Running()) {
        model = glm::rotate(model, glm::radians(degree*(timer.GetTime()/FLIP_TIME)), glm::vec3(1.0f, 0.0f, 0.0f));
    } else {
        timer.SetTime(FLIP_TIME);
        timer.Start();
        timer.Lock();
    }
}
