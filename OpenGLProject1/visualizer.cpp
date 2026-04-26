#pragma once
#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "glad.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <cmath>

#include "graph.h"         // Your Graph class
#include "fragment_shader.h" // Your Fragment Shader header
#include "vertex_shader.h"   // Your Vertex Shader header

// Constants for circle approximation
const int CIRCLE_SEGMENTS = 20;
const float PI = 3.14159265359f;

class Visualizer {
private:
    GLFWwindow* window;
    int screenWidth, screenHeight;
    float simWidth, simHeight; // Dimensions of the graph simulation (e.g., 1000x1000)

    // Camera State
    float zoomLevel;
    float panX, panY;

    // OpenGL IDs
    GLuint shaderProgram;
    GLuint VBO, VAO;

    // Data buffer for one frame
    // Format: x, y, z, r, g, b
    std::vector<float> vertexBufferData;

public:
    Visualizer(int width, int height, float graphW, float graphH)
        : screenWidth(width), screenHeight(height), simWidth(graphW), simHeight(graphH),
        zoomLevel(1.0f), panX(0.0f), panY(0.0f) {
        initWindow();
        initGL();
    }

    ~Visualizer() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    bool shouldClose() {
        return glfwWindowShouldClose(window);
    }

    void processInput() {
        glfwPollEvents();

        // Keyboard Pan Controls (Arrow Keys or WASD)
        float speed = 0.02f / zoomLevel;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    panY += speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  panY -= speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  panX -= speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) panX += speed;

        // Zoom Controls (Q/E or Z/X)
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) zoomLevel *= 1.02f;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) zoomLevel *= 0.98f;

        // Clamp Zoom
        if (zoomLevel < 0.1f) zoomLevel = 0.1f;
        if (zoomLevel > 10.0f) zoomLevel = 10.0f;
    }

    void render(const Graph& graph) {
        // Clear Screen
        glClearColor(0.9f, 0.9f, 0.95f, 1.0f); // Nice light background
        glClear(GL_COLOR_BUFFER_BIT);

        // Clear previous frame data
        vertexBufferData.clear();

        // --- 1. Draw Buildings (Squares) ---
        // Helper to normalize coordinates: 0..SimWidth -> -1..1
        auto addSquare = [&](float x, float y, float size, float r, float g, float b) {
            pushSquare(x, y, size, r, g, b);
            };

        // Draw Offices (Purple)
        /* Assuming Graph has getOfficeArray() or similar public access */
        /* Modify accessors below based on your specific Graph.h public members */
        for (const auto& b : graph.officeArray) addSquare(b.getLocation().first, b.getLocation().second, 25.0f, 0.6f, 0.2f, 0.8f);

        // Draw Schools (Cyan)
        for (const auto& b : graph.schoolArray) addSquare(b.getLocation().first, b.getLocation().second, 25.0f, 0.0f, 0.8f, 0.8f);

        // Draw Shopping Centers (Orange)
        for (const auto& b : graph.shoppingCenterArray) addSquare(b.getLocation().first, b.getLocation().second, 25.0f, 1.0f, 0.6f, 0.0f);

        // Draw Houses (Dark Gray, Smaller)
        for (const auto& h : graph.houseArray) {
            addSquare(h.getLocation().first, h.getLocation().second, 15.0f, 0.3f, 0.3f, 0.3f);

            // --- 2. Draw People (Residents) as Circles ---
            for (const auto& person : h.getResidents()) {
                float r, g, b;
                getColorFromEnum(person.getColor(), r, g, b);
                pushCircle(person.getLocation().first, person.getLocation().second, 3.0f, r, g, b);
            }
        }

        // Upload Data
        if (!vertexBufferData.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertexBufferData.size() * sizeof(float), vertexBufferData.data(), GL_DYNAMIC_DRAW);

            // Draw
            glUseProgram(shaderProgram);

            // Update Uniforms
            glUniform1f(glGetUniformLocation(shaderProgram, "uZoom"), zoomLevel);
            glUniform2f(glGetUniformLocation(shaderProgram, "uPan"), panX, panY);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, vertexBufferData.size() / 6);
        }

        glfwSwapBuffers(window);
    }

private:
    void initWindow() {
        if (!glfwInit()) exit(EXIT_FAILURE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(screenWidth, screenHeight, "Graph Simulation", NULL, NULL);
        if (!window) { glfwTerminate(); exit(EXIT_FAILURE); }
        glfwMakeContextCurrent(window);

        if (glewInit() != GLEW_OK) exit(EXIT_FAILURE);
        glViewport(0, 0, screenWidth, screenHeight);
    }

    void initGL() {
        // Compile Shaders
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // Buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Layout 0: Position (vec3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Layout 1: Color (vec3)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);
        // (Add error checking here in production code)
        return shader;
    }

    // Helper to map 0..WIDTH to -1..1
    float normalizeX(float x) { return (x / simWidth) * 2.0f - 1.0f; }
    float normalizeY(float y) { return (y / simHeight) * 2.0f - 1.0f; }
    float normalizeSizeX(float s) { return (s / simWidth) * 2.0f; }
    float normalizeSizeY(float s) { return (s / simHeight) * 2.0f; }

    void getColorFromEnum(COLOR c, float& r, float& g, float& b) {
        switch (c) {
        case BLACK:  r = 0.0f; g = 0.0f; b = 0.0f; break; // Dead
        case RED:    r = 1.0f; g = 0.0f; b = 0.0f; break; // Infected
        case GREEN:  r = 0.0f; g = 1.0f; b = 0.0f; break; // Healthy
        case BLUE:   r = 0.0f; g = 0.0f; b = 1.0f; break; // Recovered
        case YELLOW: r = 1.0f; g = 1.0f; b = 0.0f; break; // Incubating
        default:     r = 0.5f; g = 0.5f; b = 0.5f; break;
        }
    }

    // Geometry Helpers
    void pushVertex(float x, float y, float r, float g, float b) {
        vertexBufferData.push_back(normalizeX(x));
        vertexBufferData.push_back(normalizeY(y));
        vertexBufferData.push_back(0.0f); // z
        vertexBufferData.push_back(r);
        vertexBufferData.push_back(g);
        vertexBufferData.push_back(b);
    }

    void pushSquare(float cx, float cy, float size, float r, float g, float b) {
        float half = size / 2.0f;
        // Two triangles for a square
        // Tri 1
        pushVertex(cx - half, cy - half, r, g, b);
        pushVertex(cx + half, cy - half, r, g, b);
        pushVertex(cx + half, cy + half, r, g, b);
        // Tri 2
        pushVertex(cx + half, cy + half, r, g, b);
        pushVertex(cx - half, cy + half, r, g, b);
        pushVertex(cx - half, cy - half, r, g, b);
    }

    void pushCircle(float cx, float cy, float radius, float r, float g, float b) {
        for (int i = 0; i < CIRCLE_SEGMENTS; i++) {
            float theta1 = 2.0f * PI * float(i) / float(CIRCLE_SEGMENTS);
            float theta2 = 2.0f * PI * float(i + 1) / float(CIRCLE_SEGMENTS);

            // Triangle fan center method (approximated with individual triangles here for batching)
            pushVertex(cx, cy, r, g, b); // Center
            pushVertex(cx + radius * cos(theta1), cy + radius * sin(theta1), r, g, b);
            pushVertex(cx + radius * cos(theta2), cy + radius * sin(theta2), r, g, b);
        }
    }
};

#endif