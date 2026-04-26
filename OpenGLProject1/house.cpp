#include "house.h"

House::House(int houseID,std::vector<int>& residents,std::pair<float,float>& location ) {
	this->houseID = houseID;
	this->residents = move(residents);
	this->location = move(location);
}
void House::addResident(int ID) {
	this->residents.push_back(ID);
}
void House::updateLocationX(float x) {
	this->location.first = x;
}
void House::updateLocationY(float y) {
	this->location.second = y;
}
std::vector<int>& House::getResidents() {
	return this->residents;
}
std::pair<float, float>& House::getLocation() {
	return this->location;
}
int House::getHouseID() const {
	return this->houseID;
}