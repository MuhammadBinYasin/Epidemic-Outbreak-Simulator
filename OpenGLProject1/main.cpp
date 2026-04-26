#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>
#include <string>
#include <sstream>
#include <atomic>
#include <fstream> 
#include <iomanip>

// Include your existing headers
#include "graph.h"
#include "vertexShader.h"
#include "fragmentShader.h"

// =============================================================
// GLOBAL SETTINGS
// =============================================================
const float SIM_WIDTH = 1920.0f;
const float SIM_HEIGHT = 1080.0f;

float screenWidth = 1920.0f;
float screenHeight = 1080.0f;

// Camera
float zoomLevel = 1.0f;
float panX = 0.0f;
float panY = 0.0f;

// Mouse
bool isDragging = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

// Control Flow
bool isPaused = false;
std::atomic_bool requestSingleStep(false);
bool simulationEnded = false;

// Lockdown
int lockdownLevel = 0;

// Bitflags
const uint8_t FLAG_SCHOOL = 0x01;
const uint8_t FLAG_OFFICE = 0x02;
const uint8_t FLAG_SHOP_M = 0x04;
const uint8_t FLAG_SHOP_E = 0x08;

std::vector<uint8_t> dailySchedule;
std::ofstream resultsFile;

using steadyClock = std::chrono::steady_clock;
steadyClock::time_point lastStepTime;

// Time Phases
enum TimePhase { PHASE_MORNING, PHASE_EVENING, PHASE_NIGHT };
TimePhase currentPhase = PHASE_MORNING;
int dayCount = 0;

// =============================================================
// OPTIMIZED SHAPE CACHE (CPU -> GPU Speedup)
// =============================================================
// Instead of calculating sin/cos every frame for 1000s of nodes,
// we pre-calculate the local vertices once.
struct ShapeCache {
    std::vector<float> square;
    std::vector<float> triangle;
    std::vector<float> pentagon;
    std::vector<float> hexagon;
    std::vector<float> circle; // 10 sides

    void init() {
        generatePolygon(square, 4, 0.785398f); // 45 degrees offset for square
        generatePolygon(triangle, 3, -1.5708f); // Point up
        generatePolygon(pentagon, 5, -1.5708f);
        generatePolygon(hexagon, 6, -1.5708f);
        generatePolygon(circle, 10, 0.0f);
    }

    void generatePolygon(std::vector<float>& buffer, int sides, float angleOffset) {
        const float PI = 3.14159265f;
        for (int i = 0; i < sides; i++) {
            float theta1 = angleOffset + 2.0f * PI * float(i) / float(sides);
            float theta2 = angleOffset + 2.0f * PI * float(i + 1) / float(sides);

            // Center
            buffer.push_back(0.0f); buffer.push_back(0.0f);
            // Point 1
            buffer.push_back(cos(theta1)); buffer.push_back(sin(theta1));
            // Point 2
            buffer.push_back(cos(theta2)); buffer.push_back(sin(theta2));
        }
    }
} shapes;

// =============================================================
// HELPER FUNCTIONS
// =============================================================
float normX(float x) { return (x / SIM_WIDTH) * 2.0f - 1.0f; }
float normY(float y) { return (y / SIM_HEIGHT) * 2.0f - 1.0f; }

// Optimized Drawer: Uses pre-calculated shape data
void drawShapeFast(std::vector<float>& buffer, const std::vector<float>& shapeTemplate,
    float cx, float cy, float radius, float r, float g, float b) {

    // Normalize center once
    float ncx = normX(cx);
    float ncy = normY(cy);

    // Scale factors for radius to normalized coords
    float sx = (radius / SIM_WIDTH) * 2.0f;
    float sy = (radius / SIM_HEIGHT) * 2.0f;

    // The shapeTemplate contains [x, y, x, y...] relative to 0,0 with radius 1
    size_t count = shapeTemplate.size();
    for (size_t i = 0; i < count; i += 2) {
        float lx = shapeTemplate[i];
        float ly = shapeTemplate[i + 1];

        // Transform: Scale then Translate
        buffer.push_back(ncx + lx * sx);
        buffer.push_back(ncy + ly * sy);
        buffer.push_back(0.0f); // Z
        buffer.push_back(r);
        buffer.push_back(g);
        buffer.push_back(b);
    }
}

