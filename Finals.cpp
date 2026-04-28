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
 * use scroll wheel to adjust spotlight cutoff angles. Z/X to adjust outer cutoff angle.
 *
 * Press 5 to toggle shadows on/off
 * Press +/= to increase PCF sample quality (more samples = softer shadows)
 * Press - to decrease PCF sample quality (fewer samples = sharper shadows)
 * Press ] to increase PCF radius (larger sampling area = softer shadows)
 * Press [ to decrease PCF radius (smaller sampling area = sharper shadows)
 *****************************************************************************/

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <gdev.h>

// change this to your desired window attributes
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE  "GDEV32 Final Project - Gimena, Tan"
GLFWwindow *pWindow;

// models
std::vector<float> FloorMesh = {};
std::vector<float> BricksParallax = {};
std::vector<float> GrassMesh = {};
std::vector<float> LowerBuilding = {};
std::vector<float> LowerWindow = {};
std::vector<float> HigherBuilding = {};
std::vector<float> HigherWindow = {};
std::vector<float> TreeBark = {};
std::vector<float> TreeLeaves = {};
std::vector<float> MirrorPlane = {};
std::vector<float> SideStation = {};
std::vector<float> Office = {};
std::vector<float> BusStation = {};
std::vector<float> Miscellaneous = {};
std::vector<float> Water = {};
std::vector<float> TrainStation = {};
std::vector<float> TrainCart = {};
std::vector<float> LampPost = {};
std::vector<float> LampBulb = {};
std::vector<float> InstanceMesh = {};

// OpenGL object IDs
GLuint vao;
GLuint vbo;
GLuint instancedVao;
GLuint instancedVbo;
GLuint instancedVboMatrix;
GLuint shader;
GLuint texture[27];

int vertex_data_num =  20;
GLuint vaos[20], vbos[20];
std::vector<float> vertex_data[20];
size_t data_sizes[20];

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

    // for fireflies. meant to be overridden
    glm::vec3* externalPosition = nullptr;

    Light() {
        cam.owner = this;
    }

    Light(Type t) : type(t) {
        cam.owner = this;
    }

    glm::vec3 getPosition() {
        if (externalPosition) return *externalPosition;
        return cam.position;
    }
    glm::vec3 getDirection() {
        return cam.front;
    }
};

Camera main_camera;

Light main_light = Light(Light::DIRECTIONAL);
Light spotlight1 = Light(Light::SPOTLIGHT);
Light spotlight2 = Light(Light::SPOTLIGHT);

std::vector<Light*> lights = {&main_light, &spotlight1, &spotlight2};

Camera* active_camera = &main_camera;

// mouse input tracking variables
float lastX = WINDOW_WIDTH/2.0f;
float lastY = WINDOW_HEIGHT/2.0f;
bool firstMouse = true;

#define SHADOW_SIZE 1024

GLuint directionalShadowFbo;
GLuint directionalShadowArray;
std::vector<glm::mat4> directionalLightTransforms;

GLuint spotShadowFbo;
GLuint spotShadowArray;
std::vector<glm::mat4> spotLightTransforms;

GLuint shadowMapShader;   // shadow map shader

GLuint offsetTexture; // noise texture for PCF sampling
#define PI 3.14159265358979323846f

bool enableShadows = true;

// grass toggle
bool showGrassLeaves = true;

// environment map stuff
#define CUBEMAP_SIZE 512

GLuint cubemapTexture;
GLuint cubemapFbo;
GLuint cubemapRbo;

bool cubemapNeedsRender = true;

// hardcoded unit cube, centered at origin
float debugCubeVertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,  -1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,  -1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,  -1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,  -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,  -1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,  -1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,   1.0f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f,   1.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,   1.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,   1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f,   1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,   1.0f, 0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, -1.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,   0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,   0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,   0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,   0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
};

GLuint debugCubeVao, debugCubeVbo;
bool showDebugCube = false;

void generateDebugSphere(int stacks, int slices, float radius, std::vector<float>& data)
{
    data.clear();

    for (int i = 0; i < stacks; ++i)
    {
        float phi1 = glm::pi<float>() * i / stacks;
        float phi2 = glm::pi<float>() * (i + 1) / stacks;

        for (int j = 0; j < slices; ++j)
        {
            float theta1 = 2.0f * glm::pi<float>() * j / slices;
            float theta2 = 2.0f * glm::pi<float>() * (j + 1) / slices;

            // 4 points of quad
            glm::vec3 p1 = glm::vec3(sin(phi1)*cos(theta1), cos(phi1), sin(phi1)*sin(theta1)) * radius;
            glm::vec3 p2 = glm::vec3(sin(phi2)*cos(theta1), cos(phi2), sin(phi2)*sin(theta1)) * radius;
            glm::vec3 p3 = glm::vec3(sin(phi2)*cos(theta2), cos(phi2), sin(phi2)*sin(theta2)) * radius;
            glm::vec3 p4 = glm::vec3(sin(phi1)*cos(theta2), cos(phi1), sin(phi1)*sin(theta2)) * radius;

            glm::vec2 uv1 = glm::vec2(theta1 / (2.0f*glm::pi<float>()), phi1 / glm::pi<float>());
            glm::vec2 uv2 = glm::vec2(theta1 / (2.0f*glm::pi<float>()), phi2 / glm::pi<float>());
            glm::vec2 uv3 = glm::vec2(theta2 / (2.0f*glm::pi<float>()), phi2 / glm::pi<float>());
            glm::vec2 uv4 = glm::vec2(theta2 / (2.0f*glm::pi<float>()), phi1 / glm::pi<float>());

            // tangent (approx, good enough for debug)
            glm::vec3 tangent = glm::normalize(glm::vec3(-sin(theta1), 0.0f, cos(theta1)));

            auto pushVertex = [&](glm::vec3 pos, glm::vec2 uv)
            {
                glm::vec3 normal = glm::normalize(pos);

                data.push_back(pos.x);
                data.push_back(pos.y);
                data.push_back(pos.z);

                data.push_back(uv.x);
                data.push_back(uv.y);

                data.push_back(normal.x);
                data.push_back(normal.y);
                data.push_back(normal.z);

                data.push_back(tangent.x);
                data.push_back(tangent.y);
                data.push_back(tangent.z);
            };

            // triangle 1 (FLIPPED)
            pushVertex(p1, uv1);
            pushVertex(p3, uv3);
            pushVertex(p2, uv2);

            // triangle 2 (FLIPPED)
            pushVertex(p1, uv1);
            pushVertex(p4, uv4);
            pushVertex(p3, uv3);
        }
    }
}

// bloom stuff
GLuint bloomThresholdShader;
GLuint bloomBlurShader;
GLuint bloomCompositeShader;

GLuint hdrFbo;
GLuint hdrColorTexture;  // float texture — the scene
GLuint hdrDepthRbo;

GLuint pingFbo, pongFbo;
GLuint pingTexture, pongTexture;

// Fullscreen quad VAO
GLuint quadVao, quadVbo;

float bloomThreshold = 1.0f;  // pixels brighter than this get bloomed
float bloomStrength = 1.0f;  // how much bloom adds on top
float bloomExposure = 0.5f;  // tone mapping exposure
int bloomPasses = 10;    // number of blur iterations — more = wider glow

bool enableBloom = true;

float lastPrintTime = 0.0f; // for debugging camera position printouts
/*------------------FISH--------------------*/

// fish parameters
const int NUM_FISH = 16;
const int DT = 16; // milliseconds per frame (~60 FPS)
const float TURN_RATE = 0.1f; // radians per frame
const int MAX_SPEED = 30;
const int MIN_SPEED = 20;

const float AVOID_RADIUS = 0.4f;
const float AVOID_WEIGHT = 0.5f;
const float OBSTACLE_WEIGHT = 1.0f;
const float FLOW_WEIGHT = 1.0f;
const float AVOID_DISTANCE = 1.5f; // how far influence reaches
const float EPSILON = 0.0001f;

const glm::vec3 WORLD_UP(0.0f, 1.0f, 0.0f);
const glm::vec3 TANK_MIN(-20.0f, 0.0f, -20.0f);
const glm::vec3 TANK_MAX(20.0f);

float tankVertices[] = {
    // positions          // texture coords  // normals         // colors
    TANK_MIN.x, TANK_MIN.y, TANK_MIN.z, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, // back face
    TANK_MAX.x, TANK_MIN.y, TANK_MIN.z, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MAX.y, TANK_MIN.z, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MAX.y, TANK_MIN.z, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MAX.y, TANK_MIN.z, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MIN.y, TANK_MIN.z, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,

    // front face
    TANK_MIN.x, TANK_MIN.y, TANK_MAX.z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MIN.y, TANK_MAX.z, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MAX.y, TANK_MAX.z, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MAX.y, TANK_MAX.z, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MAX.y, TANK_MAX.z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MIN.y, TANK_MAX.z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,

    // left face
    TANK_MIN.x, TANK_MAX.y, TANK_MAX.z, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MAX.y, TANK_MIN.z, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MIN.y, TANK_MIN.z, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MIN.y, TANK_MIN.z, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MIN.y, TANK_MAX.z, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MAX.y, TANK_MAX.z, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    // right face
    TANK_MAX.x, TANK_MAX.y, TANK_MAX.z, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MAX.y, TANK_MIN.z, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MIN.y, TANK_MIN.z, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MIN.y, TANK_MIN.z, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MIN.y, TANK_MAX.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MAX.y, TANK_MAX.z, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    // bottom face
    TANK_MIN.x, TANK_MIN.y, TANK_MIN.z, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MIN.y, TANK_MIN.z, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MIN.y, TANK_MAX.z, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MIN.y, TANK_MAX.z, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MIN.y, TANK_MAX.z, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MIN.y, TANK_MIN.z, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f,
    // top face
    TANK_MIN.x, TANK_MAX.y, TANK_MIN.z, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MAX.y, TANK_MIN.z, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MAX.y, TANK_MAX.z, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    TANK_MAX.x, TANK_MAX.y, TANK_MAX.z, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MAX.y, TANK_MAX.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    TANK_MIN.x, TANK_MAX.y, TANK_MIN.z, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,

};

