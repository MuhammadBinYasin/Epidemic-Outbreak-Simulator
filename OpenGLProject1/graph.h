#ifndef GRAPH_H
#define GRAPH_H


#define WIDTH  1920
#define HEIGHT 1080
#define NUMBER_OF_ITERATIONS_CIRCLE 100
#define NUMBER_OF_ITERNATIONS_SPRING_LAYOUT 100

#include <utility>
#include <mutex>
#include <fstream>
#include <thread>
#include <iostream>
#include <string>
#include <unordered_set>

#include "node.h"
#include "house.h"
#include "shoppingCenter.h"
#include "office.h"
#include "school.h"

struct PopulationStatistics {
    float percentageOfChildren;
    float percentageOfTeenagers;
    float percentageOfAdults;
    float percentageOfElderly;
};
struct Disease {
    int incubationTime;
    int minTime;
    int maxTime;
    float deathProbabilityChild;
    float deathProbabilityTeenager;
    float deathProbabilityAdult;
    float deathProbailityElderly;
    float precautinoryMeasureEffectiveness;
};

class Graph {

private:
    std::random_device device;
    std::mutex Lock;
    //Storing population stats
    PopulationStatistics populationStatistics;
    //disease paramter to store death probabilities
    Disease disease;
    //% of people taking protective measures and efficacy of measure
    double protectiveMeasureChance;
    double protectiveMeasureEfficacy;
    //vaccine coverage
    double vaccineCoverage;
    double vaccineEfficacy;
    //infection spread chance
    double infectProbability;
    //infected people
    int infectedNumber;
    //number of people(nodes)
    int numberOfPeople;
    //store the graph
    

    void dynamicGraphGenerator();
    void springLayout(float width, float height, int iterations);
    void initialize();
public:
    std::vector<Node> graph;
    std::vector<House> houseArray;
    std::vector<School> schoolArray;
    std::vector<Office> officeArray;
    std::vector<shoppingCenter> shoppingCenterArray;

    std::vector<int> infectInitial(int initialInfect);
    Graph();
    double getinfectProbability();
    double getinfectNumber();
    std::vector<Node>& getNodes();
    double getVaccineEfficacy() const;
    double getPrecautionaryMeasureEfficacy();
    Disease getDisease() const;
};
#endif // !GRAPH_H
