/******************************************************************************
 * This demo is a modification of demo5.cpp to implement normal mapping,
 * simulating bumpy surfaces.
 *
 * The vertex data now includes tangent vectors (in addition to normals), and
 * the texture code is upgraded to load a diffuse map and a normal map at the
 * same time.
 *
 * (Note that the shader code is also updated -- see demo5n.vs and demo5n.fs.)
 *
 * TIP: To help you understand the code better, I highly recommend that you
 * view the changes between demo5 and demo5n in VS Code by doing the following:
 *
 * 1. Right-click demo5.cpp in VS Code's Explorer pane and click
 *    "Select for Compare".
 * 2. Right-click the demo5n.cpp and click "Compare with Selected".
 *
 * (Do the same for demo5.vs/demo5n.vs and demo5.fs/demo5n.fs.)
 *
 * Happy hacking! - eric
 *****************************************************************************/

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <gdev.h>

// change this to your desired window attributes
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE  "Hello Lighting (use WASDQE keys for camera, IKJLUO keys for light)"
GLFWwindow *pWindow;

// model
float vertices[] =
{
    // position (x, y, z)    normal (x, y, z)     tangent (x, y, z)    texture coordinates (s, t)

    // ground plane
    -8.00f, -2.00f,  8.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     8.00f, -2.00f,  8.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  4.0f, 0.0f,
     8.00f, -2.00f, -8.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  4.0f, 4.0f,
    -8.00f, -2.00f,  8.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     8.00f, -2.00f, -8.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  4.0f, 4.0f,
    -8.00f, -2.00f, -8.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 4.0f,

    // cube top
    -1.00f,  1.00f,  1.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     1.00f,  1.00f,  1.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     1.00f,  1.00f, -1.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -1.00f,  1.00f,  1.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     1.00f,  1.00f, -1.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -1.00f,  1.00f, -1.00f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,

    // cube bottom
    -1.00f, -1.00f, -1.00f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     1.00f, -1.00f, -1.00f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     1.00f, -1.00f,  1.00f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -1.00f, -1.00f, -1.00f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     1.00f, -1.00f,  1.00f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -1.00f, -1.00f,  1.00f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,

    // cube front
    -1.00f, -1.00f,  1.00f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     1.00f, -1.00f,  1.00f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     1.00f,  1.00f,  1.00f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -1.00f, -1.00f,  1.00f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     1.00f,  1.00f,  1.00f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -1.00f,  1.00f,  1.00f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,

    // cube back
     1.00f, -1.00f, -1.00f,  0.0f,  0.0f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -1.00f, -1.00f, -1.00f,  0.0f,  0.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -1.00f,  1.00f, -1.00f,  0.0f,  0.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     1.00f, -1.00f, -1.00f,  0.0f,  0.0f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -1.00f,  1.00f, -1.00f,  0.0f,  0.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     1.00f,  1.00f, -1.00f,  0.0f,  0.0f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,

    // cube right
     1.00f, -1.00f,  1.00f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     1.00f, -1.00f, -1.00f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     1.00f,  1.00f, -1.00f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     1.00f, -1.00f,  1.00f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     1.00f,  1.00f, -1.00f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     1.00f,  1.00f,  1.00f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

    // cube left
    -1.00f, -1.00f, -1.00f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
    -1.00f, -1.00f,  1.00f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
    -1.00f,  1.00f,  1.00f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -1.00f, -1.00f, -1.00f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
    -1.00f,  1.00f,  1.00f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -1.00f,  1.00f, -1.00f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f
};

std::vector<float> station = {};
std::vector<float> train = {};
std::vector<float> water = {};

// OpenGL object IDs
GLuint vao;
GLuint vbo;
GLuint shader;
GLuint simple_shader;
GLuint texture[6];

int vertex_data_num =  3;
GLuint vaos[3], vbos[3];
std::vector<float> vertex_data[3];
size_t data_sizes[3];

double previousTime = 0.0;

struct Camera {
    glm::vec3 position = glm::vec3(0.0f, 3.0f, 5.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;
    float pitch = 0.0f;
    float fov = 90.0f;
};

struct Light {
    Camera cam;
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

    float ambient;
    float diffuse;
    float specular_exponent;