void drawLine(std::vector<float>& buffer, float x1, float y1, float x2, float y2, float r, float g, float b) {
    buffer.push_back(normX(x1)); buffer.push_back(normY(y1)); buffer.push_back(0.0f);
    buffer.push_back(r); buffer.push_back(g); buffer.push_back(b);

    buffer.push_back(normX(x2)); buffer.push_back(normY(y2)); buffer.push_back(0.0f);
    buffer.push_back(r); buffer.push_back(g); buffer.push_back(b);
}

void getRGB(COLOR c, float& r, float& g, float& b) {
    switch (c) {
    case BLACK:  r = 0.1f; g = 0.1f; b = 0.1f; break;
    case RED:    r = 0.9f; g = 0.1f; b = 0.1f; break;
    case GREEN:  r = 0.2f; g = 0.8f; b = 0.2f; break;
    case BLUE:   r = 0.2f; g = 0.2f; b = 0.9f; break;
    case YELLOW: r = 0.9f; g = 0.9f; b = 0.1f; break;
    default:     r = 0.5f; g = 0.5f; b = 0.5f; break;
    }
}

double calculateDailyProb(double totalProb, int days) {
    if (days <= 0) return totalProb;
    double survivalTotal = 1.0 - totalProb;
    return 1.0 - std::pow(survivalTotal, 1.0 / (double)days);
}

// =============================================================
// SIMULATION LOGIC
// =============================================================

void generateDailySchedule(Graph& graph) {
    auto& people = graph.getNodes();
    if (dailySchedule.size() != people.size()) dailySchedule.resize(people.size());

    double pSchool = 0.95, pOffice = 0.90, pShopM = 0.15, pShopE = 0.40;

    if (lockdownLevel >= 1) pSchool = 0.0;
    if (lockdownLevel >= 2) pOffice = 0.0;
    if (lockdownLevel >= 3) { pShopM = 0.02; pShopE = 0.05; }

    static std::mt19937 gen(std::random_device{}());
    std::bernoulli_distribution distSchool(pSchool);
    std::bernoulli_distribution distOffice(pOffice);
    std::bernoulli_distribution distShopM(pShopM);
    std::bernoulli_distribution distShopE(pShopE);

    for (size_t i = 0; i < people.size(); ++i) {
        uint8_t flags = 0;
        POPULATION age = people[i].getPopulation();
        bool busyMorning = false;

        if ((age == CHILD || age == TEENAGERS)) {
            if (distSchool(gen)) { flags |= FLAG_SCHOOL; busyMorning = true; }
        }
        else if (age == ADULT) {
            if (distOffice(gen)) { flags |= FLAG_OFFICE; busyMorning = true; }
        }

        if (!busyMorning && age != CHILD && distShopM(gen)) {
            flags |= FLAG_SHOP_M;
        }

        if (age != CHILD && distShopE(gen)) {
            flags |= FLAG_SHOP_E;
        }
        dailySchedule[i] = flags;
    }
}

