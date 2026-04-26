#include "node.h"

Node::Node(int ID, std::pair<double, double> location, POPULATION category, bool isVaccinated, bool takesPrecautionaryMeasures,
    int houseID,int officeID,int shoppingCenterID,int schoolID) {
    this->ID = ID;
    this->location = move(location);
    isInfected = false;
    timeRemaining = 0;
    dead = false;
    recovered = false;
    this->color = GREEN;
    this->category = category;
    this->isVaccinated = isVaccinated;
    this->takesPrecautionaryMeasures = takesPrecautionaryMeasures;
    this->incubationTime = -1;
    this->inIncubation = false;
    this->houseID = houseID;
    this->officeID = officeID;
    this->schoolID = schoolID;
    this->shoppingCenterID = shoppingCenterID;
}
    //getters
int Node::getID() const {
    return ID;
}
std::pair<float, float>& Node::getLocation() {
    return location;
}
bool Node::isDead() const {
    return dead;
}
bool Node::getInfected() const {
    return isInfected;
}
bool Node::isRecovered() const {
    return recovered;
}
int Node::getTimeRemaining() const {
    return timeRemaining;
}
COLOR Node::getColor() const {
    return color;
}
POPULATION Node::getPopulation() const {
    return category;
}
bool Node::getVaccinated() const {
    return isVaccinated;
}
bool Node::takesProtectiveMeasure() const{
    return takesPrecautionaryMeasures;
}
bool Node::getinIncubation() const {
    return inIncubation;
}
int Node::getHouseID() const {
    return this->houseID;
}
int Node::getSchoolID() const {
    return this->schoolID;
}
int Node::getOfficeID() const {
    return this->officeID;
}
int Node::getShoppingCenterID() const {
    return this->shoppingCenterID;
}
//setters
void Node::setX(float x) {
    this->location.first = x;
}
void Node::setY(float y) {
    this->location.second = y;
}
void Node::setDead() {
    this->color = BLACK;
    this->isInfected = false;
    dead = true;
}

void Node::setInfected(int incubationTime) {
    this->incubationTime = incubationTime;
    this->color = YELLOW;
    inIncubation = true;
}
void Node::updateTime(int minTime, int maxTime, std::exponential_distribution<double>& timeDistribtion, std::mt19937& generator) {
    if (inIncubation) {
        --incubationTime;
        if (incubationTime == 0) {
            this->color = RED;
            this->inIncubation = false;
            this->isInfected = true;
            double duration = minTime + (int)timeDistribtion(generator);
            duration = std::min((double)maxTime, duration);
            switch (category) {
            case CHILD:
                timeRemaining = (int)(duration * 0.8);
                break;
            case TEENAGERS:
                timeRemaining = (int)(duration * 0.9);
                break;
            case ADULT:
                timeRemaining = (int)duration;
                break;
            case ELDERLY:
                timeRemaining = (int)(duration * 1.2);
            }
        }
    }
    else if (isInfected) {
        --timeRemaining;
        if (timeRemaining == 0) {
            this->color = BLUE;
            recovered = true;
            isInfected = false;
        }
    }
}
void Node::setColor(COLOR color) {
    this->color = color;
}