struct Fish {
    glm::vec3 position;
    glm::vec3 velocity; 
    float speed;
    float radius;
    glm::quat orientation;

    Light* light = nullptr;
};

std::vector<Fish> fishes(NUM_FISH);
std::vector<glm::mat4> fishMatrices(NUM_FISH);

void initFish() {
    float radius = 10.0f;
    float offset = 5.0f;

    int i = 0;
    for (auto& f : fishes) {
        float angle = (float)i++ / (float)NUM_FISH * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f + 1.0f;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        
        f.position = glm::vec3(x, y, z);

        f.velocity = glm::normalize(glm::vec3(
            static_cast<float>(rand() % 2000) / 1000.0f - 1.0f,
            static_cast<float>(rand() % 2000) / 1000.0f - 1.0f,
            static_cast<float>(rand() % 2000) / 1000.0f - 1.0f
        ));
        f.speed = static_cast<float>(((rand() % (MAX_SPEED - MIN_SPEED + 1)) + MIN_SPEED) / 1000.0f);
        // std::cout << f.speed << std::endl;
        f.radius = 0.15f;
        f.orientation = glm::quatLookAt(f.velocity, glm::vec3(0.0f, 1.0f, 0.0f)); 

        Light* fireflyLight = new Light(Light::POINT);
        fireflyLight->externalPosition = &f.position;
        fireflyLight->diffuse  = glm::vec3(0.8f, 0.6f, 0.2f); // warm yellow
        fireflyLight->color    = glm::vec3(1.0f, 0.9f, 0.5f);
        fireflyLight->constant  = 1.0f;
        fireflyLight->linear    = 0.7f;
        fireflyLight->quadratic = 1.8f;
        f.light = fireflyLight;
        lights.push_back(fireflyLight);
    }    
}

glm::vec3 flowField(const glm::vec3& position, float time) {
    float flowX = sin(position.z + time);
    float flowY = cos(position.x + time * 0.5f);
    float flowZ = cos(position.y + time);
    return glm::normalize(glm::vec3(flowX, flowY, flowZ));
}

glm::vec3 avoidNeighbors(const Fish& fish, const std::vector<Fish>& fishes) {
    glm::vec3 avoidance(0.0f);
    for (const auto& other : fishes) {
        if (&fish != &other) {
            glm::vec3 d = fish.position - other.position;
            float dist = glm::length(d);

            if (dist < AVOID_RADIUS) {
                avoidance += glm::normalize(d) * (AVOID_RADIUS - dist);
            }
        }
    }
    return avoidance;
}

glm::vec3 avoidWalls(const Fish& f) {
    glm::vec3 avoidance(0.0f);
    float margin = 0.5f;
    
    glm::vec3 pos = f.position;
    if (pos.x > TANK_MAX.x - margin) avoidance.x -= 1.0f;
    if (pos.x < TANK_MIN.x + margin) avoidance.x += 1.0f;
    if (pos.y > TANK_MAX.y - margin) avoidance.y -= 1.0f;
    if (pos.y < TANK_MIN.y + margin) avoidance.y += 1.0f;
    if (pos.z > TANK_MAX.z - margin) avoidance.z -= 1.0f;
    if (pos.z < TANK_MIN.z - margin) avoidance.z += 1.0f;

    return avoidance;
}

struct Obstacle {
    glm::vec3 min;
    glm::vec3 max;
};

std::vector<Obstacle> obstacles = {
    {glm::vec3(-7.9399, 6.10147, 2.57709), glm::vec3(8.09769, 1.73833, -2.57709)},
    {glm::vec3(6.73175, -0.01116, -1.64897), glm::vec3(-6.7474, 2.0365, 1.67077)},
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

std::vector<AABB> aabbs;

void makeAABBs() {
    for (const Obstacle &obs : obstacles) {
        AABB box;
        box.min = glm::min(obs.min, obs.max);
        box.max = glm::max(obs.min, obs.max);
        aabbs.push_back(box);
    }
}


glm::vec3 closestPointOnAABB(const glm::vec3& point, const glm::vec3& min, const glm::vec3& max) {
    return glm::clamp(point, min, max);
}

glm::vec3 avoidBoundingBox(const Fish& fish, const glm::vec3& boxMin, const glm::vec3& boxMax) {
    glm::vec3 closest = closestPointOnAABB(fish.position, boxMin, boxMax);
    glm::vec3 toFish = fish.position - closest;
    float distance = glm::length(toFish);

    if (distance > AVOID_DISTANCE)
        return glm::vec3(0.0f);

    if (distance < EPSILON) {
        glm::vec3 center = (boxMin + boxMax) * 0.5f;
        glm::vec3 dir = glm::normalize(fish.position - center);
        return dir * AVOID_DISTANCE;
    }

    float strength = (AVOID_DISTANCE - distance) / AVOID_DISTANCE;
    return glm::normalize(toFish) * strength;
}

void computeNextFishStates(float time) {
    for (auto& f : fishes) {
        glm::vec3 flow = flowField(f.position, time) * FLOW_WEIGHT;
        glm::vec3 avoid = avoidNeighbors(f, fishes) * AVOID_WEIGHT;
        glm::vec3 wall = avoidWalls(f) * AVOID_WEIGHT;

        // get obstacles
        glm::vec3 obstacle(0.0f);
        for (const AABB& box : aabbs) {
            obstacle += avoidBoundingBox(f, box.min, box.max) * OBSTACLE_WEIGHT;
        }

        glm::vec3 desiredVelocity = glm::normalize(flow + avoid + wall + obstacle);
        f.velocity = glm::mix(f.velocity, desiredVelocity, TURN_RATE); // smooth turning 
        f.position += f.velocity * f.speed * (DT / 16.0f); // adjust speed based on frame time 
        f.orientation = glm::quatLookAt(f.velocity, glm::vec3(0.0f, 1.0f, 0.0f)); // orient

    }
}


/*------------------------------------------*/

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
    for (const auto &light : lights) {
        switch (light->type) {
            case Light::DIRECTIONAL:
                light->cam.front = glm::vec3(-0.2f, -1.0f, -0.3f);
                light->cam.position = glm::vec3(0.0f, 40.0f, 5.0f);
                break;
            case Light::SPOTLIGHT:
                light->inner_cutoff = 12.0f;
                light->outer_cutoff = 17.0f;
                break;
            case Light::POINT:
                // todo lol
                break;
        }
    }
}



bool setupShadowMaps()
{
    int numDir = 0, numSpot = 0;
    for (auto* light : lights) {
        if (light->type == Light::DIRECTIONAL) numDir++;
        else if (light->type == Light::SPOTLIGHT) numSpot++;
    }

    directionalLightTransforms.resize(numDir);
    spotLightTransforms.resize(numSpot);

    // --- Directional shadow array ---
    // One FBO, one texture array with numDir layers
    glGenFramebuffers(1, &directionalShadowFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, directionalShadowFbo);

    glGenTextures(1, &directionalShadowArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, directionalShadowArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT,
                 SHADOW_SIZE, SHADOW_SIZE, numDir,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Attach layer 0 initially just to validate the FBO
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, directionalShadowArray, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Could not create directional shadow framebuffer.\n";
        return false;
    }

    // --- Spot shadow array ---
    glGenFramebuffers(1, &spotShadowFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, spotShadowFbo);

    glGenTextures(1, &spotShadowArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, spotShadowArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT,
                 SHADOW_SIZE, SHADOW_SIZE, numSpot,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, spotShadowArray, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Could not create spot shadow framebuffer.\n";
        return false;
    }

    // Load shadow shader
    shadowMapShader = gdevLoadShader("Finals-Shader-Shadow.vs", "Finals-Shader-Shadow.fs");
    if (!shadowMapShader)
        return false;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}


