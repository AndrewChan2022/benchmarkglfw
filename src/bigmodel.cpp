
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include "linmath.h"


// benchmark result
//      device                 ntriangle            fps
//    desktop GTX1660           100m                26
//    desktop GTX1660            66m                39
//    desktop GTX1660            47m                54
//    desktop GTX1660            30m                60
//    laptop RTX3080            100m                55
//    laptop intel              100m                11
//    -----------------------
//    summary
//    lowend nvidia gpu,  100m ~ 25fps   50m  50fps
//    highend nvidia gpu,  100m ~ 50fps


// Plane generation constants
// const int GRID_SIZE = 4000; // 30 million triangles -> fps=60, resolution not matter
// const int GRID_SIZE = 5000; // 47 million triangles -> fps=54, resolution not matter
// const int GRID_SIZE = 5150; // 50 million triangles -> fps=51, resolution not matter
// const int GRID_SIZE = 5920; // 66 million triangles -> fps=39, resolution not matter
const int GRID_SIZE = 7271; // 100 million triangles -> fps=26, resolution not matter


// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 rotationMatrix;
void main() {
    gl_Position = rotationMatrix * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.3, 0.6, 1.0, 1.0); // Light blue color
}
)";

// Function to create shader program
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    // Check vertex shader
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex Shader Compilation Failed: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    // Check fragment shader
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment Shader Compilation Failed: " << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check program
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader Program Linking Failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

template<typename V>
void fastVectorResize(V& v, size_t newSize) {
    struct vt { typename V::value_type v; vt() {}};
    static_assert(sizeof(vt[10]) == sizeof(typename V::value_type[10]), "alignment error");
    typedef std::vector<vt, typename std::allocator_traits<typename V::allocator_type>::template rebind_alloc<vt>> V2;
    reinterpret_cast<V2&>(v).resize(newSize);
}

// Function to generate grid vertices and indices with optimizations
void generatePlane(std::vector<float>& vertices, std::vector<unsigned int>& indices, int gridSize) {
    constexpr float margin = 0.1f; // Margin to keep the plane within visible NDC
    constexpr float startX = -1.0f + margin;
    constexpr float endX = 1.0f - margin;
    constexpr float startY = -1.0f + margin;
    constexpr float endY = 1.0f - margin;
    constexpr float zValue = 0.0f; // Keep Z constant

    // Calculate total sizes
    size_t numVertices = gridSize * gridSize * 3; // Each vertex has x, y, z
    size_t numIndices = (gridSize - 1) * (gridSize - 1) * 6; // Two triangles per quad

    printf("vertices: %llum indices: %llum triangles:%llum\n", numVertices / 1024 / 1024, numIndices / 1024 / 1024, numIndices / 1024 / 1024 / 3);
    printf("vertices bytes: %llum indices bytes: %llum\n", numVertices / 1024 / 1024 * 4, numIndices / 1024 / 1024 * 4);

    // Optimize resizing
    fastVectorResize(vertices, numVertices);
    fastVectorResize(indices, numIndices);

    // Generate vertices
    size_t vertexIndex = 0;
    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            float fx = static_cast<float>(x) / (gridSize - 1);
            float fy = static_cast<float>(y) / (gridSize - 1);

            // Interpolate x and y between start and end with margin
            float px = startX + fx * (endX - startX);
            float py = startY + fy * (endY - startY);

            vertices[vertexIndex++] = px;  // X
            vertices[vertexIndex++] = py;  // Y (varied)
            vertices[vertexIndex++] = zValue; // Z (constant)
        }
    }

    // Generate indices
    size_t indexIndex = 0;
    for (int y = 0; y < gridSize - 1; ++y) {
        for (int x = 0; x < gridSize - 1; ++x) {
            unsigned int topLeft = y * gridSize + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = topLeft + gridSize;
            unsigned int bottomRight = bottomLeft + 1;

            // First triangle
            indices[indexIndex++] = topLeft;
            indices[indexIndex++] = bottomLeft;
            indices[indexIndex++] = topRight;

            // Second triangle
            indices[indexIndex++] = topRight;
            indices[indexIndex++] = bottomLeft;
            indices[indexIndex++] = bottomRight;
        }
    }
}



