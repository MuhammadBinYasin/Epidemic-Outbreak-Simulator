#ifndef SHOPPING_CENTER_H
#define SHOPPING_CENTER_H

#include "node.h"
class shoppingCenter {
private:
	int shoppingCenterID;
	std::vector<int> customers;
	std::pair<float, float> location;
public:
	//constructor
	shoppingCenter(int shoppingCenterID, std::pair<float, float>& location);
	//add resident
	void addCustomer(int ID);
	//modify location
	void updateLocationX(float x);
	void updateLocationY(float y);

	//getter
	std::vector<int>& getCustomers();
	std::pair<float, float>& getLocation();
	int getshoppingCenterID() const;
};

#endif
