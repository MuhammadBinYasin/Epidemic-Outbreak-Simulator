#include "graph.h"
// =========================================================
// QuadTree Helper Class (Local to Graph.cpp)
// =========================================================
struct QuadNode {
    float x, y;       // Center of mass
    float mass;       // Total mass (count)
    float minX, minY, maxX, maxY; // Bounding box
    QuadNode* children[4]; // NW, NE, SW, SE
    bool isLeaf;
    int bodyIndex;    // Index of the body (if leaf and mass==1)

    QuadNode(float minX, float minY, float maxX, float maxY)
        : x(0), y(0), mass(0), minX(minX), minY(minY), maxX(maxX), maxY(maxY),
        isLeaf(true), bodyIndex(-1) {
        for (int i = 0; i < 4; i++) children[i] = nullptr;
    }

    ~QuadNode() {
        for (int i = 0; i < 4; i++) if (children[i]) delete children[i];
    }
};

class QuadTree {
    QuadNode* root;
    const float THETA = 0.7f; // Approximation threshold
    const int MAX_DEPTH = 30; // Preventing Stack Overflow

public:
    QuadTree(float minX, float minY, float maxX, float maxY) {
        // Pad bounds slightly to ensure points on edge don't cause issues
        float pad = 10.0f;
        root = new QuadNode(minX - pad, minY - pad, maxX + pad, maxY + pad);
    }

    ~QuadTree() { delete root; }

    void insert(int index, float bx, float by) {
        insertRec(root, index, bx, by, 0);
    }

    // Returns force components fx, fy
    void calculateForce(float bx, float by, float k, float& fx, float& fy) {
        calcForceRec(root, bx, by, k, fx, fy);
    }

private:
    void insertRec(QuadNode* node, int index, float bx, float by, int depth) {
        // 1. Update Center of Mass
        float totalMass = node->mass + 1.0f;
        // Weighted average for center of mass
        node->x = (node->x * node->mass + bx) / totalMass;
        node->y = (node->y * node->mass + by) / totalMass;
        node->mass = totalMass;

        // 2. Stop if we reached max depth (Prevents Stack Overflow on overlapping nodes)
        if (depth > MAX_DEPTH) {
            return;
        }

        // 3. Leaf processing
        if (node->isLeaf) {
            // Case A: Empty leaf -> Just store body
            if (node->mass <= 1.001f) {
                node->bodyIndex = index;
                return;
            }

            // Case B: Leaf occupied -> Split
            // We need to push the existing body down, then the new body
            int oldIndex = node->bodyIndex;
            node->bodyIndex = -1; // No longer holds a single body directly
            node->isLeaf = false;

            float midX = (node->minX + node->maxX) * 0.5f;
            float midY = (node->minY + node->maxY) * 0.5f;

            node->children[0] = new QuadNode(node->minX, node->minY, midX, midY); // NW
            node->children[1] = new QuadNode(midX, node->minY, node->maxX, midY); // NE
            node->children[2] = new QuadNode(node->minX, midY, midX, node->maxY); // SW
            node->children[3] = new QuadNode(midX, midY, node->maxX, node->maxY); // SE

            // Re-insert old body (we use current COM as approx position if we don't have access to array)
            // Ideally, we'd look up oldIndex pos, but since we are inside a logic block where we don't 
            // readily have the array, we rely on the fact that for a leaf with mass=1, x/y IS the position.
            insertRecToChild(node, oldIndex, node->x, node->y, depth + 1);

            // Note: node->x/node->y changed at start of function, but for the *previous* single body, 
            // its position was the old COM. Wait, the math at step 1 updated COM. 
            // To be perfectly accurate we would need the original pos. 
            // However, simply pushing the new body is sufficient for layout stability.
        }

        // 4. Insert new body into appropriate child
        if (!node->isLeaf) {
            insertRecToChild(node, index, bx, by, depth + 1);
        }
    }

    void insertRecToChild(QuadNode* node, int index, float bx, float by, int depth) {
        float midX = (node->minX + node->maxX) * 0.5f;
        float midY = (node->minY + node->maxY) * 0.5f;

        int quad = 0;
        if (bx >= midX) quad += 1; // East
        if (by >= midY) quad += 2; // South

        insertRec(node->children[quad], index, bx, by, depth);
    }