void drawSceneGeometry() {
    // Floor Mesh
    glBindVertexArray(vaos[0]);
    glDrawArrays(GL_TRIANGLES, 0, FloorMesh.size() / 11);
    
    // Bricks Parallax
    glBindVertexArray(vaos[1]);
    glDrawArrays(GL_TRIANGLES, 0, BricksParallax.size() / 11);

    // Lower Building
    glBindVertexArray(vaos[3]);
    glDrawArrays(GL_TRIANGLES, 0, LowerBuilding.size() / 11);

    // Lower Window
    // glBindVertexArray(vaos[4]);
    // glDrawArrays(GL_TRIANGLES, 0, LowerWindow.size() / 11);

    // Higher Building
    glBindVertexArray(vaos[5]);
    glDrawArrays(GL_TRIANGLES, 0, HigherBuilding.size() / 11);

    // Higher Window
    // glBindVertexArray(vaos[6]);
    // glDrawArrays(GL_TRIANGLES, 0, HigherWindow.size() / 11);

    glBindVertexArray(vaos[8]);
    glDrawArrays(GL_TRIANGLES, 0, TreeBark.size() / 11);

    // fish
    // glBindVertexArray(instancedVao);
    // glDrawArraysInstanced(GL_TRIANGLES, 0, InstanceMesh.size() / 11, NUM_FISH);

    glBindVertexArray(vaos[10]);
    glDrawArrays(GL_TRIANGLES, 0, MirrorPlane.size() / 11);

    glBindVertexArray(vaos[11]);
    glDrawArrays(GL_TRIANGLES, 0, SideStation.size() / 11);

    glBindVertexArray(vaos[12]);
    glDrawArrays(GL_TRIANGLES, 0, Office.size() / 11);

    glBindVertexArray(vaos[13]);
    glDrawArrays(GL_TRIANGLES, 0, BusStation.size() / 11);

    glBindVertexArray(vaos[14]);
    glDrawArrays(GL_TRIANGLES, 0, Miscellaneous.size() / 11);

    glBindVertexArray(vaos[15]);
    glDrawArrays(GL_TRIANGLES, 0, Water.size() / 11);

    glBindVertexArray(vaos[16]);
    glDrawArrays(GL_TRIANGLES, 0, TrainStation.size() / 11);

    glBindVertexArray(vaos[17]);
    glDrawArrays(GL_TRIANGLES, 0, TrainCart.size() / 11);

    glBindVertexArray(vaos[18]);
    glDrawArrays(GL_TRIANGLES, 0, LampPost.size() / 11);

    glBindVertexArray(vaos[19]);
    glDrawArrays(GL_TRIANGLES, 0, LampBulb.size() / 11);
}

void renderDirectionalShadows(int index, Light& light) {
    // use the shadow framebuffer for drawing the shadow map
    glBindFramebuffer(GL_FRAMEBUFFER, directionalShadowFbo);

    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              directionalShadowArray, 0, index);

    // the viewport should be the size of the shadow map
    glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);

    // clear the shadow map
    // (we don't have a color buffer attachment, so no need to clear that)
    glClear(GL_DEPTH_BUFFER_BIT);

    // using the shadow map shader...
    glUseProgram(shadowMapShader);

    // ... set up the light space matrix... FOR DIRECTIONAL LIGHTS
    float bounds = 45.0f;
    glm::mat4 lightTransform;
    lightTransform = glm::ortho(-bounds, bounds, -bounds, bounds, 0.1f, 100.0f) * 
                    glm::lookAt(light.getPosition(),           // light position
                                glm::vec3(0.0f, 0.0f, 0.0f),   // scene center
                                glm::vec3(0.0f, 1.0f, 0.0f));  // up vector

    glUniformMatrix4fv(glGetUniformLocation(shadowMapShader, "lightTransform"),
                       1, GL_FALSE, glm::value_ptr(lightTransform));

    // ... set up the model matrix... (just identity for this demo)
    glm::mat4 modelTransform = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shadowMapShader, "modelTransform"),
                       1, GL_FALSE, glm::value_ptr(modelTransform));

    drawSceneGeometry();

    // set the framebuffer back to the default onscreen buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    directionalLightTransforms[index] = lightTransform;

}

void renderSpotShadows(int index, Light& light) {
    // use the shadow framebuffer for drawing the shadow map
    glBindFramebuffer(GL_FRAMEBUFFER, spotShadowFbo);

    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              spotShadowArray, 0, index);

    // the viewport should be the size of the shadow map
    glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);

    // clear the shadow map
    // (we don't have a color buffer attachment, so no need to clear that)
    glClear(GL_DEPTH_BUFFER_BIT);

    // using the shadow map shader...
    glUseProgram(shadowMapShader);

    // ... set up the light space matrix... FOR SPOTLIGHTS
    glm::mat4 lightTransform;
    lightTransform = glm::perspective(glm::radians(light.outer_cutoff * 2.0f),       // fov
                                1.0f,                      // aspect ratio
                                0.1f,                      // near plane
                                100.0f);                   // far plane
    lightTransform *= glm::lookAt(light.getPosition(),                 // eye position
                                light.getPosition() + light.getDirection(),   // center position
                                glm::vec3(0.0f, 1.0f, 0.0f));  // up vector

    glUniformMatrix4fv(glGetUniformLocation(shadowMapShader, "lightTransform"),
                       1, GL_FALSE, glm::value_ptr(lightTransform));

    // ... set up the model matrix... (just identity for this demo)
    glm::mat4 modelTransform = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shadowMapShader, "modelTransform"),
                       1, GL_FALSE, glm::value_ptr(modelTransform));

    drawSceneGeometry();

    // set the framebuffer back to the default onscreen buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    spotLightTransforms[index] = lightTransform;
    


}

float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

std::vector<float> data;

// for pcf with random sampling
void generateOffsetTextureData(int windowSize, int filterSize, std::vector<float>& data) {
    
    int bufferSize = windowSize * windowSize * filterSize * filterSize * 2; 
    int numFilterSamples = filterSize * filterSize;
    data.resize(bufferSize);

    int index = 0;

    for (int texY = 0; texY < windowSize; ++texY) {
        for (int texX = 0; texX < windowSize; ++texX) {
            for (int i = 0; i < numFilterSamples / 2; ++i) {
                // generate sample 2*i
                int sampleIndex = 2 * i;
                int u = sampleIndex % filterSize;
                int v = sampleIndex / filterSize;
                float x = ((float)u + 0.5f + randomFloat(-0.5f, 0.5f)) / (float)filterSize;
                float y = ((float)v + 0.5f + randomFloat(-0.5f, 0.5f)) / (float)filterSize;
                float x1 = sqrtf(y) * cosf(2.0f * PI * x);
                float y1 = sqrtf(y) * sinf(2.0f * PI * x);

                // generate sample 2*i+1
                sampleIndex = 2 * i + 1;
                u = sampleIndex % filterSize;
                v = sampleIndex / filterSize;
                x = ((float)u + 0.5f + randomFloat(-0.5f, 0.5f)) / (float)filterSize;
                y = ((float)v + 0.5f + randomFloat(-0.5f, 0.5f)) / (float)filterSize;
                float x2 = sqrtf(y) * cosf(2.0f * PI * x);
                float y2 = sqrtf(y) * sinf(2.0f * PI * x);

                data[index++] = x1;
                data[index++] = y1;
                data[index++] = x2;
                data[index++] = y2;
            }
        }
    }
}

void createTexture(int windowSize, int filterSize, const std::vector<float>& data) {
    int numFilterSamples = filterSize * filterSize;

    glActiveTexture(GL_TEXTURE0 + 12); // using texture unit 12 for the offset texture
    glGenTextures(1, &offsetTexture);
    glBindTexture(GL_TEXTURE_3D, offsetTexture);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, numFilterSamples / 2, windowSize, windowSize);
    // glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, numFilterSamples / 2, windowSize, windowSize, GL_RGBA, GL_FLOAT, data.data());
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, numFilterSamples / 2, windowSize, windowSize, GL_RGBA, GL_FLOAT, &data[0]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_3D, 0);

}

void setupPCF() {
    int windowSize = 12;
    int filterSize = 7;

    std::vector<float> offsetData;
    generateOffsetTextureData(windowSize, filterSize, offsetData);
    createTexture(windowSize, filterSize, offsetData);

}

