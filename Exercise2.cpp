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
std::vector<float> train = {};
std::vector<float> water = {};
std::vector<float> rainbow = {};
std::vector<float> fish = {};

// OpenGL object IDs
GLuint vao;
GLuint vbo;
GLuint instancedVao;
GLuint instancedVbo;
GLuint instancedVboMatrix;
GLuint shader;
GLuint simple_shader;
GLuint texture[12];

int vertex_data_num =  5;
GLuint vaos[5], vbos[5];
std::vector<float> vertex_data[5];
size_t data_sizes[5];

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

/*------------------FISH--------------------*/


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
    readModelData(train, "train_data.txt");
    readModelData(water, "water_data.txt");
    readModelData(fish, "fish_data.txt");
    readModelData(rainbow, "rainbow_data.txt");

    vertex_data[0] = station;
    vertex_data[1] = train;
    vertex_data[2] = water;
    vertex_data[3] = rainbow;
    vertex_data[4] = std::vector<float>(std::begin(tankVertices), std::end(tankVertices));

    setupLights();

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
    shader = gdevLoadShader("Exercise2-advanced.vs", "Exercise2-advanced.fs");
    simple_shader = gdevLoadShader("Exercise2-simple.vs", "Exercise2-simple.fs");
    if (!shader || !simple_shader) return false;

    // since we now use multiple textures, we need to set the texture channel for each texture
    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "diffuseMap"), 0);
    glUniform1i(glGetUniformLocation(shader, "normalMap"),  1);
    glUniform1i(glGetUniformLocation(shader, "specularMap"),  2);

    glUseProgram(simple_shader);
    glUniform1i(glGetUniformLocation(simple_shader, "diffuseMap"), 0);


    // load our textures
    texture[0] = gdevLoadTexture("tex-station.png", GL_REPEAT, true, true);
    texture[1] = gdevLoadTexture("TrainStationNormal.png", GL_REPEAT, true, true);
    texture[2] = gdevLoadTexture("TrainStationSpecular.png", GL_REPEAT, true, true);
    texture[3] = gdevLoadTexture("tex-train.png", GL_REPEAT, true, true);
    texture[4] = gdevLoadTexture("tex-water.png", GL_REPEAT, true, true);
    texture[5] = gdevLoadTexture("tex-disp.png", GL_REPEAT, true, true);
    texture[6] = gdevLoadTexture("tex-rainbow.png", GL_REPEAT, true, true);
    texture[7] = gdevLoadTexture("tex-fish.png", GL_REPEAT, true, true);
    texture[8] = gdevLoadTexture("TrainCartNormal.png", GL_REPEAT, true, true);
    texture[9] = gdevLoadTexture("TrainCartSpecular.png", GL_REPEAT, true, true);
    texture[10] = gdevLoadTexture("FishNormal.png", GL_REPEAT, true, true);
    texture[11] = gdevLoadTexture("FishSpecular.png", GL_REPEAT, true, true);
    if (! texture[0] || ! texture[1] || !texture[2]
        || !texture[3] || !texture[4] || !texture[5]
        || !texture[6] || !texture[7] || !texture[8] || !texture[9])
        return false;

    /*---------------- INSTANCING FISH -----------------*/
    initFish();
    makeAABBs(); 

    glGenVertexArrays(1, &instancedVao);
    glGenBuffers(1, &instancedVbo);      
    glGenBuffers(1, &instancedVboMatrix); 

    glBindVertexArray(instancedVao);

    glBindBuffer(GL_ARRAY_BUFFER, instancedVbo);
    glBufferData(GL_ARRAY_BUFFER, fish.size() * sizeof(float), fish.data(), GL_STATIC_DRAW);

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
    /*--------------------------------------------------*/

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
    glUniform1i(glGetUniformLocation(simple_shader, "isInstanced"), 0);

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
    glUniform3fv(glGetUniformLocation(shader, "dir_light.color"),
                 1, glm::value_ptr(main_light.color));
    glUniform1f(glGetUniformLocation(shader, "dir_light.specular_exponent"), main_light.specular_exponent);

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
        glUniform3fv(glGetUniformLocation(shader, (base + "color").c_str()),
                     1, glm::value_ptr(spotlights[i].color));
        glUniform1f(glGetUniformLocation(shader, (base + "specular_exponent").c_str()), spotlights[i].specular_exponent);
    }


    // Drawing Station
    glUniform1i(glGetUniformLocation(shader, "isInstanced"), 0);
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

    // // Drawing Train
    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "isInstanced"), 0);
    // glUniformMatrix4fv(glGetUniformLocation(shader, "projectionTransform"), 1, GL_FALSE, glm::value_ptr(projectionTransform));
    // glUniformMatrix4fv(glGetUniformLocation(shader, "viewTransform"), 1, GL_FALSE, glm::value_ptr(viewTransform));
    // glUniformMatrix4fv(glGetUniformLocation(shader, "modelTransform"), 1, GL_FALSE, glm::value_ptr(modelTransform));

    // glUniform3fv(glGetUniformLocation(simple_shader, "lightPosition"), 1, glm::value_ptr(main_light.cam.position));
    // glUniform1i(glGetUniformLocation(simple_shader, "isInstanced"), 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[3]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[8]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture[9]);
    // glUniform1i(glGetUniformLocation(simple_shader, "diffuseMap"), 0);

    glBindVertexArray(vaos[1]);
    glDrawArrays(GL_TRIANGLES, 0, train.size() / 11);

    // Floor:
    glUseProgram(simple_shader);
    float currentTime = (float)glfwGetTime();
    glUniform1f(glGetUniformLocation(simple_shader, "time"), currentTime);
    glUniform1i(glGetUniformLocation(simple_shader, "isInstanced"), 0);

    glUniformMatrix4fv(glGetUniformLocation(simple_shader, "projectionTransform"), 1, GL_FALSE, glm::value_ptr(projectionTransform));
    glUniformMatrix4fv(glGetUniformLocation(simple_shader, "viewTransform"), 1, GL_FALSE, glm::value_ptr(viewTransform));

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

    computeNextFishStates1(static_cast<float>(glfwGetTime()));

    // update fish matrices
    for (int i = 0; i < NUM_FISH; i++) {
        const Fish& f = fishes[i];
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, f.position);
        m *= glm::mat4_cast(f.orientation);
        // m = glm::scale(m, glm::vec3(0.5f));
        fishMatrices[i] = m;
    }

    glBindBuffer(GL_ARRAY_BUFFER, instancedVboMatrix);
    glBufferSubData(GL_ARRAY_BUFFER, 0, fishMatrices.size() * sizeof(glm::mat4), fishMatrices.data());

    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projectionTransform"), 1, GL_FALSE, glm::value_ptr(projectionTransform));
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewTransform"), 1, GL_FALSE, glm::value_ptr(viewTransform));
    glUniform1i(glGetUniformLocation(shader, "isInstanced"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[7]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[10]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture[11]);
    // glUniform1i(glGetUniformLocation(shader, "diffuseMap"), 0);

    glBindVertexArray(instancedVao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, fish.size() / 11, NUM_FISH);

    glBindVertexArray(0);

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
        if (active_camera->owner && active_camera->owner->type == Light::SPOTLIGHT) {
            Light* light = active_camera->owner;
            light->outer_cutoff += 30.0f * deltaTime;
            if (light->outer_cutoff > 90.0f) {
                light->outer_cutoff = 90.0f;
            }
        }
        else {
            active_camera->fov += 30.0f * deltaTime;
            if (active_camera->fov > 90.0f) {
                active_camera->fov = 90.0f;
            }
        }
    }

    if (glfwGetKey(pWindow, GLFW_KEY_X) == GLFW_PRESS) {
        if (active_camera->owner && active_camera->owner->type == Light::SPOTLIGHT) {
            Light* light = active_camera->owner;
            light->outer_cutoff -= 30.0f * deltaTime;
            if (light->outer_cutoff < light->inner_cutoff) {
                light->outer_cutoff = light->inner_cutoff;
            }
        }
        else {
            active_camera->fov -= 30.0f * deltaTime;
            if (active_camera->fov < 1.0f) {
                active_camera->fov = 1.0f;
            }
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
