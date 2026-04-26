#include "office.h"

Office::Office(int officeID, std::pair<float, float>& location) {
	this->officeID = officeID;
	this->location = move(location);
}
void Office::addEmployee(int ID) {
	this->employees.push_back(ID);
}
void Office::updateLocationX(float x) {
	this->location.first = x;
}
void Office::updateLocationY(float y) {
	this->location.second = y;
}
std::vector<int>& Office::getEmployees() {
	return this->employees;
}
std::pair<float, float>& Office::getLocation() {
	return this->location;
}
int Office::getOfficeID() const {
	return this->officeID;
}