bool setupCubemap() {
    // Create the cubemap texture
    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    for (int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                     CUBEMAP_SIZE, CUBEMAP_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Create FBO
    glGenFramebuffers(1, &cubemapFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, cubemapFbo);

    // Depth renderbuffer — cubemap pass needs depth testing
    // (unlike shadow maps we need a color attachment here, so RBO handles depth)
    glGenRenderbuffers(1, &cubemapRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, cubemapRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, CUBEMAP_SIZE, CUBEMAP_SIZE);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, cubemapRbo);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Cubemap framebuffer incomplete.\n";
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void uploadLightUniforms(const glm::mat4& viewMatrix) {
    int spotlightCount = 0;
    int pointLightCount = 0;
    for (const auto& light : lights) {
        switch (light->type) {
            case Light::DIRECTIONAL: {
                glm::vec3 viewDir = glm::mat3(viewMatrix) * light->getDirection();
                glUniform3fv(glGetUniformLocation(shader, "dir_lights[0].direction"),
                             1, glm::value_ptr(viewDir));
                glUniform3fv(glGetUniformLocation(shader, "dir_lights[0].ambient"),
                             1, glm::value_ptr(light->ambient));
                glUniform3fv(glGetUniformLocation(shader, "dir_lights[0].diffuse"),
                             1, glm::value_ptr(light->diffuse));
                glUniform3fv(glGetUniformLocation(shader, "dir_lights[0].specular"),
                             1, glm::value_ptr(light->specular));
                glUniform3fv(glGetUniformLocation(shader, "dir_lights[0].color"),
                             1, glm::value_ptr(light->color));
                glUniform1f(glGetUniformLocation(shader, "dir_lights[0].specular_exponent"),
                            light->specular_exponent);
                break;
            }
            case Light::SPOTLIGHT: {
                std::string base = "spotlights[" + std::to_string(spotlightCount) + "].";
                spotlightCount++;

                glm::vec3 posView = glm::vec3(viewMatrix * glm::vec4(light->getPosition(), 1.0f));
                glUniform3fv(glGetUniformLocation(shader, (base + "position").c_str()),
                             1, glm::value_ptr(posView));

                glm::vec3 dirView = glm::mat3(viewMatrix) * light->getDirection();
                glUniform3fv(glGetUniformLocation(shader, (base + "direction").c_str()),
                             1, glm::value_ptr(dirView));

                glUniform1f(glGetUniformLocation(shader, (base + "innerCutoff").c_str()),
                            glm::cos(glm::radians(light->inner_cutoff)));
                glUniform1f(glGetUniformLocation(shader, (base + "outerCutoff").c_str()),
                            glm::cos(glm::radians(light->outer_cutoff)));
                glUniform1f(glGetUniformLocation(shader, (base + "constant").c_str()),  light->constant);
                glUniform1f(glGetUniformLocation(shader, (base + "linear").c_str()),    light->linear);
                glUniform1f(glGetUniformLocation(shader, (base + "quadratic").c_str()), light->quadratic);
                glUniform3fv(glGetUniformLocation(shader, (base + "ambient").c_str()),
                             1, glm::value_ptr(light->ambient));
                glUniform3fv(glGetUniformLocation(shader, (base + "diffuse").c_str()),
                             1, glm::value_ptr(light->diffuse));
                glUniform3fv(glGetUniformLocation(shader, (base + "specular").c_str()),
                             1, glm::value_ptr(light->specular));
                glUniform3fv(glGetUniformLocation(shader, (base + "color").c_str()),
                             1, glm::value_ptr(light->color));
                glUniform1f(glGetUniformLocation(shader, (base + "specular_exponent").c_str()),
                            light->specular_exponent);
                break;
            }
            case Light::POINT: {
                std::string base = "pointLights[" + std::to_string(pointLightCount) + "].";

                glm::vec3 posView = glm::vec3(viewMatrix * glm::vec4(light->getPosition(), 1.0f));
                glUniform3fv(glGetUniformLocation(shader, (base + "position").c_str()),
                            1, glm::value_ptr(posView));
                glUniform3fv(glGetUniformLocation(shader, (base + "ambient").c_str()),
                            1, glm::value_ptr(light->ambient));
                glUniform3fv(glGetUniformLocation(shader, (base + "diffuse").c_str()),
                            1, glm::value_ptr(light->diffuse));
                glUniform3fv(glGetUniformLocation(shader, (base + "specular").c_str()),
                            1, glm::value_ptr(light->specular));
                glUniform3fv(glGetUniformLocation(shader, (base + "color").c_str()),
                            1, glm::value_ptr(light->color));
                glUniform1f(glGetUniformLocation(shader, (base + "specular_exponent").c_str()),
                            light->specular_exponent);
                glUniform1f(glGetUniformLocation(shader, (base + "constant").c_str()),
                            light->constant);
                glUniform1f(glGetUniformLocation(shader, (base + "linear").c_str()),
                            light->linear);
                glUniform1f(glGetUniformLocation(shader, (base + "quadratic").c_str()),
                            light->quadratic);

                pointLightCount++;
                break;
            }
            default: break;
        }
    }
    glUniform1i(glGetUniformLocation(shader, "numPointLights"), pointLightCount); // for point lights
}


void renderCubemap() {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // needed so floor renders from below
    // glm::vec3 capturePos = glm::vec3(0.0f, 2.0f, 0.0f); // TODO: add multiple capture positions

    glm::vec3 capturePos = glm::vec3(-12.4638, 2.2572, -5.79182); // TODO: add multiple capture positions

    struct FaceSetup { glm::vec3 direction, up; };
    FaceSetup faces[6] = {
        { glm::vec3( 1,  0,  0), glm::vec3(0, -1,  0) },
        { glm::vec3(-1,  0,  0), glm::vec3(0, -1,  0) },
        { glm::vec3( 0,  1,  0), glm::vec3(0,  0,  1) },
        { glm::vec3( 0, -1,  0), glm::vec3(0,  0, -1) },
        { glm::vec3( 0,  0,  1), glm::vec3(0, -1,  0) },
        { glm::vec3( 0,  0, -1), glm::vec3(0, -1,  0) },
    };

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 500.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, cubemapFbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glViewport(0, 0, CUBEMAP_SIZE, CUBEMAP_SIZE);
    glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
    glUseProgram(shader);

    glUniformMatrix4fv(glGetUniformLocation(shader, "projectionTransform"),
                       1, GL_FALSE, glm::value_ptr(captureProjection));

    glUniform1i(glGetUniformLocation(shader, "isInstanced"),    0);
    glUniform1i(glGetUniformLocation(shader, "isAlphaBlended"), 0);
    glUniform1i(glGetUniformLocation(shader, "isReflective"),   0);
    glUniform1i(glGetUniformLocation(shader, "hasNormal"),      0);
    glUniform1i(glGetUniformLocation(shader, "hasSpecular"),    0);
    glUniform1i(glGetUniformLocation(shader, "isTile"),         0);
    glUniform1i(glGetUniformLocation(shader, "enableShadows"),  enableShadows); // use real shadows
    glUniform1f(glGetUniformLocation(shader, "alphaThreshold"), 0.1f);
    glUniform1f(glGetUniformLocation(shader, "time"),           0.0f);
    glUniform3fv(glGetUniformLocation(shader, "cameraWorldPos"),
                 1, glm::value_ptr(capturePos));

    // Upload shadow maps — same as main pass, they're already computed
    if (enableShadows) {
        for (int i = 0; i < (int)directionalLightTransforms.size(); i++) {
            std::string name = "directionalLightTransforms[" + std::to_string(i) + "]";
            glUniformMatrix4fv(glGetUniformLocation(shader, name.c_str()),
                               1, GL_FALSE, glm::value_ptr(directionalLightTransforms[i]));
        }
        for (int i = 0; i < (int)spotLightTransforms.size(); i++) {
            std::string name = "spotLightTransforms[" + std::to_string(i) + "]";
            glUniformMatrix4fv(glGetUniformLocation(shader, name.c_str()),
                               1, GL_FALSE, glm::value_ptr(spotLightTransforms[i]));
        }
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D_ARRAY, directionalShadowArray);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D_ARRAY, spotShadowArray);
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_3D, offsetTexture);
        glUniform1i(glGetUniformLocation(shader, "offsetTexture"), 12);
    }

    for (int i = 0; i < 6; i++) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               cubemapTexture, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 faceView = glm::lookAt(capturePos,
                                         capturePos + faces[i].direction,
                                         faces[i].up);

        glUniformMatrix4fv(glGetUniformLocation(shader, "viewTransform"),
                           1, GL_FALSE, glm::value_ptr(faceView));

        glm::mat4 identityModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader, "modelTransform"),
                           1, GL_FALSE, glm::value_ptr(identityModel));

        // Upload real lights transformed into this face's camera space
        uploadLightUniforms(faceView);

        // Floor
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[0]);
        glBindVertexArray(vaos[0]);
        glDrawArrays(GL_TRIANGLES, 0, FloorMesh.size() / 11);

        // Bricks
        glBindTexture(GL_TEXTURE_2D, texture[2]);
        glBindVertexArray(vaos[1]);
        glDrawArrays(GL_TRIANGLES, 0, BricksParallax.size() / 11);

        // Lower Building
        glBindTexture(GL_TEXTURE_2D, texture[5]);
        glBindVertexArray(vaos[3]);
        glDrawArrays(GL_TRIANGLES, 0, LowerBuilding.size() / 11);

        // Tree Bark
        glBindTexture(GL_TEXTURE_2D, texture[9]);
        glBindVertexArray(vaos[8]);
        glDrawArrays(GL_TRIANGLES, 0, TreeBark.size() / 11);

        // Mirror
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[6]); // Temp/black Pic
        glBindVertexArray(vaos[10]);
        glDrawArrays(GL_TRIANGLES, 0, MirrorPlane.size() / 11);

        // Side Station
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[11]);
        glBindVertexArray(vaos[11]);
        glDrawArrays(GL_TRIANGLES, 0, SideStation.size() / 11);

        // Office
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[12]);
        glBindVertexArray(vaos[12]);
        glDrawArrays(GL_TRIANGLES, 0, Office.size() / 11);

        // Bus Station
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[14]);
        glBindVertexArray(vaos[13]);
        glDrawArrays(GL_TRIANGLES, 0, BusStation.size() / 11);

        // Miscellaneous
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[16]);
        glBindVertexArray(vaos[14]);
        glDrawArrays(GL_TRIANGLES, 0, Miscellaneous.size() / 11);

        // Water
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[18]);
        glBindVertexArray(vaos[15]);
        glDrawArrays(GL_TRIANGLES, 0, Water.size() / 11);

        // Train Station
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[19]);
        glBindVertexArray(vaos[16]);
        glDrawArrays(GL_TRIANGLES, 0, TrainStation.size() / 11);

        // Train Cart
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[22]);
        glBindVertexArray(vaos[17]);
        glDrawArrays(GL_TRIANGLES, 0, TrainCart.size() / 11);

        // Lamp Post
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[25]);
        glBindVertexArray(vaos[18]);
        glDrawArrays(GL_TRIANGLES, 0, LampPost.size() / 11);

        // Lamp Bulb
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[26]);
        glBindVertexArray(vaos[19]);
        glDrawArrays(GL_TRIANGLES, 0, LampBulb.size() / 11);

        // Higher Building
        glBindTexture(GL_TEXTURE_2D, texture[5]);
        glBindVertexArray(vaos[5]);
        glDrawArrays(GL_TRIANGLES, 0, HigherBuilding.size() / 11);

        // lower windows
        glBindTexture(GL_TEXTURE_2D, texture[6]); // window diffuse lol
        glBindVertexArray(vaos[4]);
        glDrawArrays(GL_TRIANGLES, 0, LowerWindow.size() / 11);

        // higher windows
        glBindVertexArray(vaos[6]);
        glDrawArrays(GL_TRIANGLES, 0, HigherWindow.size() / 11);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_CULL_FACE); // restore
}

