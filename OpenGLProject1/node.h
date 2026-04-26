#pragma once
#include <utility>
#include <vector>
#include <random>
#ifndef NODE_H
#define NODE_H

/*
    BLACK = DEAD
    RED = INFECTED
    GREEN = HEALTHY
    BLUE = RECOVERED
    YELLOW = INCUBATION PERIOD
*/
enum COLOR {
    BLACK,
    RED,
    GREEN,
    BLUE,
    YELLOW
};
enum POPULATION {
    CHILD,
    TEENAGERS,
    ADULT,
    ELDERLY
};
class Node {
private:
    COLOR color;
    POPULATION category;
    std::pair<float, float> location;

    int ID;
    int incubationTime;
    int timeRemaining;
    int houseID;
    int officeID;
    int shoppingCenterID;
    int schoolID;
    bool isVaccinated;
    bool takesPrecautionaryMeasures;
    bool inIncubation;
    bool dead;
    bool isInfected;
    bool recovered;
public:
    Node(int ID, std::pair<double, double> location,POPULATION category, bool isVaccinated, bool takesPrecautionaryMeasures,int houseID,int officeID,int shoppingCenterID,int schoolID);
    //getters
    int getID() const;
    std::pair<float, float>& getLocation();
    bool isDead() const;
    bool getInfected() const;
    bool isRecovered() const;
    int getTimeRemaining() const;
    COLOR getColor() const;
    POPULATION getPopulation() const;
    bool getVaccinated() const;
    bool takesProtectiveMeasure() const;
    bool getinIncubation() const;

    int getHouseID() const;
    int getShoppingCenterID() const;
    int getSchoolID() const;
    int getOfficeID() const; 
    //setters
    void setX(float x);
    void setY(float y);
    void setDead();
    void setInfected(int incubationTime);
    void updateTime(int minTime, int maxTime, std::exponential_distribution<double>& timeDistribtion, std::mt19937& generator);
    void setColor(COLOR color);
};
#endif // !NODE_H

