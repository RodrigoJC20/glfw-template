#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#include "map.h"

#include <iostream>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);

unsigned int loadTexture(const char *path);

void updatePhysics(float deltaTime);
bool AABBIntersect(const AABB& box1, const AABB& box2);
AABB GenerateBoindingBox(glm::vec3 position, float w, float h, float d);
bool checkCollision();

void setLights(Shader& shader);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;

// camera
glm::vec3 cameraStartPos ((float)MAP_ROWS / 2, 20.0f, 0.5f + (float)MAP_COLS/2);
Camera camera(cameraStartPos, glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -89.0f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool cameraFixed = true;

// game
glm::vec3 startPos(1.5f, 0.5f, 5.5f);
glm::vec3 endPos(13.5f, 0.5f, 13.5f);
const float gravity = -9.81;
bool gravityActive = true;

// player
glm::vec3 playerPos(1.5f, 3.0f, 5.5f);
glm::vec3 playerVelocity(0.0f, 0.0f, 0.0f);
const float PLAYER_SIDE = 0.6f;

// timing
float deltaTime = 0.0f; 
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(7.5f, 20.0f, 7.5f);

// lights
glm::vec3 pointLightPositions[] = {
    glm::vec3( 1.5f, 2.0f,  1.5f),
    glm::vec3( 1.5f, 2.0f,  13.5f),
    glm::vec3(13.5f, 2.0f,  1.5f),
    glm::vec3(13.5f, 2.0f,  13.5f)
};

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Renderer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGL(glfwGetProcAddress)) 
    {
        std::cout << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // build and compile our shaders program
    Shader shader("res/shaders/wall.vs", "res/shaders/wall.fs");
    Shader lightShader("res/shaders/light.vs", "res/shaders/light.fs");
    Shader floorShader("res/shaders/floor.vs", "res/shaders/floor.fs");

    unsigned int wallVBO, wallVAO;
    glGenVertexArrays(1, &wallVAO);
    glGenBuffers(1, &wallVBO);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glBindVertexArray(wallVAO);
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned floorVBO, floorVAO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glBindVertexArray(floorVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // load textures
    unsigned int diffuseMap = loadTexture("res/textures/container2.png");
    unsigned int specularMap = loadTexture("res/textures/container2_specular.png");
    unsigned int diffuseMap_floor = loadTexture("res/textures/floor.jpg");
    unsigned int diffuseMap_player = loadTexture("res/textures/player_diffuse.jpg");
    unsigned int specularMap_player = loadTexture("res/textures/player_specular.jpg");


    // shader configuration
    shader.use(); 
    shader.setInt("material.diffuse", 0);   
    shader.setInt("material.specular", 1);

    floorShader.use();
    floorShader.setInt("material.diffuse", 2);

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        if (gravityActive) 
        {
            updatePhysics(deltaTime);
        }

        // check if player is at end
        if (playerPos.x >= endPos.x - 0.20f && playerPos.x <= endPos.x + 0.20f && playerPos.z >= endPos.z - 0.20f && playerPos.z <= endPos.z + 0.20f) {
            playerPos = startPos;
            playerPos.y = 3.0f;
            playerVelocity.y = 0.0f;
        }

        // render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        setLights(shader);

        // material properties
        shader.setVec3("material.specular", 0.8f, 0.8f, 0.8f);
        shader.setFloat("material.shininess", 64.0f);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        glBindVertexArray(wallVAO);

        glm::mat4 model;

        for (int i=0; i<MAP_ROWS; i++) {
            for (int j=0; j<MAP_COLS; j++) {
                if (labyrinth[i][j] == 0) continue;
                model = glm::mat4(1.0f);
                glm::vec3 position;
                position.x = j + BLOCK_SIDE / 2;
                position.y = BLOCK_SIDE / 2;
                position.z = i + BLOCK_SIDE / 2;
                model = glm::translate(model, position);
                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
        //
        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap_player);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap_player);

        model = glm::mat4(1.0f);
        model = glm::translate(model, playerPos);
        model = glm::scale(model, glm::vec3(0.6f));
        lightShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        floorShader.use();
        setLights(floorShader);

        floorShader.setVec3("material.specular", 0.5f, 0.5f, 0.5f);
        floorShader.setFloat("material.shininess", 32.0f);


        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, diffuseMap_floor);

        floorShader.setMat4("projection", projection);
        floorShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        floorShader.setMat4("model", model);

        glBindVertexArray(floorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);

        glBindVertexArray(lightCubeVAO);

        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.4f));
        lightShader.setVec3("CubeColor", glm::vec3(1.0f, 1.0f, 1.0f));
        lightShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        for (unsigned int i = 0; i < 4; i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPositions[i]);
            model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
            lightShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        model = glm::mat4(1.0f);
        model = glm::translate(model, startPos);
        model = glm::scale(model, glm::vec3(0.2f));
        lightShader.setVec3("CubeColor", glm::vec3(0.0f, 1.0f, 0.0f));
        lightShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        model = glm::mat4(1.0f);
        model = glm::translate(model, endPos);
        model = glm::scale(model, glm::vec3(0.2f));
        lightShader.setVec3("CubeColor", glm::vec3(1.0f, 0.0f, 0.0f));
        lightShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &wallVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &wallVBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    glm::vec3 previousPos = playerPos;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) 
    {
        if (cameraFixed) {
            playerPos.z -= 3.0f * deltaTime;
            if (checkCollision()) 
            {
                playerPos.z = previousPos.z;
            }

        } else {
            camera.ProcessKeyboard(FORWARD, deltaTime);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        if (cameraFixed) {
            playerPos.z += 3.0f * deltaTime;
            if (checkCollision()) 
            {
                playerPos.z = previousPos.z;
            }
        } else {
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) 
    {
        if (cameraFixed) {
            playerPos.x -= 3.0f * deltaTime;
            if (checkCollision()) 
            {
                playerPos.x = previousPos.x;
            }
        } else {
            camera.ProcessKeyboard(LEFT, deltaTime);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) 
    {
        if (cameraFixed) {
            playerPos.x += 3.0f * deltaTime;
            if (checkCollision()) 
            {
                playerPos.x = previousPos.x;
            }
        } else {
            camera.ProcessKeyboard(RIGHT, deltaTime);
        }
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (cameraFixed) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        firstMouse = true;
        cameraFixed = !cameraFixed;
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS) { 
        Camera startCamera(cameraStartPos, glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -89.0f);
        camera = startCamera;
        playerPos = startPos;
        cameraFixed = true;
        firstMouse = true;
        gravityActive = true;
        playerPos.y = 3.0f;
        playerVelocity.y = 0.0f;
    }
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        gravityActive = !gravityActive;
    }
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void updatePhysics(float deltaTime)
{
	playerVelocity.y += gravity * deltaTime;
	playerPos.y += playerVelocity.y * deltaTime;
    if (playerPos.y < 0.3) {
        playerPos.y = 0.3;
    }
}

bool AABBIntersect(const AABB& box1, const AABB& box2) {
    bool xOverlap = box1.min.x <= box2.max.x && box1.max.x >= box2.min.x;
    bool yOverlap = box1.min.y <= box2.max.y && box1.max.y >= box2.min.y;
    bool zOverlap = box1.min.z <= box2.max.z && box1.max.z >= box2.min.z;
    return xOverlap && yOverlap && zOverlap;
}

AABB GenerateBoundingBox(glm::vec3 position, float w, float h, float d) {
    AABB box;
    box.min = glm::vec3(position.x - w / 2.0f, position.y - h / 2.0f, position.z - d / 2.0f);
    box.max = glm::vec3(position.x + w / 2.0f, position.y + h / 2.0f, position.z + d / 2.0f);
    return box;
}

bool checkCollision() {
    AABB playerBox = GenerateBoundingBox(playerPos, 0.6f, 0.6f, 0.6f);

    for (int z = 0; z < MAP_ROWS; ++z) {
        for (int x = 0; x < MAP_COLS; ++x) {
            if (labyrinth[z][x] == 1) {
                glm::vec3 cubePosition = glm::vec3(x + 0.5f, 0.5f, z + 0.5f); // Center of the cube
                AABB cubeBox = GenerateBoundingBox(cubePosition, 1.0f, 1.0f, 1.0f);
                if (AABBIntersect(playerBox, cubeBox)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void setLights(Shader& shader) {
    shader.setVec3("light.direction", 7.5, -1.0, 7.5f);
    shader.setVec3("viewPos", camera.Position);

    shader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f); 
    shader.setVec3("light.diffuse", 0.8f, 0.8f, 8.6f);
    shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
    
    // point light 1
    shader.setVec3("pointLights[0].position", pointLightPositions[0]);
    shader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
    shader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
    shader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
    shader.setFloat("pointLights[0].constant", 1.0f);
    shader.setFloat("pointLights[0].linear", 0.09f);
    shader.setFloat("pointLights[0].quadratic", 0.032f);
    // point light 2
    shader.setVec3("pointLights[1].position", pointLightPositions[1]);
    shader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
    shader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
    shader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
    shader.setFloat("pointLights[1].constant", 1.0f);
    shader.setFloat("pointLights[1].linear", 0.09f);
    shader.setFloat("pointLights[1].quadratic", 0.032f);
    // point light 3
    shader.setVec3("pointLights[2].position", pointLightPositions[2]);
    shader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
    shader.setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
    shader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
    shader.setFloat("pointLights[2].constant", 1.0f);
    shader.setFloat("pointLights[2].linear", 0.09f);
    shader.setFloat("pointLights[2].quadratic", 0.032f);
    // point light 4
    shader.setVec3("pointLights[3].position", pointLightPositions[3]);
    shader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
    shader.setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
    shader.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
    shader.setFloat("pointLights[3].constant", 1.0f);
    shader.setFloat("pointLights[3].linear", 0.09f);
    shader.setFloat("pointLights[3].quadratic", 0.032f);
    // spotLight
    shader.setVec3("spotLight.position", camera.Position);
    shader.setVec3("spotLight.direction", camera.Front);
    shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    shader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    shader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    shader.setFloat("spotLight.constant", 1.0f);
    shader.setFloat("spotLight.linear", 0.09f);
    shader.setFloat("spotLight.quadratic", 0.032f);
    shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));   
}