bool setupBloom() {
    int w = WINDOW_WIDTH, h = WINDOW_HEIGHT;

    // --- HDR framebuffer ---
    glGenFramebuffers(1, &hdrFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFbo);

    // Float texture for HDR color
    glGenTextures(1, &hdrColorTexture);
    glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, hdrColorTexture, 0);

    // Depth renderbuffer
    glGenRenderbuffers(1, &hdrDepthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, hdrDepthRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "HDR framebuffer incomplete.\n";
        return false;
    }

    // --- Ping-pong framebuffers for blur ---
    GLuint pingpongFbos[2] = {pingFbo, pongFbo};
    GLuint pingpongTextures[2] = {pingTexture, pongTexture};
    glGenFramebuffers(2, pingpongFbos);
    glGenTextures(2, pingpongTextures);
    pingFbo = pingpongFbos[0]; pongFbo = pingpongFbos[1];
    pingTexture = pingpongTextures[0]; pongTexture = pingpongTextures[1];

    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFbos[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, pingpongTextures[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Ping-pong framebuffer " << i << " incomplete.\n";
            return false;
        }
    }

    // --- Fullscreen quad ---
    float quadVertices[] = {
        // x      y      u     v
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
    };

    glGenVertexArrays(1, &quadVao);
    glGenBuffers(1, &quadVbo);
    glBindVertexArray(quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // --- Load bloom shaders ---
    bloomThresholdShader = gdevLoadShader("Finals-Bloom-Shader.vs", "Finals-Bloom-Threshold.fs");
    bloomBlurShader = gdevLoadShader("Finals-Bloom-Shader.vs", "Finals-Bloom-Blur.fs");
    bloomCompositeShader = gdevLoadShader("Finals-Bloom-Shader.vs", "Finals-Bloom-Composite.fs");

    if (!bloomThresholdShader || !bloomBlurShader || !bloomCompositeShader)
        return false;

    // Set sampler uniforms once
    glUseProgram(bloomThresholdShader);
    glUniform1i(glGetUniformLocation(bloomThresholdShader, "hdrScene"), 0);

    glUseProgram(bloomBlurShader);
    glUniform1i(glGetUniformLocation(bloomBlurShader, "image"), 0);

    glUseProgram(bloomCompositeShader);
    glUniform1i(glGetUniformLocation(bloomCompositeShader, "hdrScene"),  0);
    glUniform1i(glGetUniformLocation(bloomCompositeShader, "bloomBlur"), 1);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

// called by the main function to do initial setup, such as uploading vertex
// arrays, shader programs, etc.; returns true if successful, false otherwise
bool setup()
{
    readModelData(FloorMesh, "Finals-Data-FloorMesh.txt");
    readModelData(BricksParallax, "Finals-Data-Parallax.txt");
    readModelData(LowerBuilding, "Finals-Data-LowerBuilding.txt");
    readModelData(LowerWindow, "Finals-Data-LowerWindow.txt");
    readModelData(HigherBuilding, "Finals-Data-HigherBuilding.txt");
    readModelData(HigherWindow, "Finals-Data-HigherWindow.txt");
    readModelData(GrassMesh, "Finals-Data-Grass.txt");
    readModelData(TreeBark, "Finals-Data-Tree.txt");
    readModelData(TreeLeaves, "Finals-Data-Leaves.txt");
    readModelData(MirrorPlane, "Finals-Data-MirrorPlane.txt");
    readModelData(SideStation, "Finals-Data-SideStation.txt");
    readModelData(Office, "Finals-Data-Office.txt");
    readModelData(BusStation, "Finals-Data-BusStation.txt");
    readModelData(Miscellaneous, "Finals-Data-Misc.txt");
    readModelData(Water, "Finals-Data-Water.txt");
    readModelData(TrainStation, "Finals-Data-Station.txt");
    readModelData(TrainCart, "Finals-Data-TrainCart.txt");
    readModelData(LampPost, "Finals-Data-LampPost.txt");
    readModelData(LampBulb, "Finals-Data-LampBulb.txt");
    // readModelData(InstanceMesh, "fish_data.txt");
    generateDebugSphere(4, 4, 0.05f, InstanceMesh);

    vertex_data[0] = FloorMesh;
    vertex_data[1] = BricksParallax;
    vertex_data[2] = GrassMesh;
    vertex_data[3] = LowerBuilding;
    vertex_data[4] = LowerWindow;
    vertex_data[5] = HigherBuilding;
    vertex_data[6] = HigherWindow;
    vertex_data[7] = InstanceMesh;
    vertex_data[8] = TreeBark;
    vertex_data[9] = TreeLeaves;
    vertex_data[10] = MirrorPlane;
    vertex_data[11] = SideStation;
    vertex_data[12] = Office;
    vertex_data[13] = BusStation;
    vertex_data[14] = Miscellaneous;
    vertex_data[15] = Water;
    vertex_data[16] = TrainStation;
    vertex_data[17] = TrainCart;
    vertex_data[18] = LampPost;
    vertex_data[19] = LampBulb;
    // vertex_data[4] = std::vector<float>(std::begin(tankVertices), std::end(tankVertices));

    initFish(); // since fireflies have lights lol

    setupLights();

    // upload the model to the GPU (explanations omitted for brevity)
    glGenVertexArrays(vertex_data_num, vaos);
    glGenBuffers(vertex_data_num, vbos);

    for (int i = 0; i < vertex_data_num; ++i) {
        glBindVertexArray(vaos[i]);
        glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
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
    shader = gdevLoadShader("Finals-Shader.vs", "Finals-Shader.fs");
    if (!shader) return false;

    // since we now use multiple textures, we need to set the texture channel for each texture
    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "diffuseMap"), 0);
    glUniform1i(glGetUniformLocation(shader, "normalMap"),  1);
    glUniform1i(glGetUniformLocation(shader, "specularMap"),  2);
    glUniform1i(glGetUniformLocation(shader, "directionalShadowArray"), 3);
    glUniform1i(glGetUniformLocation(shader, "spotShadowArray"), 4);    
    glUniform1i(glGetUniformLocation(shader, "offsetTexture"), 12);

    glUniform1f(glGetUniformLocation(shader, "shadowMapSize"), SHADOW_SIZE);
    glUniform1f(glGetUniformLocation(shader, "radius"), 8.0f);
    // glUniform2f(glGetUniformLocation(shader, "shadowTexelStep"), 1.0f / SHADOW_SIZE, 1.0f / SHADOW_SIZE);

    // texture unit layout
    // 0 - diffuse map
    // 1 - normal map
    // 2 - specular map
    // 3 - dir shadow maps
    // 4 - spot shadow maps
    // 5 - point light shadow maps (todo)
    // 6 - transparent texture (for grass)
    // 12 - offset texture for pcf

    // for alpha blending
    glUniform1i(glGetUniformLocation(shader, "isAlphaBlended"), 0);
    glUniform1f(glGetUniformLocation(shader, "alphaThreshold"), 0.1f);

    // load our textures
    // Floor Mesh:
    texture[0] = gdevLoadTexture("Tex-FloorMesh-Diffuse.png", GL_REPEAT, true, true);
    texture[1] = gdevLoadTexture("Tex-FloorMesh-Normals.png", GL_REPEAT, true, true);

    // Brick Elevation:
    texture[2] = gdevLoadTexture("Tex-Parallax-Diffuse.png", GL_REPEAT, true, true);
    texture[3] = gdevLoadTexture("Tex-Parallax-Normals.png", GL_REPEAT, true, true);

    // Transparent Grass:
    texture[4] = gdevLoadTexture("Tex-Grass-Diffuse.png", GL_CLAMP_TO_EDGE, true, true);

    // Lower Building:
    texture[5] = gdevLoadTexture("Tex-LowerBuilding-Diffuse.png", GL_REPEAT, true, true);
    texture[6] = gdevLoadTexture("Tex-Windows.jpg", GL_REPEAT, true, true); // window diffuse

    // Higher Building:
    texture[7] = gdevLoadTexture("Tex-HigherBuilding-Diffuse.png", GL_REPEAT, true, true);

    // Instanced Model:
    texture[8] = gdevLoadTexture("Tex-Firefly-Diffuse.png", GL_REPEAT, true, true); // temporary fish

    // Tree Bark:
    texture[9] = gdevLoadTexture("Tex-TreeBark-Diffuse.png", GL_REPEAT, true, true);

    // Tree Leaves:
    texture[10] = gdevLoadTexture("Tex-TreeLeaves-Diffuse.png", GL_REPEAT, true, true);

    // Side Station:
    texture[11] = gdevLoadTexture("Tex-SideStation-Diffuse.png", GL_REPEAT, true, true);

    // Office:
    texture[12] = gdevLoadTexture("Tex-Office-Diffuse.png", GL_REPEAT, true, true);
    texture[13] = gdevLoadTexture("Tex-Office-Normals.png", GL_REPEAT, true, true);

    // Bus Station:
    texture[14] = gdevLoadTexture("Tex-BusSta-Diffuse.png", GL_REPEAT, true, true);
    texture[15] = gdevLoadTexture("Tex-BusSta-Normals.png", GL_REPEAT, true, true);

    // Miscelleanous:
    texture[16] = gdevLoadTexture("Tex-Misc-Diffuse.png", GL_REPEAT, true, true);
    texture[17] = gdevLoadTexture("Tex-Misc-Normals.png", GL_REPEAT, true, true);

    // Water:
    texture[18] = gdevLoadTexture("Tex-Water-Diffuse.png", GL_REPEAT, true, true);

    // Station:
    texture[19] = gdevLoadTexture("Tex-Station-Diffuse.png", GL_REPEAT, true, true);
    texture[20] = gdevLoadTexture("Tex-Station-Normals.png", GL_REPEAT, true, true);
    texture[21] = gdevLoadTexture("Tex-Station-Specular.png", GL_REPEAT, true, true);

    // Station:
    texture[22] = gdevLoadTexture("Tex-Train-Diffuse.png", GL_REPEAT, true, true);
    texture[23] = gdevLoadTexture("Tex-Train-Normals.png", GL_REPEAT, true, true);
    texture[24] = gdevLoadTexture("Tex-Train-Specular.png", GL_REPEAT, true, true);

    // LampPost
    texture[25] = gdevLoadTexture("Tex-LampPost-Diffuse.png", GL_REPEAT, true, true);
    texture[26] = gdevLoadTexture("Tex-LampBulb-Diffuse.png", GL_REPEAT, true, true);

    if (! texture[0] || ! texture[1] || ! texture[2]
        || ! texture[3] || ! texture[4] || ! texture[5]
        || ! texture[6] || ! texture[7] || ! texture[8]
        || ! texture[9] || ! texture[10]|| ! texture[11]
        || ! texture[12]|| ! texture[13]|| ! texture[14]
        || ! texture[15]|| ! texture[16]|| ! texture[17]
        || ! texture[18]|| ! texture[19]|| ! texture[20]
        || ! texture[21]|| ! texture[22]|| ! texture[23]
        || ! texture[24]|| ! texture[25]|| ! texture[26])
        return false;

    /*---------------- INSTANCING FISH -----------------*/
    makeAABBs(); 

    glGenVertexArrays(1, &instancedVao);
    glGenBuffers(1, &instancedVbo);      
    glGenBuffers(1, &instancedVboMatrix); 

    glBindVertexArray(instancedVao);

    glBindBuffer(GL_ARRAY_BUFFER, instancedVbo);
    glBufferData(GL_ARRAY_BUFFER, InstanceMesh.size() * sizeof(float), InstanceMesh.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (3 * sizeof(float)));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (5 * sizeof(float)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (8 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, instancedVboMatrix);
    glBindBuffer(GL_ARRAY_BUFFER, instancedVboMatrix);
    glBufferData(GL_ARRAY_BUFFER, fishMatrices.size() * sizeof(glm::mat4), fishMatrices.data(), GL_STATIC_DRAW);
    
    GLuint instancedVaoMatrix = instancedVao;
    glBindVertexArray(instancedVaoMatrix);

    std::size_t vec4Size = sizeof(glm::vec4);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);

    glBindVertexArray(0);
    /*--------------------------------------------------*/

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if ( !setupShadowMaps())
        return false;
    
    // for pcf with random sampling
    setupPCF();

    if ( !setupCubemap()) 
        return false;
    
    if (!setupBloom()) return false;


    // Bind cubemap to unit 7
    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "cubemap"), 7);
    glUniform1i(glGetUniformLocation(shader, "isReflective"), 0);
    glUniform1f(glGetUniformLocation(shader, "reflectivity"), 0.5f); 

    // renderCubemap();

    // Keep cubemap bound to unit 7 persistently
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    glGenVertexArrays(1, &debugCubeVao);
    glGenBuffers(1, &debugCubeVbo);
    glBindVertexArray(debugCubeVao);
    glBindBuffer(GL_ARRAY_BUFFER, debugCubeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(debugCubeVertices), debugCubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (3 * sizeof(float)));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (5 * sizeof(float)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (8 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    return true;
}

glm::mat4 buildReflectionMatrix(glm::vec3 n, float d)
{
    // n must be normalized
    glm::mat4 M = glm::mat4(1.0f);

    // I - 2(n ⊗ n) for the 3x3 rotation part
    M[0][0] = 1 - 2*n.x*n.x;
    M[0][1] =   - 2*n.y*n.x;
    M[0][2] =   - 2*n.z*n.x;

    M[1][0] =   - 2*n.x*n.y;
    M[1][1] = 1 - 2*n.y*n.y;
    M[1][2] =   - 2*n.z*n.y;

    M[2][0] =   - 2*n.x*n.z;
    M[2][1] =   - 2*n.y*n.z;
    M[2][2] = 1 - 2*n.z*n.z;

    // Translation component to account for plane not at origin
    // t = 2 * d * n
    M[3][0] = 2*d*n.x;
    M[3][1] = 2*d*n.y;
    M[3][2] = 2*d*n.z;

    return M;
}

void drawScene(glm::mat4 projectionTransform, glm::mat4 viewTransform, glm::mat4 mirrorMat = glm::mat4(1.0f)) {

    glUniformMatrix4fv(glGetUniformLocation(shader, "projectionTransform"), 1, GL_FALSE, glm::value_ptr(projectionTransform));
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewTransform"), 1, GL_FALSE, glm::value_ptr(viewTransform));

    glUniformMatrix4fv(glGetUniformLocation(shader, "modelTransform"), 1, GL_FALSE, glm::value_ptr(mirrorMat));

    // Renders:
    // 1) Floor Mesh: hasNormal, No for the rest
    glUniform1i(glGetUniformLocation(shader, "hasNormal"), 1); 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glBindVertexArray(vaos[0]);
    glDrawArrays(GL_TRIANGLES, 0, FloorMesh.size() / 11);

    // 2) Bricks With Parallax: hasNormal, No for the rest
    // No need to set hasNormal, use from previous draw
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[2]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[3]);
    glBindVertexArray(vaos[1]);
    glDrawArrays(GL_TRIANGLES, 0, BricksParallax.size() / 11);

    glUniform1i(glGetUniformLocation(shader, "hasNormal"), 0); 
    // 3) Lower Building: Just Use Diffuse, no normal nor specular
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[5]);
    glBindVertexArray(vaos[3]);
    glDrawArrays(GL_TRIANGLES, 0, LowerBuilding.size() / 11);


    // 5) Higher Building: Just Use Diffuse, no normal nor specular
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[5]); // uses same texture as other 
    glBindVertexArray(vaos[5]);
    glDrawArrays(GL_TRIANGLES, 0, HigherBuilding.size() / 11);


    // 5) Tree Bark: Just Use Diffuse, no normal nor specular
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[9]); // uses same texture as other 
    glBindVertexArray(vaos[8]);
    glDrawArrays(GL_TRIANGLES, 0, TreeBark.size() / 11);

    // 5) Mirror Plane
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, texture[6]); // Temp/black Pic
    // glBindVertexArray(vaos[10]);
    // glDrawArrays(GL_TRIANGLES, 0, MirrorPlane.size() / 11);

    // 6) Side Station
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[11]);
    glBindVertexArray(vaos[11]);
    glDrawArrays(GL_TRIANGLES, 0, SideStation.size() / 11);

    // 7) Office
    glUniform1i(glGetUniformLocation(shader, "hasNormal"), 1); 
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, texture[12]);
    // glActiveTexture(GL_TEXTURE1);
    // glBindTexture(GL_TEXTURE_2D, texture[13]);
    // glBindVertexArray(vaos[12]);
    // glDrawArrays(GL_TRIANGLES, 0, Office.size() / 11);
    
    // 8) Bus Station
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[14]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[15]);
    glBindVertexArray(vaos[13]);
    glDrawArrays(GL_TRIANGLES, 0, BusStation.size() / 11);

    
    // 9) Miscellaneous
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[16]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[17]);
    glBindVertexArray(vaos[14]);
    glDrawArrays(GL_TRIANGLES, 0, Miscellaneous.size() / 11);
    glUniform1i(glGetUniformLocation(shader, "hasNormal"), 0);

    // 10) Water
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[18]);
    glBindVertexArray(vaos[15]);
    glDrawArrays(GL_TRIANGLES, 0, Water.size() / 11);


    glUniform1i(glGetUniformLocation(shader, "hasNormal"), 1);
    glUniform1i(glGetUniformLocation(shader, "hasSpecular"), 1);
    // 11) Station
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[19]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[20]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture[21]);
    glBindVertexArray(vaos[16]);
    glDrawArrays(GL_TRIANGLES, 0, TrainStation.size() / 11);
    // 12) Train Carts
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[22]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[23]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture[24]);
    glBindVertexArray(vaos[17]);
    glDrawArrays(GL_TRIANGLES, 0, TrainCart.size() / 11);
    glUniform1i(glGetUniformLocation(shader, "hasNormal"), 0);
    glUniform1i(glGetUniformLocation(shader, "hasSpecular"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[25]);
    glBindVertexArray(vaos[18]);
    glDrawArrays(GL_TRIANGLES, 0, LampPost.size() / 11);

    glUniform1i(glGetUniformLocation(shader, "isEmissive"), 1); // for bloom on lamp bulbs
    glUniform3f(glGetUniformLocation(shader, "emissiveColor"), 3.0f, 2.5f, 1.5f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[26]);
    glBindVertexArray(vaos[19]);
    glDrawArrays(GL_TRIANGLES, 0, LampBulb.size() / 11);
    glUniform1i(glGetUniformLocation(shader, "isEmissive"),  0);


    /*---------------- INSTANCING FISH -----------------*/
    // computeNextFishStates(static_cast<float>(glfwGetTime()));

    // update fish matrices
    for (int i = 0; i < NUM_FISH; i++) {
        const Fish& f = fishes[i];
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, f.position);
        m *= glm::mat4_cast(f.orientation);
        // m = glm::scale(m, glm::vec3(0.5f));
        fishMatrices[i] = mirrorMat * m;
    }

    glBindBuffer(GL_ARRAY_BUFFER, instancedVboMatrix);
    glBufferSubData(GL_ARRAY_BUFFER, 0, fishMatrices.size() * sizeof(glm::mat4), fishMatrices.data());

    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projectionTransform"), 1, GL_FALSE, glm::value_ptr(projectionTransform));
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewTransform"), 1, GL_FALSE, glm::value_ptr(viewTransform));
    glUniform1i(glGetUniformLocation(shader, "isInstanced"), 1);
    glUniform1i(glGetUniformLocation(shader, "isEmissive"),  1);
    glUniform3f(glGetUniformLocation(shader, "emissiveColor"), 2.5f, 2.0f, 0.8f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[8]);
    // glActiveTexture(GL_TEXTURE1);
    // glBindTexture(GL_TEXTURE_2D, texture[10]);
    // glActiveTexture(GL_TEXTURE2);
    // glBindTexture(GL_TEXTURE_2D, texture[11]);
    
    
    glBindVertexArray(instancedVao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, InstanceMesh.size() / 11, NUM_FISH);
    
    glUniform1i(glGetUniformLocation(shader, "isInstanced"), 0);
    glUniform1i(glGetUniformLocation(shader, "isEmissive"),  0);
    /*--------------------------------------------------*/

    // --- Windows (reflective) ---
    if (mirrorMat == glm::mat4(1.0f)) {
        glUniform1i(glGetUniformLocation(shader, "isReflective"), 1);
        glUniform1i(glGetUniformLocation(shader, "hasNormal"), 0);
        glUniform1i(glGetUniformLocation(shader, "hasSpecular"), 0);
        glUniform1i(glGetUniformLocation(shader, "isTile"), 0);

        // Upload inverse view rotation so the shader can convert
        // reflection vectors from camera space to world space
        glm::mat3 invViewRot = glm::transpose(glm::mat3(viewTransform));
        glUniformMatrix3fv(glGetUniformLocation(shader, "inverseViewRotation"),
                        1, GL_FALSE, glm::value_ptr(invViewRot));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[6]); // window diffuse

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

        // lower windows
        glBindVertexArray(vaos[4]);
        glDrawArrays(GL_TRIANGLES, 0, LowerWindow.size() / 11);

        // higher windows
        glBindVertexArray(vaos[6]);
        glDrawArrays(GL_TRIANGLES, 0, HigherWindow.size() / 11);

        // reset
        glUniform1i(glGetUniformLocation(shader, "isReflective"), 0);

        // debug cube
        if (showDebugCube) {
            glUniform1i(glGetUniformLocation(shader, "isReflective"), 1);
            glUniform1i(glGetUniformLocation(shader, "hasNormal"),    0);
            glUniform1i(glGetUniformLocation(shader, "hasSpecular"),  0);
            glUniform1i(glGetUniformLocation(shader, "isTile"),       0);
            glUniform1i(glGetUniformLocation(shader, "isAlphaBlended"), 0);

            // Place the cube somewhere visible — adjust as needed
            glm::mat4 cubeModel = glm::mat4(1.0f);
            cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, 1.0f, 0.0f));
            cubeModel = glm::scale(cubeModel, glm::vec3(2.0f));  // 2 unit cube
            glUniformMatrix4fv(glGetUniformLocation(shader, "modelTransform"),
                            1, GL_FALSE, glm::value_ptr(cubeModel));

            // inverseViewRotation converts reflection dir from camera to world space
            glm::mat3 invViewRot = glm::transpose(glm::mat3(viewTransform));
            glUniformMatrix3fv(glGetUniformLocation(shader, "inverseViewRotation"),
                            1, GL_FALSE, glm::value_ptr(invViewRot));

            // No diffuse texture needed for a pure reflection test,
            // but the shader still samples diffuseMap so bind something valid
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture[6]);

            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

            glBindVertexArray(debugCubeVao);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Reset
            glUniform1i(glGetUniformLocation(shader, "isReflective"), 0);
            glUniformMatrix4fv(glGetUniformLocation(shader, "modelTransform"),
                            1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        }
    }

    // GRASS
    if (showGrassLeaves) {
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUniform1i(glGetUniformLocation(shader, "isAlphaBlended"), 1);
        glUniform1f(glGetUniformLocation(shader, "alphaThreshold"), 0.1f);
        glUniform1i(glGetUniformLocation(shader, "hasNormal"), 0);
        glUniform1i(glGetUniformLocation(shader, "hasSpecular"), 0);
        glUniform1i(glGetUniformLocation(shader, "isTile"), 0);

        glm::mat4 identityModel = mirrorMat * glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader, "modelTransform"),
                        1, GL_FALSE, glm::value_ptr(identityModel));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[4]);
        glBindVertexArray(vaos[2]);
        glDrawArrays(GL_TRIANGLES, 0, GrassMesh.size() / 11);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[10]);
        glBindVertexArray(vaos[9]);
        glDrawArrays(GL_TRIANGLES, 0, TreeLeaves.size() / 11);

        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glUniform1i(glGetUniformLocation(shader, "isAlphaBlended"), 0);
    }
}

