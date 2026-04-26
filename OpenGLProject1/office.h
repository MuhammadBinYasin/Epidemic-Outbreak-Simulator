#ifndef OFFICE_H
#define OFFICE_H

#include "node.h"
class Office {
private:
	int officeID;
	std::vector<int> employees;
	std::pair<float, float> location;
public:
	//constructor
	Office(int officeID, std::pair<float, float>& location);
	//add resident
	void addEmployee(int ID);
	//modify location
	void updateLocationX(float x);
	void updateLocationY(float y);

	//getter
	std::vector<int>& getEmployees();
	std::pair<float, float>& getLocation();
	int getOfficeID() const;
};

#endif
