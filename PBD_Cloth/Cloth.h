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
		Vec3f vel; // velocity of the point
		Vec3f accel; // acceleration of the point
		float mass;
	};

	int resX, resY;  // # of points on each width and height
	float sizeX, sizeY;  // size of the length between each two points (could be used to initialize the restLength)
	float k_stiff;  // stiffness of the distance constraint
	bool hasPosConstr;
	std::vector<Point> points; // the points that constructs the piece of cloth
	std::vector<Vec2i> distConstraintList;  // containing the distance constrains between the edges
	std::vector<float> restLength; // the rest lengths between each two points of the cloth
	std::vector<GLuint> indexArray;  // for drawing the cloth grid

	GLuint _vertexBuffer;
	GLuint _indexBuffer;

	Cloth() {}
	~Cloth() {};
	Cloth(int resX, int resY, float sizeX, float sizeY, float k_stiff, bool hasPosConstr)
		: resX(resX), resY(resY), sizeX(sizeX), sizeY(sizeY), k_stiff(k_stiff), hasPosConstr(hasPosConstr){
		init();
	}
	void update(float deltaTime, bool hasPosConstr, int solverIter); // change the positions and velosities of each point
	void save(std::string path);  // store the object to the hard disk
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
//	void createCloth(int resX, int resY, float sizeX, float sizeY)
//	{
//		// setting up initial points positions
//		int totalPoints = resX * resY;
//		float shearRestLength = sqrt(sizeX*sizeX + sizeY * sizeY);
//		for (int j = 0; j < resY; ++j)
//		{
//			for (int i = 0; i < resX; ++i)
//			{
//				Vec3f newPos = Vec3f((float)i*sizeX, (float)j*sizeY, 0.0f);
//				float newMass = 1.0f;
//
//				// initialize point constraint
//				if (j == resY - 1 && (i == 0 || i == resX - 1))
//				{
//					Point newP(newPos, Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f), INFINITY);
//					points.push_back(newP);
//					posConstraintList.push_back(newPos);
//				}
//				else
//				{
//					Point newP(newPos, Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f), newMass);
//					points.push_back(newP);
//				}
//				
//				// keep the vertices for OpenGL drawing
//				verticesArray.push_back(newPos);
//
//				/*if (i == 2 && j == 0)
//					printf("position of point2 is %f, %f\n", newP.pos[0], newP.pos[1]);*/
//
//				// initialize constraint and restLength
//				for (int x = -1; x < 2; ++x)
//				{
//					for (int y = -1; y < 2; ++y)
//					{
//						if (x == 0 && y == 0)
//							continue;
//						int currX = i + x;
//						int currY = j + y;
//						//if (i == 0 && j == 0) printf("currX: %d, currY: %d\n", currX, currY);
//						int currIndex = Vec2iToInt(currX, currY);
//						int currOrigin = Vec2iToInt(i, j);
//						if (isInside(currX, currY) && currIndex > currOrigin)
//						{
//							//if (i == 0 && j == 0) printf("adding\n");
//							distConstraintList.push_back(Vec2i(currOrigin, currIndex));
//							if (x == y || x == -y)  // shear
//								restLength.push_back(shearRestLength);
//							else if (x > y) // horizontal; works only when we restrict that currIndex > currOrigin
//								restLength.push_back(sizeX);
//							else // verticle
//								restLength.push_back(sizeY);
//						}	
//					}
//				}
//			}
//		}
//		// check  position constraint
//		/*printf("position constr pos %d: pos: %f,%f,%f; %f,%f,%f\n", posConstraintList.size(), posConstraintList[0][0], posConstraintList[0][1],
//			posConstraintList[0][2],posConstraintList[1][0],posConstraintList[1][1], posConstraintList[1][2]);*/
//
//		/*for (int k = 25; k < 50; ++k)
//		{
//			printf("constr (%d, %d) -- rest length is %f \n", distConstraintList[k][0], distConstraintList[k][1], restLength[k]);
//		}*/
//		assert(verticesArray.size() == points.size());
//		assert(totalPoints == points.size());
//		assert(distConstraintList.size() == restLength.size());
//		// printf("total points is %d\n ", points.size());
//		//printf("total constraint is %d\n ", constraintList.size());
//
//		// setting up constraints
//
//		// horizontal 
//		//for (int j = 0; j < resY; ++j)	
//		//	for (int i = 0; i < resX-1; ++i) {
//		//		Vec2i newConstr(j*resX+i, j*resX + i+1);
//		//		constraintList.push_back(newConstr);
//		//		restLength.push_back(sizeX);
//		//	}
//		//// verticle
//		//for (int i = 0; i < resX; ++i)
//		//	for (int j = 0; i < resY - 1; ++j) {
//		//		Vec2i newConstr(j*resX + i, (j + 1)*resX + i);
//		//		constraintList.push_back(newConstr);
//		//		restLength.push_back(sizeY);
//		//	}
//		//// shear
//		//float shearConstr = sqrt(sizeX*sizeX+sizeY*sizeY);
//		//for (int j = 0; j < resY; ++j)
//		//	for (int i = 0; i < resX - 1; ++i) {
//		//		Vec2i newConstr1(j*resX + i, (j + 1)*resX + i + 1);
//		//		constraintList.push_back(newConstr1);
//		//		Vec2i newConstr2((j+1)*resX + i, j*resX + i + 1);
//		//		constraintList.push_back(newConstr2);
//		//		restLength.push_back(shearConstr);
//		//		restLength.push_back(shearConstr);
//		//	}
//		//assert(constraintList.size() == restLength.size());
//	}
//};
//
//void Cloth::init()
//{
//	createCloth(resX, resY, sizeX, sizeY);
//}
//
//void Cloth::update(float deltaTime, bool hasPosConstr, int solverIter)
//{
//	// printf("updating... %f\n", deltaTime);
//	// external forces (gravity ONLY)
//	// --------------------------------
//	Vec3f gravity = Vec3f(0, -9.8f, 0);
//	std::vector<Vec3f> predPos;
//	for (int i = 0; i < this->points.size(); ++i)
//	{
//		if (this->points[i].mass != 0) // should always be true
//		{
//			if (!(i == resX * (resY - 1) || i == (resX * resY) - 1))  // do not conserve external forces
//			{
//				float invMass = 1 / this->points[i].mass;
//				this->points[i].vel += deltaTime*invMass*gravity;
//				// COARSE: damping velocities
//				this->points[i].vel *= 0.9;
//			}
//			
//			// add the predicted position with velocities
//			predPos.push_back(this->points[i].pos + deltaTime * this->points[i].vel);
//		}
//	}
//	assert(predPos.size() == points.size());
//	// TODO: generate Collision contraints
//	// ---------------------------------
//
//	// project constraints (ONLY distance contraints and position contraints for now)
//	// ---------------------------------
//	for (int iter = 0; iter < solverIter; iter++)
//	{
//		// distance contraint
//		for(int i = 0; i < distConstraintList.size(); ++i)
//		{
//			//printf("calculating constraint..\n");
//			Vec2i currDistConstr = distConstraintList[i]; // point-pair
//			float currRestLength = restLength[i];
//			Point pt1 = points[currDistConstr[0]];
//			Point pt2 = points[currDistConstr[1]];
//			Vec3f p1 = predPos[currDistConstr[0]]; 
//			Vec3f p2 = predPos[currDistConstr[1]];
//
//			Vec3f vecP2P1 = p1 - p2;
//			float magP2P1 = mag(vecP2P1);
//			if (magP2P1 <= M_EPSION)
//				return;
//			float w1 = 1 / pt1.mass;
//			float w2 = 1 / pt2.mass;
//			float invMass = w1 + w2;
//			if (invMass <= M_EPSION)
//				return;
//
//			Vec3f n_val = vecP2P1 / magP2P1;  // direction
//			float s_val = (magP2P1 - currRestLength) / invMass;  // scaler
//
//			Vec3f distProj = s_val * n_val * k_stiff;
//			if (w1 > 0.0) // should always be true
//				p1 -= w1 * distProj;
//			if (w2 > 0.0) // should always be true
//				p2 += w2 * distProj;
//		}
//		// position constraint
//		if(hasPosConstr)
//			setPositionConstraint();
//	}
//
//
//	// commit the velosity and the position changes
//	// ---------------------------------
//	for (int i = 0; i < this->points.size(); ++i)
//	{
//		// commit velosity based on position changes
//		this->points[i].vel = (predPos[i] - points[i].pos) / deltaTime;
//		// printf("vel is %f\n", points[i].vel);
//		// commit position
//		this->points[i].pos = predPos[i];
//		this->verticesArray[i] = predPos[i];  // update the vertices for OpenGL drawing
//		/*if (i == 95)
//			printf("predPos x: %f, y: %f; currPos x: %f, y: %f; vel is %f\n", predPos[i][0], predPos[i][1], points[i].pos[0], points[i].pos[1], points[i].vel[1]);*/
//	}
//
//	// TODO: velocity update due to the friction
//	// ---------------------------------
//}
//
//void Cloth::save(std::string path) 
//{
//
//}
//
//void Cloth::setPositionConstraint()
//{
//	// top left point = first point in last row
//	int tlIndex = resX * (resY-1);
//	this->points[tlIndex].pos = posConstraintList[0];
//	// top right point = last point in last row
//	int trIndex = (resX * resY)-1;
//	this->points[trIndex].pos = posConstraintList[1];
//}

#endif