void simulatePhaseInfection(Graph& graph, std::vector<int>& infected) {
    auto& schools = graph.schoolArray;
    auto& offices = graph.officeArray;
    auto& houses = graph.houseArray;
    auto& shoppingCenters = graph.shoppingCenterArray;
    auto& people = graph.getNodes();

    const double infectProb = graph.getinfectProbability();
    const int incubationTime = graph.getDisease().incubationTime;

    static std::mt19937 gen(std::random_device{}());
    std::bernoulli_distribution infectDist(infectProb);
    std::bernoulli_distribution vaccineDist(graph.getVaccineEfficacy());
    std::bernoulli_distribution protectiveDist(graph.getDisease().precautinoryMeasureEffectiveness);

    size_t currentCount = infected.size();

    // Helper lambda for infection check
    auto tryInfect = [&](int targetID) {
        Node& target = people[targetID];
        if (!target.getInfected() && !target.getinIncubation() && !target.isRecovered() && !target.isDead()) {
            if (infectDist(gen)) {
                bool prot = (target.getVaccinated() && vaccineDist(gen)) || (target.takesProtectiveMeasure() && protectiveDist(gen));
                if (!prot) {
                    target.setInfected(incubationTime);
                    infected.push_back(targetID);
                }
            }
        }
        };

    for (size_t i = 0; i < currentCount; ++i) {
        int id = infected[i];
        if (id < 0 || id >= people.size()) continue;
        Node& carrier = people[id];
        uint8_t sched = dailySchedule[id];

        if (currentPhase == PHASE_MORNING) {
            int sID = carrier.getSchoolID();
            if (sID != -1 && (sched & FLAG_SCHOOL)) {
                for (int tID : schools[sID].getStudents()) {
                    if (tID != id && (dailySchedule[tID] & FLAG_SCHOOL)) tryInfect(tID);
                }
            }
            int oID = carrier.getOfficeID();
            if (oID != -1 && (sched & FLAG_OFFICE)) {
                for (int tID : offices[oID].getEmployees()) {
                    if (tID != id && (dailySchedule[tID] & FLAG_OFFICE)) tryInfect(tID);
                }
            }
            int shID = carrier.getShoppingCenterID();
            if (shID != -1 && (sched & FLAG_SHOP_M)) {
                for (int tID : shoppingCenters[shID].getCustomers()) {
                    if (tID != id && (dailySchedule[tID] & FLAG_SHOP_M)) tryInfect(tID);
                }
            }
        }
        else if (currentPhase == PHASE_EVENING) {
            int shID = carrier.getShoppingCenterID();
            if (shID != -1 && (sched & FLAG_SHOP_E)) {
                for (int tID : shoppingCenters[shID].getCustomers()) {
                    if (tID != id && (dailySchedule[tID] & FLAG_SHOP_E)) tryInfect(tID);
                }
            }
        }
        else if (currentPhase == PHASE_NIGHT) {
            int hID = carrier.getHouseID();
            if (hID != -1) {
                for (int tID : houses[hID].getResidents()) {
                    if (tID != id) tryInfect(tID);
                }
            }
        }
    }
}

void endOfDayUpdate(Graph& graph, std::vector<int>& infected) {
    auto& people = graph.getNodes();
    int meanDays = std::max(1, (graph.getDisease().maxTime + graph.getDisease().minTime) / 2);

    double dChild = calculateDailyProb(graph.getDisease().deathProbabilityChild, meanDays);
    double dTeen = calculateDailyProb(graph.getDisease().deathProbabilityTeenager, meanDays);
    double dAdult = calculateDailyProb(graph.getDisease().deathProbabilityAdult, meanDays);
    double dElder = calculateDailyProb(graph.getDisease().deathProbailityElderly, meanDays);

    static std::mt19937 gen(std::random_device{}());
    std::bernoulli_distribution distChild(dChild);
    std::bernoulli_distribution distTeen(dTeen);
    std::bernoulli_distribution distAdult(dAdult);
    std::bernoulli_distribution distElder(dElder);
    std::exponential_distribution<double> distRecover(1.0 / (double)meanDays);

    std::vector<int> toRemove;

    for (int id : infected) {
        Node& p = people[id];
        bool died = false;
        if (p.getInfected()) {
            switch (p.getPopulation()) {
            case CHILD:     died = distChild(gen); break;
            case TEENAGERS: died = distTeen(gen); break;
            case ADULT:     died = distAdult(gen); break;
            case ELDERLY:   died = distElder(gen); break;
            }
        }

        if (died) {
            p.setDead();
            toRemove.push_back(id);
        }
        else {
            p.updateTime(graph.getDisease().minTime, graph.getDisease().maxTime, distRecover, gen);
            if (p.isRecovered()) toRemove.push_back(id);
        }
    }

    if (!toRemove.empty()) {
        std::sort(toRemove.begin(), toRemove.end());
        toRemove.erase(std::unique(toRemove.begin(), toRemove.end()), toRemove.end());
        infected.erase(std::remove_if(infected.begin(), infected.end(),
            [&](int id) { return std::binary_search(toRemove.begin(), toRemove.end(), id); }), infected.end());
    }

    int nHealthy = 0, nInfected = 0, nIncubation = 0, nRecovered = 0, nDead = 0;
    for (const auto& n : people) {
        if (n.isDead()) nDead++;
        else if (n.isRecovered()) nRecovered++;
        else if (n.getinIncubation()) nIncubation++;
        else if (n.getInfected()) nInfected++;
        else nHealthy++;
    }

    if (resultsFile.is_open()) {
        resultsFile << nHealthy << " " << nInfected << " "
            << nIncubation << " " << nRecovered << " " << nDead << "\n";
    }

    if (nInfected == 0 && nIncubation == 0) {
        simulationEnded = true;
        resultsFile.close();
        std::cout << "Epidemic Ended. Results saved.\n";
    }
}

