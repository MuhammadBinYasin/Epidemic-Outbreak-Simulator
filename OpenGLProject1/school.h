#ifndef SCHOOL_H
#define SCHOOL_H
#include "node.h"
class School {
private:
	int schoolID;
	std::vector<int> students;
	std::pair<float, float> location;
public:
	//constructor
	School(int schoolID, std::pair<float, float>& location);
	//add resident
	void addStudent(int ID);
	//modify location
	void updateLocationX(float x);
	void updateLocationY(float y);

	//getter
	std::vector<int>& getStudents();
	std::pair<float, float>& getLocation();
	int getSchoolID() const;
};
#endif // !SCHOOL_H