    glm::vec3 getPosition() {
        return cam.position;
    }
    glm::vec3 getDirection() {
        return cam.front;
    }
};

Camera main_camera;
Light main_light;

Camera* active_camera = &main_camera;

// mouse input tracking variables
float lastX = WINDOW_WIDTH/2.0f;
float lastY = WINDOW_HEIGHT/2.0f;
bool firstMouse = true;

// helper function for reading model data from a file
void readModelData(std::vector<float> &array, const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string value;
        std::istringstream tokenizer(line);
        while (std::getline(tokenizer, value, ',')) {
            try {
                array.push_back(std::stof(value));
            } catch (const std::invalid_argument& e) {
                // skip
            }
        }
    }
    
}

// called by the main function to do initial setup, such as uploading vertex
// arrays, shader programs, etc.; returns true if successful, false otherwise
bool setup()
{
    readModelData(station, "station_data.txt");
    readModelData(train, "train_data.txt");
    readModelData(water, "water_data.txt");

    vertex_data[0] = station;
    vertex_data[1] = train;
    vertex_data[2] = water;

    // upload the model to the GPU (explanations omitted for brevity)
    glGenVertexArrays(vertex_data_num, vaos);
    glGenBuffers(vertex_data_num, vbos);

    for (int i = 0; i < vertex_data_num; ++i) {
        glBindVertexArray(vaos[i]);
        glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
        
        // std::cout << vertex_data[i].data() << std::endl;
        // std::cout << vertex_data[i].size() << std::endl;
        glBufferData(GL_ARRAY_BUFFER, vertex_data[i].size() * sizeof(float), vertex_data[i].data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) 0);                     // position
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (3 * sizeof(float)));   // texture coord
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (5 * sizeof(float)));   // normal
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (8 * sizeof(float)));   // tangent

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
    }

    // load our shader program
    shader = gdevLoadShader("demo5n.vs", "demo5n.fs");
    simple_shader = gdevLoadShader("Exercise2-simple.vs", "Exercise2-simple.fs");
    if (!shader || !simple_shader) return false;

    // since we now use multiple textures, we need to set the texture channel for each texture
    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "diffuseMap"), 0);
    glUniform1i(glGetUniformLocation(shader, "normalMap"),  1);
    glUniform1i(glGetUniformLocation(shader, "specularMap"),  2);

    glUseProgram(simple_shader);
    glUniform1i(glGetUniformLocation(simple_shader, "texture_file"), 0);


    // load our textures
    texture[0] = gdevLoadTexture("tex-station.png", GL_REPEAT, true, true);
    texture[1] = gdevLoadTexture("TrainStationNormal.png", GL_REPEAT, true, true);
    texture[2] = gdevLoadTexture("TrainStationSpecular.png", GL_REPEAT, true, true);
    texture[3] = gdevLoadTexture("tex-train.png", GL_REPEAT, true, true);
    texture[4] = gdevLoadTexture("tex-water.png", GL_REPEAT, true, true);
    texture[5] = gdevLoadTexture("tex-disp.png", GL_REPEAT, true, true);
    if (! texture[0] || ! texture[1] || !texture[2] || !texture[3] || !texture[4] || !texture[5])
        return false;

    // enable z-buffer depth testing and face culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    return true;
}