// =============================================================
// INPUT CALLBACKS
// =============================================================
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        }
        else if (action == GLFW_RELEASE) {
            isDragging = false;
        }
    }
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    if (isDragging) {
        double dx = xpos - lastMouseX;
        double dy = ypos - lastMouseY;
        float pixelToNDC_X = 2.0f / screenWidth;
        float pixelToNDC_Y = 2.0f / screenHeight;
        panX -= (float)dx * pixelToNDC_X / zoomLevel;
        panY += (float)dy * pixelToNDC_Y / zoomLevel;
        lastMouseX = xpos;
        lastMouseY = ypos;
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    float sensitivity = 0.1f;
    if (yoffset > 0) zoomLevel *= (1.0f + sensitivity);
    else             zoomLevel /= (1.0f + sensitivity);
    if (zoomLevel < 0.1f) zoomLevel = 0.1f;
    if (zoomLevel > 20.0f) zoomLevel = 20.0f;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    screenWidth = (float)width;
    screenHeight = (float)height;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_SPACE) {
            isPaused = !isPaused;
            std::cout << (isPaused ? "[PAUSED]\n" : "[UNPAUSED]\n");
        }
        else if (key == GLFW_KEY_R) {
            requestSingleStep.store(true);
        }
        else if (key == GLFW_KEY_0) { lockdownLevel = 0; std::cout << "Lvl 0\n"; }
        else if (key == GLFW_KEY_1) { lockdownLevel = 1; std::cout << "Lvl 1\n"; }
        else if (key == GLFW_KEY_2) { lockdownLevel = 2; std::cout << "Lvl 2\n"; }
        else if (key == GLFW_KEY_3) { lockdownLevel = 3; std::cout << "Lvl 3\n"; }
    }
}

