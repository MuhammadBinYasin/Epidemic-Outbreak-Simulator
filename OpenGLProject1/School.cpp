#include "school.h"
School::School(int schoolID, std::pair<float, float>& location) {
	this->schoolID = schoolID;
	this->location = move(location);
}
void School::addStudent(int ID) {
	this->students.push_back(ID);
}
void School::updateLocationX(float x) {
	this->location.first = x;
}
void School::updateLocationY(float y) {
	this->location.second = y;
}
std::vector<int>& School::getStudents() {
	return this->students;
}
std::pair<float, float>& School::getLocation() {
	return this->location;
}
int School::getSchoolID() const {
	return this->schoolID;
}