// called by the main function to do rendering per frame
void render()
{
    // clear the whole frame
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // using our shader program...
    glUseProgram(shader);

    // ... set up the projection matrix...
    glm::mat4 projectionTransform;
    projectionTransform = glm::perspective(glm::radians(active_camera->fov),                   // fov
                                           (float) WINDOW_WIDTH / WINDOW_HEIGHT,  // aspect ratio
                                           0.1f,                                  // near plane
                                           100.0f);                               // far plane
    glUniformMatrix4fv(glGetUniformLocation(shader, "projectionTransform"),
                       1, GL_FALSE, glm::value_ptr(projectionTransform));

    // ... set up the view matrix...
    glm::mat4 viewTransform;
    viewTransform = glm::lookAt(active_camera->position,                // eye position
                                active_camera->position + active_camera->front,   // center position
                                active_camera->up);  // up vector
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewTransform"),
                       1, GL_FALSE, glm::value_ptr(viewTransform));


    // ... set up the model matrix... (just identity for this demo)
    glm::mat4 modelTransform = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "modelTransform"),
                       1, GL_FALSE, glm::value_ptr(modelTransform));

    // ... set up the light position...
    glUniform3fv(glGetUniformLocation(shader, "lightPosition"),
                 1, glm::value_ptr(main_light.cam.position));

    // Drawing Station
    // ... set the active textures...
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture[2]);

    // ... then draw our triangles
    glBindVertexArray(vaos[0]);
    glDrawArrays(GL_TRIANGLES, 0, station.size() / 11);

    // Drawing Train
    glUseProgram(simple_shader);
    glUniformMatrix4fv(glGetUniformLocation(simple_shader, "projectionTransform"), 1, GL_FALSE, glm::value_ptr(projectionTransform));
    glUniformMatrix4fv(glGetUniformLocation(simple_shader, "viewTransform"), 1, GL_FALSE, glm::value_ptr(viewTransform));
    glUniformMatrix4fv(glGetUniformLocation(simple_shader, "modelTransform"), 1, GL_FALSE, glm::value_ptr(modelTransform));

    glUniform3fv(glGetUniformLocation(simple_shader, "lightPosition"), 1, glm::value_ptr(main_light.cam.position));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[3]); 
    glUniform1i(glGetUniformLocation(simple_shader, "diffuseMap"), 0);

    glBindVertexArray(vaos[1]);
    glDrawArrays(GL_TRIANGLES, 0, train.size() / 11);

    // Floor:
    glUseProgram(simple_shader);
    float currentTime = (float)glfwGetTime();
    glUniform1f(glGetUniformLocation(simple_shader, "time"), currentTime);

    glm::mat4 floorModel = glm::mat4(1.0f);
    floorModel = glm::translate(floorModel, glm::vec3(active_camera->position.x, 0.0f, active_camera->position.z));
    floorModel = glm::scale(floorModel, glm::vec3(10.0f, 1.0f, 10.0f));
    
    glUniformMatrix4fv(glGetUniformLocation(simple_shader, "modelTransform"), 1, GL_FALSE, glm::value_ptr(floorModel));
    glUniform1i(glGetUniformLocation(simple_shader, "isTile"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[4]); 
    glUniform1i(glGetUniformLocation(simple_shader, "diffuseMap"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[5]);
    glUniform1i(glGetUniformLocation(simple_shader, "shaderTextureSmoke"), 1);

    glBindVertexArray(vaos[2]);
    glDrawArrays(GL_TRIANGLES, 0, water.size() / 11);

    glUniform1i(glGetUniformLocation(simple_shader, "isTile"), 0);

}

/*****************************************************************************/

// input handling function for controlling the camera; called by the main function every frame
void processInput(GLFWwindow *pWindow, float deltaTime) {
    float cameraSpeed = 1.5f * deltaTime;

    if (glfwGetKey(pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        cameraSpeed *= 2.0f;
    }

    if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS) {
        active_camera->position += active_camera->front * cameraSpeed;
    } 
    
    if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS) {
        active_camera->position += -active_camera->front * cameraSpeed;
    } 

    if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS) {
        active_camera->position += glm::cross(active_camera->front, active_camera->up) * cameraSpeed;
    }

    if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS) {
        active_camera->position += -glm::cross(active_camera->front, active_camera->up) * cameraSpeed;
    }

    if (glfwGetKey(pWindow, GLFW_KEY_Q) == GLFW_PRESS) {
        active_camera->position += active_camera->up * cameraSpeed;
    }

    if (glfwGetKey(pWindow, GLFW_KEY_E) == GLFW_PRESS) {
        active_camera->position += -active_camera->up * cameraSpeed;
    }

    if (glfwGetKey(pWindow, GLFW_KEY_R) == GLFW_PRESS) {
        active_camera->position = glm::vec3(0.0f, 0.0f, 3.0f);
        active_camera->yaw = -90.0f;
        active_camera->pitch = 0.0f;
        active_camera->fov = 45.0f;
        active_camera->front = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    if (glfwGetKey(pWindow, GLFW_KEY_Z) == GLFW_PRESS) {
        active_camera->fov += 30.0f * deltaTime;
        if (active_camera->fov > 90.0f) {
            active_camera->fov = 90.0f;
        }
    }

    if (glfwGetKey(pWindow, GLFW_KEY_X) == GLFW_PRESS) {
        active_camera->fov -= 30.0f * deltaTime;
        if (active_camera->fov < 1.0f) {
            active_camera->fov = 1.0f;
        }
    }
    
    if (glfwGetKey(pWindow, GLFW_KEY_F) == GLFW_PRESS) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    }

    if (glfwGetKey(pWindow, GLFW_KEY_L) == GLFW_PRESS) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }

    if (glfwGetKey(pWindow, GLFW_KEY_P) == GLFW_PRESS) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
    }

    if (glfwGetKey(pWindow, GLFW_KEY_1) == GLFW_PRESS) {
        active_camera = &main_camera;
    }
    
    if (glfwGetKey(pWindow, GLFW_KEY_2) == GLFW_PRESS) {
        active_camera = &main_light.cam;
    }


}