static void saveToObj(const std::string& filename, std::vector<float>& vertices, std::vector<unsigned int>& indices) {

    size_t nnode = vertices.size() / 3;
    std::vector<float> normals;
    fastVectorResize(normals, nnode * 3);
    for (size_t i = 0; i < nnode; i++) {
        normals[i * 3 + 0] = 0;
        normals[i * 3 + 1] = 0;
        normals[i * 3 + 2] = 1.0;
    }

    std::ofstream outFile(filename);

    if (!outFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // Write vertices
    for (size_t i = 0; i < vertices.size(); i += 3) {
        outFile << "v " 
                << vertices[i] << " " 
                << vertices[i + 1] << " " 
                << vertices[i + 2] << "\n";
        
        if (i % (1024 * 1024) == 0) printf("write: %llum vertices\n", i / 1024 / 1024);
    }

    // Write normals
    // for (size_t i = 0; i < normals.size(); i += 3) {
    //     outFile << "vn " 
    //             << normals[i] << " " 
    //             << normals[i + 1] << " " 
    //             << normals[i + 2] << "\n";
    // }

    // Write faces (indices)
    for (size_t i = 0; i < indices.size(); i += 3) {
        // OBJ uses 1-based indexing
        // outFile << "f " 
        //         << indices[i] + 1 << "//" << indices[i] + 1 << " "
        //         << indices[i + 1] + 1 << "//" << indices[i + 1] + 1 << " "
        //         << indices[i + 2] + 1 << "//" << indices[i + 2] + 1 << "\n";
        outFile << "f " 
                << indices[i] + 1  << " "
                << indices[i + 1] + 1 << " "
                << indices[i + 2] + 1 << "\n";
        if (i % (1024 * 1024) == 0) printf("write: %llum indices\n", i / 1024 / 1024);
    }

    outFile.close();

    std::cout << "Successfully saved to " << filename << std::endl;
}

int bigmodel() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Plane with 100M Triangles", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Generate plane
    printf("generate plane begin\n");
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generatePlane(vertices, indices, GRID_SIZE);
    printf("generate plane done\n");

    // saveToObj("./plane100m.obj", vertices, indices);
    // printf("save obj file done\n");

    // Create VBO and VAO
    GLuint VBO, VAO, EBO;
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Create shader program
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    // Get location of uniform
    GLint rotationMatrixLocation = glGetUniformLocation(shaderProgram, "rotationMatrix");

    // Timing variables for FPS calculation
    auto lastTime = std::chrono::high_resolution_clock::now();
    int frameCount = 0;
    float rotationAngle = 0.0f;


    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Time management
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        if (deltaTime >= 1.0f) {
            std::string title = "FPS: " + std::to_string(frameCount);
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0;
            lastTime = currentTime;
        }

        rotationAngle += 90.0f * deltaTime; // Rotate at 90 degrees per second
        if (rotationAngle > 360.0f) rotationAngle -= 360.0f;

        // Calculate rotation matrix
        const float PI = 3.14159265359f;
        float radians = rotationAngle * (PI / 180.0f);
        //float radians = glm::radians(rotationAngle);
        float cosTheta = cos(radians);
        float sinTheta = sin(radians);
        float scale = 1;
        float rotationMatrix[16] = {
            cosTheta * scale, -sinTheta * scale, 0.0f, 0.0f,
            sinTheta * scale,  cosTheta * scale, 0.0f, 0.0f,
            0.0f,      0.0f,    1.0f, 0.0f,
            0.0f,      0.0f,    0.0f, 1.0f
        };


        // draw
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);    // Set uniform
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        frameCount++;
    }

    // Clean up
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
