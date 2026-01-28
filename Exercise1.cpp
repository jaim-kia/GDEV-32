/******************************************************************************
 * This demo draws a triangle by defining its vertices in 3 dimensions
 * (the 3rd dimension is currently ignored and is just set to 0).
 *
 * The drawing is accomplished by:
 * - Uploading the vertices to the GPU using a Vertex Buffer Object (VBO).
 * - Specifying the vertices' format using a Vertex Array Object (VAO).
 * - Using a GLSL shader program (consisting of a simple vertex shader and a
 *   simple fragment shader) to actually draw the vertices as a triangle.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <gdev.h>
#include <vector>

// file reading and formatting
#include <fstream>
#include <sstream>
#include <iomanip>

// change this to your desired window attributes
#define WINDOW_WIDTH  1600
#define WINDOW_HEIGHT 900
#define WINDOW_TITLE  "Exercise 1"
GLFWwindow *pWindow;

glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float fov = 90.0f;

float lastX = WINDOW_WIDTH/2.0f;
float lastY = WINDOW_HEIGHT/2.0f;
bool firstMouse = true;

std::vector<float> station = {};
std::vector<float> train = {};
std::vector<float> rainbow = {};
std::vector<float> fish = {};
std::vector<float> cloud = {};
std::vector<float> water = {};
std::vector<float> skybox = {};

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
// define OpenGL object IDs to represent the vertex array and the shader program in the GPU
// GLuint vao_station, vao_train;         // vertex array object (stores the render state for our vertex array)
// GLuint vbo_station, vbo_train;         // vertex buffer object (reserves GPU memory for our vertex array)
int vertex_data_num =  6;
GLuint vaos[6], vbos[6];
std::vector<float> vertex_data[6];
size_t data_sizes[6];

GLuint shader;      // combined vertex and fragment shader
GLuint texture_station;
GLuint texture_train;
GLuint texture_rainbow;
GLuint texture_fish;
GLuint texture_water;
GLuint texture_disp_smoke;
GLuint texture_skybox;

GLuint instancedVao;
GLuint instancedVbo;
GLuint instancedShader;
GLuint instancedTexture;
GLuint instancedVboMatrix;

// fish parameters
const int NUM_FISH = 200;
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

        // glm::vec3 steering = flow + avoid + wall + obstacle;

        // glm::vec3 desiredDir = glm::normalize(steering);  
        // glm::vec3 currentDir = glm::normalize(f.velocity);
        // glm::vec3 newDir = glm::normalize(glm::mix(currentDir, desiredDir, TURN_RATE)); // smooth turning

        // // new basis
        // glm::vec3 forward = newDir;
        // glm::vec3 right = glm::normalize(glm::cross(WORLD_UP, forward));
        // glm::vec3 up = glm::normalize(glm::cross(forward, right));

        // glm::mat3 rotationMatrix(right, up, forward);
        // f.orientation = glm::quat_cast(rotationMatrix);

        // f.velocity = forward;
        // f.position += f.velocity * f.speed * (DT / 16.0f);

        glm::vec3 desiredVelocity = glm::normalize(flow + avoid + wall + obstacle);
        f.velocity = glm::mix(f.velocity, desiredVelocity, TURN_RATE); // smooth turning 
        f.position += f.velocity * f.speed * (DT / 16.0f); // adjust speed based on frame time 
        f.orientation = glm::quatLookAt(f.velocity, glm::vec3(0.0f, 1.0f, 0.0f)); // orient

    }
}

void computeNextFishStates1(float time) {
    for (auto& f : fishes) {
        glm::vec3 flow = flowField(f.position, time) * FLOW_WEIGHT;
        glm::vec3 avoid = avoidNeighbors(f, fishes) * AVOID_WEIGHT;
        glm::vec3 wall = avoidWalls(f) * AVOID_WEIGHT;

        // get obstacles
        glm::vec3 obstacle(0.0f);
        for (const AABB& box : aabbs) {
            obstacle += avoidBoundingBox(f, box.min, box.max) * OBSTACLE_WEIGHT;
        }

        glm::vec3 steering = flow + avoid + wall + obstacle;

        glm::vec3 desiredDir = glm::normalize(steering);  
        glm::vec3 currentDir = glm::normalize(f.velocity);
        glm::vec3 newDir = glm::normalize(glm::mix(currentDir, desiredDir, TURN_RATE)); // smooth turning

        // new basis
        glm::vec3 forward = newDir;
        glm::vec3 right = glm::normalize(glm::cross(WORLD_UP, forward));
        glm::vec3 up = glm::normalize(glm::cross(forward, right));

        glm::mat3 rotationMatrix(right, up, forward);
        f.orientation = glm::quat_cast(rotationMatrix);

        f.velocity = forward;
        f.position += f.velocity * f.speed * (DT / 16.0f);
    }
}

// called by the main function to do initial setup, such as uploading vertex
// arrays, shader programs, etc.; returns true if successful, false otherwise
bool setup()
{
    readModelData(station, "station_data.txt");
    readModelData(train, "train_data.txt");
    readModelData(rainbow, "rainbow_data.txt");
    readModelData(fish, "fish_data.txt");
    readModelData(water, "water_data.txt");
    readModelData(skybox, "skybox_data.txt");
  
    vertex_data[0] = station;
    vertex_data[1] = train;
    vertex_data[2] = rainbow;
    vertex_data[3] = std::vector<float>(std::begin(tankVertices), std::end(tankVertices));
    vertex_data[4] = water;
    vertex_data[5] = skybox;

    makeAABBs();

    // generate the VAO and VBO objects and store their IDs in vao and vbo, respectively
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
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (8 * sizeof(float)));   // color

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
    }

    // loading textures
    texture_station = gdevLoadTexture("tex-merged-station.png", GL_REPEAT, true, true);
    if (! texture_station) return false;

    texture_train = gdevLoadTexture("test-train.png", GL_REPEAT, true, true);
    if (! texture_train) return false;

    texture_rainbow = gdevLoadTexture("rainbow.png", GL_REPEAT, true, true);
    if (! texture_rainbow) return false;

    texture_water = gdevLoadTexture("tex-water-2.png", GL_REPEAT, true, true);
    if (! texture_water) return false;

    texture_disp_smoke = gdevLoadTexture("tex-disp.png", GL_REPEAT, true, true);
    if (! texture_disp_smoke) return false;

    texture_skybox = gdevLoadTexture("tex-skybox.png", GL_REPEAT, true, true);
    if (! texture_disp_smoke) return false;

    // load our shader program
    shader = gdevLoadShader("Exercise1.vs", "Exercise1.fs");
    if (! shader)
        return false;

    //------------- Instanced VAO stuff ------------------------
    initFish();

    glGenVertexArrays(1, &instancedVao);
    glGenBuffers(1, &instancedVbo);
    
    // bind the newly-created VAO to make it the current one that OpenGL will apply state changes to
    glBindVertexArray(instancedVao);

    // upload our vertex array data to the newly-created VBO
    glBindBuffer(GL_ARRAY_BUFFER, instancedVbo);
    glBufferData(GL_ARRAY_BUFFER, fish.size() * sizeof(float), fish.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) 0);                     // position
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (3 * sizeof(float)));   // texture coord
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (5 * sizeof(float)));   // normal
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*) (8 * sizeof(float)));   // color
    // enable the newly-created layout location 0;
    // this shall be used by our vertex shader to read the vertex's x, y, and z
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    // instancedTexture = gdevLoadTexture("rainbow.png", GL_REPEAT, true, true);
    // if (! instancedTexture) return false;
    texture_fish = gdevLoadTexture("test-fish.png", GL_REPEAT, true, true);
    if (! texture_fish) return false;

    // load our shader program
    instancedShader = gdevLoadShader("Exercise1_fish.vs", "Exercise1_fish.fs");
    if (! instancedShader)
        return false;

    //--------------------------------------------------------

    
    // fish model matrices
    glGenBuffers(1, &instancedVboMatrix);
    glBindBuffer(GL_ARRAY_BUFFER, instancedVboMatrix);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * NUM_FISH, &fishMatrices[0], GL_STATIC_DRAW);
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

    return true;
}


// called by the main function to do rendering per frame
void render()
{
    float t = glfwGetTime();
    // clear the whole frame
    glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // ghibli sky blue
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // using our shader program...
    glUseProgram(shader);
    glUniform1f(glGetUniformLocation(shader, "time"), t);
    glEnable(GL_CULL_FACE); 
    glEnable(GL_DEPTH_TEST); // enable OpenGL's hidden surface removal
    glm::mat4 projview;
    projview = glm::perspective(glm::radians(fov), (float) WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);


    glUniform3f(glGetUniformLocation(shader, "eye"), cameraPos.x, cameraPos.y, cameraPos.z);


    projview *= glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    
    glm::mat4 model(1.0f);
    // model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // to 0,0,0


    glUniformMatrix4fv(glGetUniformLocation(shader, "projview"),
                        1, GL_FALSE, glm::value_ptr(projview));
    

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"),
                        1, GL_FALSE, glm::value_ptr(model));
 

    // set the active texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_station);

    // then connect each texture unit to a sampler2D in the fragment shader
    glUniform1i(glGetUniformLocation(shader, "texture_file"), 0);
    
    // ... draw our triangles
    glBindVertexArray(vaos[0]); // station
    glDrawArrays(GL_TRIANGLES, 0, (station.size() * sizeof(float)) / (11 * sizeof(float)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_train);
    glUniform1i(glGetUniformLocation(shader, "texture_file"), 0);
    
    glBindVertexArray(vaos[1]); // train
    glDrawArrays(GL_TRIANGLES, 0, (train.size() * sizeof(float)) / (11 * sizeof(float)));

    // glBindVertexArray(vaos[3]); // tank
    // glDrawArrays(GL_TRIANGLES, 0, (sizeof(tankVertices)) / (11 * sizeof(float)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_rainbow);
    glUniform1i(glGetUniformLocation(shader, "texture_file"), 0);

    glBindVertexArray(vaos[2]); // rainbow
    glDrawArrays(GL_TRIANGLES, 0, (rainbow.size() * sizeof(float)) / (11 * sizeof(float)));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_disp_smoke);
    glUniform1i(glGetUniformLocation(shader, "shaderTextureSmoke"), 1);

    glm::mat4 floorModel = glm::mat4(1.0f);
    floorModel = glm::translate(floorModel, glm::vec3(cameraPos.x, 0.0f, cameraPos.z));
    floorModel = glm::scale(floorModel, glm::vec3(10.0f, 1.0f, 10.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(floorModel));
    glUniform1i(glGetUniformLocation(shader, "isTile"), 1);
    glUniform2f(glGetUniformLocation(shader, "cameraPlanePos"), cameraPos.x, cameraPos.z);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_water);
    glUniform1i(glGetUniformLocation(shader, "texture_file"), 0);

    glBindVertexArray(vaos[4]); // water
    glDrawArrays(GL_TRIANGLES, 0, (water.size() * sizeof(float)) / (11 * sizeof(float)));
    
    glUniform1i(glGetUniformLocation(shader, "isTile"), 0);

    // glm::mat4 skyModel = glm::mat4(1.0f);
    // skyModel = glm::translate(skyModel, glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z));
    // skyModel = glm::scale(skyModel, glm::vec3(5.0f, 1.0f, 5.0f));

    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, texture_skybox);
    // glUniform1i(glGetUniformLocation(shader, "texture_file"), 0);

    // glBindVertexArray(vaos[4]); // water
    // glDrawArrays(GL_TRIANGLES, 0, (skybox.size() * sizeof(float)) / (11 * sizeof(float)));


    // computeNextFishStates(static_cast<float>(glfwGetTime()));
    computeNextFishStates1(static_cast<float>(glfwGetTime()));
    
    // update fish matrices
    for (int i = 0; i < NUM_FISH; i++) {
        const Fish& f = fishes[i];
        glm::mat4 m = glm::mat4(1.0f);

        m = glm::translate(m, f.position);
        m *= glm::mat4_cast(f.orientation);
        // m = glm::scale(m, glm::vec3(1.0f));
        fishMatrices[i] = m;
        
    }

    // update instance matrix buffer
    glBindBuffer(GL_ARRAY_BUFFER, instancedVboMatrix);
    glBufferData(GL_ARRAY_BUFFER, fishMatrices.size() * sizeof(glm::mat4), fishMatrices.data(), GL_STATIC_DRAW);

    glUseProgram(instancedShader);

    glUniformMatrix4fv(glGetUniformLocation(instancedShader, "projview"),
                        1, GL_FALSE, glm::value_ptr(projview));
 
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_fish);

    glUniform1i(glGetUniformLocation(instancedShader, "texture_file_instanced"), 1);

    glBindVertexArray(instancedVao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, (fish.size() * sizeof(float)) / (11 * sizeof(float)), NUM_FISH);

    
}


void processInput(GLFWwindow *pWindow, float deltaTime) {
    float cameraSpeed = 0.5f * deltaTime;

    if (glfwGetKey(pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        cameraSpeed *= 2.0f;
    }

    if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS) {
        cameraPos += cameraFront * cameraSpeed;
    } 
    
    if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS) {
        cameraPos += -cameraFront * cameraSpeed;
    } 

    if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS) {
        cameraPos += glm::cross(cameraFront, cameraUp) * cameraSpeed;
    }

    if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS) {
        cameraPos += -glm::cross(cameraFront, cameraUp) * cameraSpeed;
    }

    if (glfwGetKey(pWindow, GLFW_KEY_Q) == GLFW_PRESS) {
        cameraPos += cameraUp * cameraSpeed;
    }

    if (glfwGetKey(pWindow, GLFW_KEY_E) == GLFW_PRESS) {
        cameraPos += -cameraUp * cameraSpeed;
    }

    if (glfwGetKey(pWindow, GLFW_KEY_R) == GLFW_PRESS) {
        cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
        yaw = -90.0f;
        pitch = 0.0f;
        fov = 45.0f;
        cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    if (glfwGetKey(pWindow, GLFW_KEY_Z) == GLFW_PRESS) {
        fov += 30.0f * deltaTime;
        if (fov > 90.0f) {
            fov = 90.0f;
        }
    }

    if (glfwGetKey(pWindow, GLFW_KEY_X) == GLFW_PRESS) {
        fov -= 30.0f * deltaTime;
        if (fov < 1.0f) {
            fov = 1.0f;
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

}

/*****************************************************************************/

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

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) {
        pitch = 89.0f;
    }
    if (pitch < -89.0f) {
        pitch = -89.0f;
    }

    glm::vec3 cam_dir;
    cam_dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    cam_dir.y = sin(glm::radians(pitch));
    cam_dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(cam_dir);
    
}

void scroll_callback(GLFWwindow *pWindow, double xoffset, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f) {
        fov = 1.0f;
    }
    if (fov > 90.0f) {
        fov = 90.0f;
    }

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