void mouse_callback(GLFWwindow* pWindow, double xpos, double ypos) {
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos-lastX;
    float yoffset = lastY - ypos; // reverse cause y is reversed in window space;

    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.1f;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    active_camera->yaw += xoffset;
    active_camera->pitch += yoffset;

    if (active_camera->pitch > 89.0f) {
        active_camera->pitch = 89.0f;
    }
    if (active_camera->pitch < -89.0f) {
        active_camera->pitch = -89.0f;
    }

    glm::vec3 cam_dir;
    cam_dir.x = cos(glm::radians(active_camera->yaw)) * cos(glm::radians(active_camera->pitch));
    cam_dir.y = sin(glm::radians(active_camera->pitch));
    cam_dir.z = sin(glm::radians(active_camera->yaw)) * cos(glm::radians(active_camera->pitch));
    active_camera->front = glm::normalize(cam_dir);
    
}

void scroll_callback(GLFWwindow *pWindow, double xoffset, double yoffset) {
    active_camera->fov -= (float)yoffset;
    if (active_camera->fov < 1.0f) {
        active_camera->fov = 1.0f;
    }
    if (active_camera->fov > 90.0f) {
        active_camera->fov = 90.0f;
    }

}

// handler called by GLFW when there is a keyboard event
void handleKeys(GLFWwindow* pWindow, int key, int scancode, int action, int mode)
{
    // pressing Esc closes the window
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(pWindow, GL_TRUE);
}

// handler called by GLFW when the window is resized
void handleResize(GLFWwindow* pWindow, int width, int height)
{
    // tell OpenGL to do its drawing within the entire "client area" (area within the borders) of the window
    glViewport(0, 0, width, height);
}

// main function
int main(int argc, char** argv)
{
    // initialize GLFW and ask for OpenGL 3.3 core
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // create a GLFW window with the specified width, height, and title
    pWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (! pWindow)
    {
        // gracefully terminate if we cannot create the window
        std::cout << "Cannot create the GLFW window.\n";
        glfwTerminate();
        return -1;
    }

    // make the window the current context of subsequent OpenGL commands,
    // and enable vertical sync and aspect-ratio correction on the GLFW window
    glfwMakeContextCurrent(pWindow);
    glfwSwapInterval(1);
    glfwSetWindowAspectRatio(pWindow, WINDOW_WIDTH, WINDOW_HEIGHT);

    // set up callback functions to handle window system events
    glfwSetKeyCallback(pWindow, handleKeys);
    glfwSetFramebufferSizeCallback(pWindow, handleResize);

    // don't miss any momentary keypresses
    glfwSetInputMode(pWindow, GLFW_STICKY_KEYS, GLFW_TRUE);

    // set up callback functions to handle mouse events
    glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
    glfwSetCursorPosCallback(pWindow, mouse_callback);
    glfwSetScrollCallback(pWindow, scroll_callback);

    // initialize GLAD, which acts as a library loader for the current OS's native OpenGL library
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    float delta;
    float last_frame = 0.0f;
    // if our initial setup is successful...
    if (setup())
    {
        // do rendering in a loop until the user closes the window
        while (! glfwWindowShouldClose(pWindow))
        {
            // render our next frame
            // (by default, GLFW uses double-buffering with a front and back buffer;
            // all drawing goes to the back buffer, so the frame does not get shown yet)
            float current_frame = glfwGetTime();
            delta = current_frame - last_frame;
            last_frame = current_frame;
            processInput(pWindow, delta);
            render();

            // swap the GLFW front and back buffers to show the next frame
            glfwSwapBuffers(pWindow);

            // process any window events (such as moving, resizing, keyboard presses, etc.)
            glfwPollEvents();
        }
    }

    // gracefully terminate the program
    glfwTerminate();
    return 0;
}
