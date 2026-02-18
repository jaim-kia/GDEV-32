/******************************************************************************
 * Use WASD keys to move the main camera, QE to move up and down; use left shift to move faster
 * Use mouse to look around; use scroll wheel to adjust spotlight cutoff angles
 * Press F to switch to fill mode, L to switch to line mode, P to switch to point mode
 * Press R to reset camera position and orientation, Z/X to adjust main camera FOV
 * 
 * Press 1 to switch to main camera, 2 to switch to directional light camera;
 *  
 * To move spotlight, press 3 or 4 to switch to spotlight camera, then use 
 * the same set of keys to move the spotlight and mouse to adjust its direction; 
 *****************************************************************************/

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <gdev.h>

// change this to your desired window attributes
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE  "Exercise 2"
GLFWwindow *pWindow;

// models
std::vector<float> station = {};

// OpenGL object IDs
GLuint vao;
GLuint vbo;
GLuint shader;
GLuint texture[2];

double previousTime = 0.0;

struct Light;

struct Camera {
    glm::vec3 position = glm::vec3(0.0f, 3.0f, 5.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;
    float pitch = 0.0f;
    float fov = 90.0f;

    // owner pointer
    struct Light* owner = nullptr;
};

struct Light {
    enum Type {
        DIRECTIONAL,
        POINT,
        SPOTLIGHT
    } type = DIRECTIONAL;

    glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 specular = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    float specular_exponent = 32.0f;

    float inner_cutoff = 0.0f; // for spotlight
    float outer_cutoff = 0.0f; // for spotlight
    
    // for attenuation (for point and spotlight)
    float constant = 1.0f; 
    float linear = 0.09f;
    float quadratic = 0.032f;
    
    Camera cam;

    Light() {
        cam.owner = this;
    }

    glm::vec3 getPosition() {
        return cam.position;
    }
    glm::vec3 getDirection() {
        return cam.front;
    }
};

Camera main_camera;
Light main_light;
Light spotlights[2];

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

void setupLights() {
    main_light.cam.front = glm::vec3(-0.2f, -1.0f, -0.3f);
    for (int i = 0; i < 2; i++) {
        // spotlights[i].cam.position = glm::vec3(0.f, 5.0f, -5.0f + i * 10.0f);
        // spotlights[i].cam.front = glm::vec3(0.0f, -1.0f, 0.0f);
        spotlights[i].type = Light::SPOTLIGHT;
        spotlights[i].inner_cutoff = 12.0f;
        spotlights[i].outer_cutoff = 17.0f;
    }
}

// called by the main function to do initial setup, such as uploading vertex
// arrays, shader programs, etc.; returns true if successful, false otherwise
bool setup()
{
    readModelData(station, "station_data.txt");

    setupLights();

    // upload the model to the GPU (explanations omitted for brevity)
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, station.size() * sizeof(float), station.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) 0);                     // position
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (3 * sizeof(float)));   // texture coord
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (5 * sizeof(float)));   // normal
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (8 * sizeof(float)));   // tangent
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    // load our shader program
    shader = gdevLoadShader("demo5n.vs", "demo5n.fs");
    if (! shader)
        return false;

    // since we now use multiple textures, we need to set the texture channel for each texture
    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "diffuseMap"), 0);
    glUniform1i(glGetUniformLocation(shader, "normalMap"),  1);

    // load our textures
    texture[0] = gdevLoadTexture("tex-station.png", GL_REPEAT, true, true);
    texture[1] = gdevLoadTexture("TrainStationNormal.png", GL_REPEAT, true, true);
    if (! texture[0] || ! texture[1])
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
    projectionTransform = glm::perspective(glm::radians(active_camera->fov),      // fov
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
    glm::vec3 viewDir = glm::mat3(viewTransform) * main_light.getDirection();
    glUniform3fv(glGetUniformLocation(shader, "dir_light.direction"),
                 1, glm::value_ptr(viewDir));
    glUniform3fv(glGetUniformLocation(shader, "dir_light.ambient"),
                 1, glm::value_ptr(main_light.ambient));
    glUniform3fv(glGetUniformLocation(shader, "dir_light.diffuse"),
                 1, glm::value_ptr(main_light.diffuse));
    glUniform3fv(glGetUniformLocation(shader, "dir_light.specular"),
                 1, glm::value_ptr(main_light.specular));

    for (int i = 0; i < 2; i++) {
        std::string base = "spotlights[" + std::to_string(i) + "].";
        
        glm::vec3 posView = glm::vec3(viewTransform * glm::vec4(spotlights[i].getPosition(), 1.0));
        glUniform3fv(glGetUniformLocation(shader, (base + "position").c_str()),
                    1, glm::value_ptr(posView));

        glm::vec3 dirView = glm::mat3(viewTransform) * spotlights[i].getDirection();
        glUniform3fv(glGetUniformLocation(shader, (base + "direction").c_str()),
                    1, glm::value_ptr(dirView));

        glUniform1f(glGetUniformLocation(shader, (base + "innerCutoff").c_str()), glm::cos(glm::radians(spotlights[i].inner_cutoff)));
        
        glUniform1f(glGetUniformLocation(shader, (base + "outerCutoff").c_str()), glm::cos(glm::radians(spotlights[i].outer_cutoff)));
        
        glUniform1f(glGetUniformLocation(shader, (base + "constant").c_str()), spotlights[i].constant);
        
        glUniform1f(glGetUniformLocation(shader, (base + "linear").c_str()), spotlights[i].linear);
        
        glUniform1f(glGetUniformLocation(shader, (base + "quadratic").c_str()), spotlights[i].quadratic);

        glUniform3fv(glGetUniformLocation(shader, (base + "ambient").c_str()),
                     1, glm::value_ptr(spotlights[i].ambient));

        glUniform3fv(glGetUniformLocation(shader, (base + "diffuse").c_str()),
                     1, glm::value_ptr(spotlights[i].diffuse));

        glUniform3fv(glGetUniformLocation(shader, (base + "specular").c_str()),
                     1, glm::value_ptr(spotlights[i].specular));
    }


    // ... set the active textures...
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[1]);

    // ... then draw our triangles
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, station.size() / 11);
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

    if (glfwGetKey(pWindow, GLFW_KEY_3) == GLFW_PRESS) {
        active_camera = &spotlights[0].cam;
    }
    if (glfwGetKey(pWindow, GLFW_KEY_4) == GLFW_PRESS) {
        active_camera = &spotlights[1].cam;
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
    if (active_camera->owner && active_camera->owner->type == Light::SPOTLIGHT) {
        Light* light = active_camera->owner;
    
        float spread = 5.0f; 
        light->inner_cutoff += (float)yoffset;
        if (light->inner_cutoff < 1.0f) light->inner_cutoff = 1.0f;
        if (light->inner_cutoff > (90.0f - spread)) light->inner_cutoff = 90.0f - spread;

        // force the outer cutoff to always be exactly 'spread' degrees larger
        light->outer_cutoff = light->inner_cutoff + spread;
    }
    else {
        active_camera->fov -= (float)yoffset;
        if (active_camera->fov < 1.0f) {
            active_camera->fov = 1.0f;
        }
        if (active_camera->fov > 90.0f) {
            active_camera->fov = 90.0f;
    }
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