// =============================================================
// MAIN
// =============================================================
int main() {
    std::cout << "Initializing Graph..." << std::endl;
    shapes.init(); // Pre-calculate shapes
    Graph myGraph;

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow((int)screenWidth, (int)screenHeight, "Graph Visualizer", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    auto& nodes = myGraph.getNodes();
    std::vector<int> infected = myGraph.infectInitial(20);

    resultsFile.open("C:\\Users\\ambre\\source\\repos\\OpenGLProject1\\x64\\Debug\\results.txt", std::ios::trunc);
    generateDailySchedule(myGraph);

    lastStepTime = steadyClock::now();
    std::vector<float> linesBuffer;
    std::vector<float> shapesBuffer;

    // Pre-allocate to reduce reallocation
    linesBuffer.reserve(nodes.size() * 12);
    shapesBuffer.reserve(nodes.size() * 60);

    std::cout << "Controls:\nSpace: Pause\nR: Step\n1-3: Lockdown\n0: Reset\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (!simulationEnded) {
            auto now = steadyClock::now();
            std::chrono::duration<double> diff = now - lastStepTime;

            // Update every 1.0s unless paused
            bool advance = (!isPaused && diff.count() >= 1.0) || requestSingleStep.load();

            if (advance) {
                simulatePhaseInfection(myGraph, infected);

                if (currentPhase == PHASE_MORNING) currentPhase = PHASE_EVENING;
                else if (currentPhase == PHASE_EVENING) currentPhase = PHASE_NIGHT;
                else if (currentPhase == PHASE_NIGHT) {
                    endOfDayUpdate(myGraph, infected);
                    dayCount++;
                    generateDailySchedule(myGraph);
                    currentPhase = PHASE_MORNING;
                }

                requestSingleStep.store(false);
                lastStepTime = steadyClock::now();

                std::string phaseStr = (currentPhase == PHASE_MORNING) ? "Morning" :
                    (currentPhase == PHASE_EVENING) ? "Evening" : "Night";
                std::ostringstream ss;
                ss << "Day: " << dayCount << " [" << phaseStr << "] | Inf: " << infected.size()
                    << (isPaused ? " [PAUSED]" : "");
                glfwSetWindowTitle(window, ss.str().c_str());
            }
        }

        glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glUniform1f(glGetUniformLocation(prog, "uZoom"), zoomLevel);
        glUniform2f(glGetUniformLocation(prog, "uPan"), panX, panY);

        linesBuffer.clear();
        shapesBuffer.clear();

        // 1. Draw Lines (CPU check, but batch push)
        for (auto& house : myGraph.houseArray) {
            float hx = house.getLocation().first;
            float hy = house.getLocation().second;

            for (int pID : house.getResidents()) {
                if (pID < 0 || pID >= (int)nodes.size()) continue;
                float px = nodes[pID].getLocation().first;
                float py = nodes[pID].getLocation().second;
                uint8_t sched = dailySchedule[pID];

                if (currentPhase == PHASE_NIGHT || currentPhase == PHASE_MORNING) {
                    drawLine(linesBuffer, px, py, hx, hy, 0.2f, 0.2f, 0.2f);
                }

                if (currentPhase == PHASE_MORNING) {
                    int sID = nodes[pID].getSchoolID();
                    if (sID != -1 && (sched & FLAG_SCHOOL)) {
                        drawLine(linesBuffer, px, py, myGraph.schoolArray[sID].getLocation().first,
                            myGraph.schoolArray[sID].getLocation().second, 0.0f, 0.4f, 0.4f);
                    }
                    int oID = nodes[pID].getOfficeID();
                    if (oID != -1 && (sched & FLAG_OFFICE)) {
                        drawLine(linesBuffer, px, py, myGraph.officeArray[oID].getLocation().first,
                            myGraph.officeArray[oID].getLocation().second, 0.4f, 0.0f, 0.4f);
                    }
                    int shID = nodes[pID].getShoppingCenterID();
                    if (shID != -1 && (sched & FLAG_SHOP_M)) {
                        drawLine(linesBuffer, px, py, myGraph.shoppingCenterArray[shID].getLocation().first,
                            myGraph.shoppingCenterArray[shID].getLocation().second, 0.5f, 0.3f, 0.0f);
                    }
                }
                else if (currentPhase == PHASE_EVENING) {
                    int shID = nodes[pID].getShoppingCenterID();
                    if (shID != -1 && (sched & FLAG_SHOP_E)) {
                        drawLine(linesBuffer, px, py, myGraph.shoppingCenterArray[shID].getLocation().first,
                            myGraph.shoppingCenterArray[shID].getLocation().second, 0.5f, 0.3f, 0.0f);
                    }
                }
            }
        }

        // Upload Lines
        if (!linesBuffer.empty()) {
            // GL_STREAM_DRAW is crucial for per-frame updates
            glBufferData(GL_ARRAY_BUFFER, linesBuffer.size() * sizeof(float), linesBuffer.data(), GL_STREAM_DRAW);
            glDrawArrays(GL_LINES, 0, (GLsizei)linesBuffer.size() / 6);
        }

        // 2. Draw Shapes (Using Fast Cache)
        for (auto& b : myGraph.shoppingCenterArray)
            drawShapeFast(shapesBuffer, shapes.hexagon, b.getLocation().first, b.getLocation().second, 30.0f, 1.0f, 0.6f, 0.0f);
        for (auto& b : myGraph.schoolArray)
            drawShapeFast(shapesBuffer, shapes.pentagon, b.getLocation().first, b.getLocation().second, 22.0f, 0.0f, 0.8f, 0.9f);
        for (auto& b : myGraph.officeArray)
            drawShapeFast(shapesBuffer, shapes.triangle, b.getLocation().first, b.getLocation().second, 15.0f, 0.7f, 0.2f, 0.8f);

        for (auto& h : myGraph.houseArray) {
            drawShapeFast(shapesBuffer, shapes.square, h.getLocation().first, h.getLocation().second, 6.0f, 0.5f, 0.5f, 0.5f);
            for (int p : h.getResidents()) {
                float r, g, b; getRGB(nodes[p].getColor(), r, g, b);
                drawShapeFast(shapesBuffer, shapes.circle, nodes[p].getLocation().first, nodes[p].getLocation().second, 4.0f, r, g, b);
            }
        }

        // Upload Shapes
        if (!shapesBuffer.empty()) {
            glBufferData(GL_ARRAY_BUFFER, shapesBuffer.size() * sizeof(float), shapesBuffer.data(), GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)shapesBuffer.size() / 6);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(prog);
    glfwTerminate();
    return 0;
}