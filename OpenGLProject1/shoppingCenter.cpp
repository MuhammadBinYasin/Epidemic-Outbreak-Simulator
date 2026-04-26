#include "shoppingCenter.h"

shoppingCenter::shoppingCenter(int shoppingCenterID, std::pair<float, float>& location) {
	this->shoppingCenterID = shoppingCenterID;
	this->location = move(location);
}
void shoppingCenter::addCustomer(int ID) {
	this->customers.push_back(ID);
}
void shoppingCenter::updateLocationX(float x) {
	this->location.first = x;
}
void shoppingCenter::updateLocationY(float y) {
	this->location.second = y;
}
std::vector<int>& shoppingCenter::getCustomers() {
	return this->customers;
}
std::pair<float, float>& shoppingCenter::getLocation() {
	return this->location;
}
int shoppingCenter::getshoppingCenterID() const {
	return this->shoppingCenterID;
}