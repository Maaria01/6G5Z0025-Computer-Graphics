#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Vertex Shader source code
const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = 2.0; // Set point size for stars
}
)glsl";

// Fragment Shader source code
const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;

void main()
{
    FragColor = vec4(objectColor, 1.0);
}
)glsl";

// Globals for camera control
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 12.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.3f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

void processInput(GLFWwindow* window)
{
    float cameraSpeed = 5.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

// Compile shader helper
GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

// Create shader program helper
GLuint createShaderProgram()
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM_LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// Sphere generation function (returns vertices and indices for a UV sphere)
void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, int latSegments, int lonSegments)
{
    for (int lat = 0; lat <= latSegments; ++lat) {
        float theta = lat * glm::pi<float>() / latSegments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int lon = 0; lon <= lonSegments; ++lon) {
            float phi = lon * 2.0f * glm::pi<float>() / lonSegments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            vertices.push_back(radius * x);
            vertices.push_back(radius * y);
            vertices.push_back(radius * z);
        }
    }

    for (int lat = 0; lat < latSegments; ++lat) {
        for (int lon = 0; lon < lonSegments; ++lon) {
            int first = (lat * (lonSegments + 1)) + lon;
            int second = first + lonSegments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
}

// Generate random star positions
std::vector<float> generateStarfield(int numStars, float radius)
{
    std::vector<float> stars;
    stars.reserve(numStars * 3);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distPos(-radius, radius);
    std::uniform_real_distribution<float> distIntensity(0.5f, 1.0f);

    for (int i = 0; i < numStars; ++i) {
        // Generate random positions within a sphere
        float x, y, z;
        do {
            x = distPos(gen);
            y = distPos(gen);
            z = distPos(gen);
        } while (x * x + y * y + z * z > radius * radius); // Ensure they're within the sphere

        stars.push_back(x);
        stars.push_back(y);
        stars.push_back(z);
    }

    return stars;
}

int main()
{
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // OpenGL version 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create Window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Solar System with Earth and Venus", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Init GLEW
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE); // Enable point size in shader

    GLuint shaderProgram = createShaderProgram();

    // Generate sphere data (for sun, earth and venus)
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(sphereVertices, sphereIndices, 1.0f, 40, 40);

    GLuint sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Generate starfield
    const int numStars = 1000;
    std::vector<float> starVertices = generateStarfield(numStars, 30.0f);

    GLuint starVAO, starVBO;
    glGenVertexArrays(1, &starVAO);
    glGenBuffers(1, &starVBO);

    glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER, starVBO);
    glBufferData(GL_ARRAY_BUFFER, starVertices.size() * sizeof(float), starVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);

        // Clear screen
        glClearColor(0.0f, 0.0f, 0.05f, 1.0f); // Dark blue background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Setup matrices
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        GLint colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // --- Render Starfield ---
        glm::mat4 starModel = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(starModel));
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // white stars

        glBindVertexArray(starVAO);
        glDrawArrays(GL_POINTS, 0, numStars);

        // --- Render Sun (orange center) ---
        glm::mat4 model = glm::mat4(1.0f);
        // Sun doesn't rotate in this simple model
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(colorLoc, 1.0f, 0.5f, 0.2f); // orange sun color

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)sphereIndices.size(), GL_UNSIGNED_INT, 0);

        // --- Render Earth (blue planet) ---
        model = glm::mat4(1.0f);

        // Earth orbit parameters
        float earthOrbitRadius = 4.0f;
        float earthOrbitSpeed = glm::radians(20.0f);
        float earthRotationSpeed = glm::radians(50.0f);
        float earthAxisTilt = glm::radians(23.5f);

        // Earth position in orbit
        float earthX = earthOrbitRadius * cos(earthOrbitSpeed * currentFrame);
        float earthZ = earthOrbitRadius * sin(earthOrbitSpeed * currentFrame);

        model = glm::translate(model, glm::vec3(earthX, 0.0f, earthZ));

        // Earth rotation with axial tilt
        model = glm::rotate(model, earthRotationSpeed * currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, earthAxisTilt, glm::vec3(0.0f, 0.0f, 1.0f));

        // Scale down Earth
        model = glm::scale(model, glm::vec3(0.5f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(colorLoc, 0.2f, 0.5f, 1.0f); // blue earth color

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)sphereIndices.size(), GL_UNSIGNED_INT, 0);

        // --- Render Venus (yellowish planet) ---
        model = glm::mat4(1.0f);

        // Venus orbit parameters (closer to sun, faster orbit)
        float venusOrbitRadius = 2.5f;
        float venusOrbitSpeed = glm::radians(35.0f);
        float venusRotationSpeed = glm::radians(30.0f); // Venus rotates slowly

        // Venus position in orbit (offset from Earth to avoid overlap)
        float venusX = venusOrbitRadius * cos(venusOrbitSpeed * currentFrame + glm::pi<float>());
        float venusZ = venusOrbitRadius * sin(venusOrbitSpeed * currentFrame + glm::pi<float>());

        model = glm::translate(model, glm::vec3(venusX, 0.0f, venusZ));

        // Venus rotation
        model = glm::rotate(model, venusRotationSpeed * currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));

        // Scale down Venus (slightly smaller than Earth)
        model = glm::scale(model, glm::vec3(0.4f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(colorLoc, 0.9f, 0.7f, 0.3f); // yellowish venus color

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)sphereIndices.size(), GL_UNSIGNED_INT, 0);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    glDeleteVertexArrays(1, &starVAO);
    glDeleteBuffers(1, &starVBO);

    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}