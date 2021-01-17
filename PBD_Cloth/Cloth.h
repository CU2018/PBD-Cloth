#ifndef CLOTH_H
#define CLOTH_H

#include <vector>
#include <math.h>
#include <string>
#include <iostream>
#include "Vec.h"
#include "Shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define DEBUG_ID 

class Cloth
{
public:
	struct Point
	{
		Vec3f pos; // position of the point
		Vec3f predPos;  // predicted position stored here in update process
		Vec3f vel; // velocity of the point
		Vec3f accel; // acceleration of the point
		float mass;
	};

	int resX, resY;  // # of points on each width and height
	float sizeX, sizeY;  // size of the length between each two points (could be used to initialize the restLength)
	float k_stiff;  // stiffness of the distance constraint
	bool hasPosConstr;
	Vec3f initPos;  // init pos in the world coordinate
	std::vector<Point> points; // the points that constructs the piece of cloth
	std::vector<Vec2i> distConstraintList;  // containing the distance constrains between the edges
	std::vector<float> restLength; // the rest lengths between each two points of the cloth
	std::vector<GLuint> indexArray;  // for drawing the cloth grid

	GLuint _vertexBuffer;
	GLuint _indexBuffer;

	Cloth() {}
	~Cloth() {};
	Cloth(int resX, int resY, float sizeX, float sizeY, float k_stiff, bool hasPosConstr, Vec3f initPos)
		: resX(resX), resY(resY), sizeX(sizeX), sizeY(sizeY), k_stiff(k_stiff), hasPosConstr(hasPosConstr), initPos(initPos){
		init();
	}
	void update(float deltaTime, float dampingRate, bool hasPosConstr, int solverIter, Vec3f sphereCenter, float sphereRadius); // change the positions and velosities of each point
	// void save(std::string path);  // store the object to the hard disk
	// void bindBuffers();
	// void render(Shader myShader, glm::mat4 model, glm::mat4 view, glm::mat4 projection);

private:
	std::vector<Vec3f> _posConstraintList;  // stores the position of position contraints

	void init();  // initialize the restLength
	bool isInside(int x, int y) { return x >= 0 && y >= 0 && x < resX && y < resX; } // check whether the current checking point is inside the grid
	int Vec2iToInt(int p0, int p1) { return p1 * resX + p0; }  // change from vec2i of constraint to grid point index
	void createCloth(int resX, int resY, float sizeX, float sizeY, bool hasPosConstr);
	void initIndexArray();
	void setPositionConstraint(); // only used in single cloth mode to check updating
};

#endif