    void calcForceRec(QuadNode* node, float bx, float by, float k, float& fx, float& fy) {
        // Ignore empty nodes
        if (!node || node->mass < 0.1f) return;

        float dx = node->x - bx;
        float dy = node->y - by;
        float distSq = dx * dx + dy * dy + 0.01f; // Softener to prevent div/0
        float dist = std::sqrt(distSq);
        float width = node->maxX - node->minX;

        // Barnes-Hut criterion: If node is far enough away, treat as single body
        bool isFar = (width / dist) < THETA;

        // Also treat leaf nodes as single bodies (obviously)
        if (node->isLeaf || isFar) {
            if (dist > 1.0f) { // Prevent self-force or extreme close range jitter
                // F = k^2 * mass / dist^2
                // We assume unit mass for bodies, so node->mass is just the count
                float force = (k * k * node->mass) / distSq;
                fx -= (dx / dist) * force;
                fy -= (dy / dist) * force;
            }
        }
        else {
            // Otherwise, recurse into children
            for (int i = 0; i < 4; i++) {
                if (node->children[i]) calcForceRec(node->children[i], bx, by, k, fx, fy);
            }
        }
    }
};

    void Graph::dynamicGraphGenerator() {
        int tempID = 0;
        //number schools
        int numberOfChildren = this->numberOfPeople * (this->populationStatistics.percentageOfChildren + this->populationStatistics.percentageOfTeenagers);
        const int avgNumberOfChildPerSchool = 100;
        int numberOfSchools = std::max(numberOfChildren / avgNumberOfChildPerSchool,1);
        //number of houses
        int numberOfHouses = std::max(this->numberOfPeople / 4,1);
        //number of Offices
        int numberOfWorkingClassPeople = this->numberOfPeople * this->populationStatistics.percentageOfAdults;
        const int avgNumberOfEmployeesPerOffice = 35;
        int numberOfOffices = std::max(numberOfWorkingClassPeople / avgNumberOfEmployeesPerOffice,1);
        //number of grocery stores
        int avgNumberOfPeopleVisitingShoppingCenters = 200;
        int numberOfShoppingCenters = std::max(this->numberOfPeople / avgNumberOfPeopleVisitingShoppingCenters,1);

        //creating a random number generator
        std::mt19937 gen(device());
        std::exponential_distribution<double> houseDistribution(0.25);
        std::uniform_real_distribution<double> popultaionChance(0.0, 1.0);
        std::bernoulli_distribution protectiveMeasureDistribution(protectiveMeasureChance);
        std::bernoulli_distribution vaccinatedChance(vaccineCoverage);
        //create offices
        for (int i = 0; i < numberOfOffices; ++i) {
            std::pair<float, float> location = { 0,0 };
            officeArray.emplace_back(i, location);
        }
        //create schools
        for (int i = 0; i < numberOfSchools; ++i) {
            std::pair<float, float> location = { 0,0 };
            schoolArray.emplace_back(i, location);
        }
        //create shopping centers
        for (int i = 0; i < numberOfShoppingCenters; ++i) {
            std::pair<float, float> location = { 0,0 };
            shoppingCenterArray.emplace_back(i, location);
        }
        //create houses and add people to it
        for (int i = 0; i < numberOfHouses; ++i) {
            int numberOfPeople = std::min((int)(std::round(houseDistribution(gen)) + 1),10);
            std::vector<int> residents;
            for (int j = 0; j < numberOfPeople; ++j) {
                int officeID = -1;
                int shoppingCenterID = -1;
                int schoolID = -1;
                std::pair<double, double> temp = { 0,0 };
                POPULATION current;
                double tempProbPopulationCategory = popultaionChance(gen);
                if (tempProbPopulationCategory < populationStatistics.percentageOfChildren) {
                    schoolID = rand() % numberOfSchools;
                    current = CHILD;
                }
                else if (tempProbPopulationCategory < populationStatistics.percentageOfChildren + populationStatistics.percentageOfTeenagers) {
                    shoppingCenterID = rand() % numberOfShoppingCenters;
                    schoolID = rand() % numberOfSchools;
                    current = TEENAGERS;
                }
                else if (tempProbPopulationCategory <
                    populationStatistics.percentageOfChildren + populationStatistics.percentageOfTeenagers + populationStatistics.percentageOfAdults) {
                    current = ADULT;
                    shoppingCenterID = rand() % numberOfShoppingCenters;
                    officeID = rand() % numberOfOffices;
                }
                else {
                    shoppingCenterID = rand() % numberOfShoppingCenters;
                    current = ELDERLY;
                }
                bool protectiveMeasure = protectiveMeasureDistribution(gen) ? true : false;
                bool vaccinated = vaccinatedChance(gen) ? true : false;
                residents.push_back(tempID);
                graph.emplace_back(tempID, temp, current, vaccinated, protectiveMeasure, i, officeID, shoppingCenterID, schoolID);
                //add to school 
                if (schoolID != -1) {
                    schoolArray[schoolID].addStudent(tempID);
                }
                //add to shopping center 
                if (shoppingCenterID != -1) {
                    shoppingCenterArray[shoppingCenterID].addCustomer(tempID);
                }
                //add to office
                if (officeID != -1) {
                    officeArray[officeID].addEmployee(tempID);
                }
                ++tempID;
            }
            //add house to array
            std::pair<float, float> locationHouse = { 0,0 };
            houseArray.emplace_back(i, residents, locationHouse);
        }
    }
    void Graph::springLayout(float width, float height, int iterations) {
        if (houseArray.empty()) return;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> uni01(0.0, 1.0);
        std::normal_distribution<double> smallNoise(0.0, 1.0);

        const double PI = 3.141592653589793;
        const double cx = width * 0.5;
        const double cy = height * 0.5;

        // --- 1. Central Buildings Setup ---
        struct PointRef { float* x; float* y; };
        std::vector<PointRef> centralBuildings;

        auto addToRefs = [&](auto& arr) {
            for (auto& b : arr) {
                centralBuildings.push_back({ &b.getLocation().first, &b.getLocation().second });
            }
            };
        addToRefs(officeArray);
        addToRefs(schoolArray);
        addToRefs(shoppingCenterArray);

        // --- 2. Initial Placement (INVERTED) ---

        // A. Houses go in the CENTER (Cluster)
        const int nH = static_cast<int>(houseArray.size());
        double houseClusterRadius = std::min(width, height) * 0.25;

        for (int i = 0; i < nH; ++i) {
            double angle = (2.0 * PI * i) / double(nH);
            // Distribute in a filled circle
            double r = std::sqrt(uni01(gen)) * houseClusterRadius;
            houseArray[i].updateLocationX(static_cast<float>(cx + r * cos(angle)));
            houseArray[i].updateLocationY(static_cast<float>(cy + r * sin(angle)));
        }

        // B. Buildings go in the RING (Annulus)
        double ringTarget = std::min(width, height) * 0.45; // The target radius for buildings

        for (size_t i = 0; i < centralBuildings.size(); ++i) {
            double angle = (2.0 * PI * i) / double(centralBuildings.size());
            // Jitter around the ring
            double r = ringTarget + (uni01(gen) - 0.5) * 50.0;
            *centralBuildings[i].x = static_cast<float>(cx + r * cos(angle));
            *centralBuildings[i].y = static_cast<float>(cy + r * sin(angle));
        }

        // --- 3. Physics Loop ---
        double k_repel = 100.0;      // Standard repulsion
        double k_house_grav = 0.05;  // Pull houses to center
        double k_building_ring = 0.15; // Pull buildings to the ring

        double temp = width * 0.15;
        double dt = temp / (double)iterations;

        std::vector<std::pair<float, float>> dispHouses(nH, { 0.0f, 0.0f });
        std::vector<std::pair<float, float>> dispCentral(centralBuildings.size(), { 0.0f, 0.0f });

        for (int it = 0; it < iterations; ++it) {
            std::fill(dispHouses.begin(), dispHouses.end(), std::pair<float, float>{0.0f, 0.0f});
            std::fill(dispCentral.begin(), dispCentral.end(), std::pair<float, float>{0.0f, 0.0f});

            // A. QuadTree Bounds
            float minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
            auto updateBounds = [&](float x, float y) {
                if (x < minX) minX = x; if (x > maxX) maxX = x;
                if (y < minY) minY = y; if (y > maxY) maxY = y;
                };
            for (auto& b : centralBuildings) updateBounds(*b.x, *b.y);
            for (auto& h : houseArray) updateBounds(h.getLocation().first, h.getLocation().second);

            // B. Build QuadTree
            QuadTree tree(minX, minY, maxX, maxY);
            for (size_t i = 0; i < centralBuildings.size(); i++) {
                tree.insert((int)i, *centralBuildings[i].x, *centralBuildings[i].y);
            }
            int offset = (int)centralBuildings.size();
            for (int i = 0; i < nH; i++) {
                tree.insert(offset + i, houseArray[i].getLocation().first, houseArray[i].getLocation().second);
            }

            // C. Repulsion (Keep things apart)
            // Buildings push harder
            for (size_t i = 0; i < centralBuildings.size(); i++) {
                float fx = 0, fy = 0;
                tree.calculateForce(*centralBuildings[i].x, *centralBuildings[i].y, (float)k_repel * 3.0f, fx, fy);
                dispCentral[i].first += fx;
                dispCentral[i].second += fy;
            }
            // Houses
            for (int i = 0; i < nH; i++) {
                float fx = 0, fy = 0;
                tree.calculateForce(houseArray[i].getLocation().first, houseArray[i].getLocation().second, (float)k_repel, fx, fy);
                dispHouses[i].first += fx;
                dispHouses[i].second += fy;
            }

            // D. TOPOLOGY ENFORCEMENT (Inverted)

            // 1. Houses: Gravity to Center (The Core)
            for (int i = 0; i < nH; i++) {
                float dx = cx - houseArray[i].getLocation().first;
                float dy = cy - houseArray[i].getLocation().second;
                // Linear pull
                dispHouses[i].first += dx * k_house_grav;
                dispHouses[i].second += dy * k_house_grav;
            }

            // 2. Central Buildings: Ring Logic
            // They should float in a ring around the houses
            for (size_t i = 0; i < centralBuildings.size(); i++) {
                float bx = *centralBuildings[i].x;
                float by = *centralBuildings[i].y;
                float dx = bx - cx;
                float dy = by - cy;
                float dist = std::sqrt(dx * dx + dy * dy) + 0.001f;

                // Force = (TargetRadius - CurrentRadius)
                // If dist < RingTarget: Push Out (Positive)
                // If dist > RingTarget: Pull In (Negative)
                float forceMag = (ringTarget - dist) * k_building_ring;

                float dirX = dx / dist; // Direction away from center
                float dirY = dy / dist;

                dispCentral[i].first += dirX * forceMag;
                dispCentral[i].second += dirY * forceMag;
            }

            // E. Apply and Update
            auto applyMove = [&](float& x, float& y, float dx, float dy) {
                float dist = std::sqrt(dx * dx + dy * dy);
                float limit = (float)std::min((double)dist, temp);
                if (dist > 0.001f) {
                    x += (dx / dist) * limit;
                    y += (dy / dist) * limit;
                }
                };

            for (int i = 0; i < nH; i++) {
                applyMove(houseArray[i].getLocation().first, houseArray[i].getLocation().second, dispHouses[i].first, dispHouses[i].second);
            }
            for (size_t i = 0; i < centralBuildings.size(); i++) {
                applyMove(*centralBuildings[i].x, *centralBuildings[i].y, dispCentral[i].first, dispCentral[i].second);
            }

            temp -= dt;
            if (temp < 0.5) temp = 0.5;
        }

        // --- 4. Residents Micro-Simulation (Local) ---
        const int residentIterations = 15;

        for (int i = 0; i < nH; ++i) {
            std::vector<int> residents = houseArray[i].getResidents();
            if (residents.empty()) continue;

            float hx = houseArray[i].getLocation().first;
            float hy = houseArray[i].getLocation().second;

            // Reset
            for (int id : residents) {
                this->graph[id].setX(hx + (float)smallNoise(gen));
                this->graph[id].setY(hy + (float)smallNoise(gen));
            }

            for (int it = 0; it < residentIterations; ++it) {
                std::vector<std::pair<float, float>> rDisp(residents.size(), { 0.0f, 0.0f });

                for (size_t a = 0; a < residents.size(); ++a) {
                    Node& nodeA = this->graph[residents[a]];
                    // Pull to House
                    rDisp[a].first += (hx - nodeA.getLocation().first) * 0.3f;
                    rDisp[a].second += (hy - nodeA.getLocation().second) * 0.3f;

                    // Repel Neighbors
                    for (size_t b = 0; b < residents.size(); ++b) {
                        if (a == b) continue;
                        Node& nodeB = this->graph[residents[b]];
                        float rx = nodeA.getLocation().first - nodeB.getLocation().first;
                        float ry = nodeA.getLocation().second - nodeB.getLocation().second;
                        float d2 = rx * rx + ry * ry + 0.01f;
                        if (d2 < 64.0f) {
                            float f = 20.0f / d2;
                            rDisp[a].first += rx * f;
                            rDisp[a].second += ry * f;
                        }
                    }
                }
                for (size_t a = 0; a < residents.size(); ++a) {
                    Node& n = this->graph[residents[a]];
                    n.setX(n.getLocation().first + rDisp[a].first * 0.5f);
                    n.setY(n.getLocation().second + rDisp[a].second * 0.5f);
                }
            }
        }
    }
    void Graph::initialize() {
        std::ifstream file("C:\\Users\\ambre\\source\\repos\\OpenGLProject1\\x64\\Debug\\parameters.txt");
        if (!file.is_open()) {
            std::cout << "error opening file\n";
            return;
        }
        double array[18] = { 0 };
        int index = 0;
        std::string num;
        while (file >> num) {
            array[index++] = stod(num);
        }

        this->infectProbability = array[0];
        this->infectedNumber = (int)array[1];
        this->numberOfPeople = (int)array[2];
        this->vaccineCoverage = array[3];
        this->vaccineEfficacy = array[4];
        this->protectiveMeasureChance = array[5];
        this->protectiveMeasureEfficacy = array[6];
        this->disease.deathProbabilityChild = array[7];
        this->disease.deathProbabilityTeenager = array[8];
        this->disease.deathProbabilityAdult = array[9];
        this->disease.deathProbailityElderly = array[10];
        this->populationStatistics.percentageOfChildren = array[11];
        this->populationStatistics.percentageOfTeenagers = array[12];
        this->populationStatistics.percentageOfAdults = array[13];
        this->populationStatistics.percentageOfElderly = array[14];
        this->disease.incubationTime = (int)array[15];
        this->disease.minTime = (int)array[16];
        this->disease.maxTime = (int)array[17];
        dynamicGraphGenerator();
        file.close();
    }
    std::vector<int> Graph::infectInitial(int initialInfect) {
        //array of infected indices
        std::vector<int> infected;
        std::unordered_set<int> infectedCheck;
        int temp = 0;
        //infect people randomly
        while (temp < initialInfect) {
            int index = rand() % graph.size();
            //avoid duplicate entries
            if (infectedCheck.count(index)) {
                continue;
            }
            infected.push_back(index);
            infectedCheck.insert(index);
            //mark as infected
            this->graph[index].setInfected(this->disease.incubationTime);
            ++temp;
        }
        return infected;
    }
    Graph::Graph() {
        initialize();
        springLayout(WIDTH, HEIGHT, NUMBER_OF_ITERNATIONS_SPRING_LAYOUT);
    }

    double Graph::getinfectProbability() {
        return this->infectProbability;
    }
    double Graph::getinfectNumber() {
        return this->infectedNumber;
    }
    std::vector<Node>& Graph::getNodes() {
        return graph;
    }
    double Graph::getVaccineEfficacy() const {
        return this->vaccineEfficacy;
    }
    double Graph::getPrecautionaryMeasureEfficacy() {
        return this->protectiveMeasureEfficacy;
    }
    Disease Graph::getDisease() const {
        return this->disease;
    }