void drawPostProcess() {
    // pass 2: post process bloom
    glDisable(GL_DEPTH_TEST); // depth not needed for post process
    if (enableBloom) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingFbo);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(bloomThresholdShader);
        glUniform1f(glGetUniformLocation(bloomThresholdShader, "threshold"), bloomThreshold);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
        glBindVertexArray(quadVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // pass 3: ping pong gaussian blur
        glUseProgram(bloomBlurShader);
        bool horizontal = true;
        for (int i = 0; i < bloomPasses; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, horizontal ? pongFbo : pingFbo);
            glClear(GL_COLOR_BUFFER_BIT);
            glUniform1i(glGetUniformLocation(bloomBlurShader, "horizontal"), horizontal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, horizontal ? pingTexture : pongTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            horizontal = !horizontal;
        }

        GLuint blurResult = (bloomPasses % 2 == 0) ? pingTexture : pongTexture;

        // pass 4: composite bloom with original scene
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default screen framebuffer
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(bloomCompositeShader);
        glUniform1f(glGetUniformLocation(bloomCompositeShader, "bloomStrength"), bloomStrength);
        glUniform1f(glGetUniformLocation(bloomCompositeShader, "exposure"),      bloomExposure);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrColorTexture); // original scene
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, blurResult); // blurred bright regions
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else { // for tonemap and gamma correction without bloom since scene is rendered in HDR framebuffer and not directly to screen
       glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(bloomCompositeShader);
        glUniform1f(glGetUniformLocation(bloomCompositeShader, "bloomStrength"), 0.0f); // no bloom added
        glUniform1f(glGetUniformLocation(bloomCompositeShader, "exposure"),      bloomExposure);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingTexture); // bind something valid — multiplied by 0 anyway
        glBindVertexArray(quadVao);
        glDrawArrays(GL_TRIANGLES, 0, 6); 
    }
    
    glEnable(GL_DEPTH_TEST); // restore for next frame
}

