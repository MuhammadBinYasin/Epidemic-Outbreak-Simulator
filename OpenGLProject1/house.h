#ifndef HOUSE_H
#define HOUSE_H

#include "node.h"
class House {
private:
	int houseID;
	std::vector<int> residents;
	std::pair<float, float> location;
public:
	//constructor
	House(int houseID,std::vector<int>& residents,std::pair<float,float>& location);
	//add resident
	void addResident(int ID);
	//modify location
	void updateLocationX(float x);
	void updateLocationY(float y);

	//getter
	std::vector<int>& getResidents();
	std::pair<float,float>& getLocation();
	int getHouseID() const;
};

#endif