// called by the main function to do rendering per frame
void render()
{
    // render cubemap
    if (cubemapNeedsRender) {
        // Run shadow passes first so cubemap capture has valid shadow maps
        int dirIdx = 0, spotIdx = 0;
        for (auto* light : lights) {
            if (light->type == Light::DIRECTIONAL) renderDirectionalShadows(dirIdx++, *light);
            else if (light->type == Light::SPOTLIGHT) renderSpotShadows(spotIdx++, *light);
        }
        renderCubemap();
        cubemapNeedsRender = false;
    }   

    // draw shadow map
    if (enableShadows) {
        int dirIdx = 0, spotIdx = 0;
        for (auto* light : lights) {
            if (light->type == Light::DIRECTIONAL) {
                renderDirectionalShadows(dirIdx++, *light);
            } 
            else if (light->type == Light::SPOTLIGHT) {
                renderSpotShadows(spotIdx++, *light);
            } 
            else if (light->type == Light::POINT) {
                // TODO: lol
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFbo); // bind HDR framebuffer for main scene rendering

    // before drawing the final scene, we need to set drawing to the whole window
    int width, height;
    glfwGetFramebufferSize(pWindow, &width, &height);
    glViewport(0, 0, width, height);

    // clear the whole frame
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // using our shader program...
    glUseProgram(shader);
    
    glUniform1f(glGetUniformLocation(shader, "time"), (float)glfwGetTime());
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

    
    // uploading camera position
    glUniform3fv(glGetUniformLocation(shader, "cameraWorldPos"),
             1, glm::value_ptr(active_camera->position));


    // ... set up the model matrix... (just identity for this demo)
    glm::mat4 modelTransform = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "modelTransform"),
                       1, GL_FALSE, glm::value_ptr(modelTransform));

   
    
    uploadLightUniforms(viewTransform);
    

    glUniform1i(glGetUniformLocation(shader, "enableShadows"), enableShadows);


    if (enableShadows) {
        // Upload light-space transform matrices
        for (int i = 0; i < (int)directionalLightTransforms.size(); i++) {
            std::string name = "directionalLightTransforms[" + std::to_string(i) + "]";
            glUniformMatrix4fv(glGetUniformLocation(shader, name.c_str()),
                            1, GL_FALSE, glm::value_ptr(directionalLightTransforms[i]));
        }
        for (int i = 0; i < (int)spotLightTransforms.size(); i++) {
            std::string name = "spotLightTransforms[" + std::to_string(i) + "]";
            glUniformMatrix4fv(glGetUniformLocation(shader, name.c_str()),
                            1, GL_FALSE, glm::value_ptr(spotLightTransforms[i]));
        }

        // Bind the shadow arrays to their fixed units
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D_ARRAY, directionalShadowArray);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D_ARRAY, spotShadowArray);

        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_3D, offsetTexture);
        glUniform1i(glGetUniformLocation(shader, "offsetTexture"), 12);
    }

    // Setting Up Default Uniforms
    glUniform1i(glGetUniformLocation(shader, "isInstanced"), 0);
    glUniform1i(glGetUniformLocation(shader, "hasNormal"), 0); 
    glUniform1i(glGetUniformLocation(shader, "hasSpecular"), 0);
    glUniform1i(glGetUniformLocation(shader, "isTile"), 0);
    glUniform1i(glGetUniformLocation(shader, "isAlphaBlended"), 0);

    glm::vec3 n = glm::normalize(glm::vec3(-0.9848f, 0.1736f, 0.0f));
    glm::vec3 mirrorPoint = glm::vec3(19.272734f, 2.855113f, 5.277397f);
    float d = glm::dot(n, mirrorPoint); // used for reflection matrix
    glm::mat4 mirrorMatrix = buildReflectionMatrix(n, d);

    glm::vec3 clipNormal = -n;
    float clipD = glm::dot(clipNormal, mirrorPoint);
    glm::vec4 clipPlane = glm::vec4(clipNormal.x, clipNormal.y, clipNormal.z, -clipD);

    // --- PASS 1: Draw mirror shape into stencil buffer ---
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // don't write color
    glDepthMask(GL_FALSE);                               // don't write depth

    // draw ONLY the mirror quad
    glBindVertexArray(vaos[10]);
    glDrawArrays(GL_TRIANGLES, 0, MirrorPlane.size() / 11);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);

    // --- PASS 2: Draw reflected scene, clipped to stencil ---
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0x00); // don't modify stencil anymore

    glDisable(GL_CULL_FACE);

    // glm::vec4 clipPlane = glm::vec4(-n.x, -n.y, -n.z, d); // ax+by+cz+d form
    glEnable(GL_CLIP_DISTANCE0);
    glUniform4fv(glGetUniformLocation(shader, "clipPlane"), 1, glm::value_ptr(clipPlane));


    glFrontFace(GL_CW); // flip winding for reflected geometry
    drawScene(projectionTransform, viewTransform, mirrorMatrix);
    glFrontFace(GL_CCW);

    glDisable(GL_CLIP_DISTANCE0);
    glEnable(GL_CULL_FACE);

    // --- PASS 3: Clear stencil, draw real world normally ---
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);
    glDisable(GL_STENCIL_TEST);

    drawScene(projectionTransform, viewTransform); // identity mirrorMat, normal world
    drawPostProcess();

    // print main camera pos every 1 second for debugging
    if (glfwGetTime() - lastPrintTime >= 1.0f) {
        std::cout << "Camera Position: (" 
                  << active_camera->position.x << ", " 
                  << active_camera->position.y << ", " 
                  << active_camera->position.z << ")" << std::endl;
        lastPrintTime = glfwGetTime();
    }

}

/*****************************************************************************/

// for continuosly checking if certain keys are pressed and moving the camera accordingly
void processInput(GLFWwindow *pWindow, float deltaTime) {
    float cameraSpeed = 1.5f * deltaTime;

    if (glfwGetKey(pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraSpeed *= 2.0f;

    if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS)
        active_camera->position += active_camera->front * cameraSpeed;

    if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS)
        active_camera->position -= active_camera->front * cameraSpeed;

    if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS)
        active_camera->position += glm::cross(active_camera->front, active_camera->up) * cameraSpeed;

    if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS)
        active_camera->position -= glm::cross(active_camera->front, active_camera->up) * cameraSpeed;

    if (glfwGetKey(pWindow, GLFW_KEY_Q) == GLFW_PRESS)
        active_camera->position += active_camera->up * cameraSpeed;

    if (glfwGetKey(pWindow, GLFW_KEY_E) == GLFW_PRESS)
        active_camera->position -= active_camera->up * cameraSpeed;

    if (glfwGetKey(pWindow, GLFW_KEY_Z) == GLFW_PRESS)
    {
        if (active_camera->owner && active_camera->owner->type == Light::SPOTLIGHT) {
            Light* light = active_camera->owner;
            light->outer_cutoff = glm::min(light->outer_cutoff + 30.0f * deltaTime, 90.0f);
        }
        else {
            active_camera->fov = glm::min(active_camera->fov + 30.0f * deltaTime, 90.0f);
        }
    }

    if (glfwGetKey(pWindow, GLFW_KEY_X) == GLFW_PRESS)
    {
        if (active_camera->owner && active_camera->owner->type == Light::SPOTLIGHT) {
            Light* light = active_camera->owner;
            light->outer_cutoff = glm::max(light->outer_cutoff - 30.0f * deltaTime, light->inner_cutoff);
        }
        else {
            active_camera->fov = glm::max(active_camera->fov - 30.0f * deltaTime, 1.0f);
        }
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
    // // pressing Esc closes the window
    // if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    //     glfwSetWindowShouldClose(pWindow, GL_TRUE);
    
    if (action != GLFW_PRESS) return;

    switch (key)
    {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(pWindow, GL_TRUE);
            break;

        case GLFW_KEY_F:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;

        case GLFW_KEY_L:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;

        case GLFW_KEY_P:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;

        case GLFW_KEY_R:
            active_camera->position = glm::vec3(0.0f, 0.0f, 3.0f);
            active_camera->yaw = -90.0f;
            active_camera->pitch = 0.0f;
            active_camera->fov = 45.0f;
            active_camera->front = glm::vec3(0.0f, 0.0f, -1.0f);
            break;

        case GLFW_KEY_1:
            active_camera = &main_camera;
            break;
        
        case GLFW_KEY_2:
            active_camera = &main_light.cam;
            break;

        case GLFW_KEY_3:
            active_camera = &spotlight1.cam;
            break;

        case GLFW_KEY_4:
            active_camera = &spotlight2.cam;
            break;
        case GLFW_KEY_5:
            enableShadows = !enableShadows;
            break;
        case GLFW_KEY_G:
            showGrassLeaves = !showGrassLeaves;
            break;
        case GLFW_KEY_C:
            showDebugCube = !showDebugCube;
            break;
        case GLFW_KEY_B:
            enableBloom = !enableBloom;
            break;
    }
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
