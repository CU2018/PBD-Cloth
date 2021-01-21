#include "Cloth.h"


void Cloth::initIndexArray()
{
	int countVer = 0;
	int totalTris = (resX - 1)*(resY - 1) * 2;
	// printf("totalTris is %d\n", totalTris);
	//             1/5._______.4
	//                |       |
	//                |       |
	//               2|_______|3/6
	for (int j = 0; j < resY - 1; ++j)
	{
		for (int i = 0; i < resX - 1; ++i)
		{
			// 1
			indexArray.push_back((GLuint)(j*resX + i));

			// 2
			indexArray.push_back((GLuint)((j + 1)*resX + i));//j*resX + i + 1

			// 3
			indexArray.push_back((GLuint)((j + 1)*resX + i + 1));

			// 4
			indexArray.push_back((GLuint)(j*resX + i + 1)); //(j + 1)*resX + i + 1)

			// 5
			indexArray.push_back((GLuint)(j*resX + i)); // (j + 1)*resX + i)

			// 6
			indexArray.push_back((GLuint)((j + 1)*resX + i + 1));//j*resX + i

			countVer += 6;
		}
	}
	assert(countVer == totalTris*3);
}

void Cloth::init()
{
	createCloth(resX, resY, sizeX, sizeY, hasPosConstr);
	initIndexArray();
}

void Cloth::createCloth(int resX, int resY, float sizeX, float sizeY, bool hasPosConstr)
{
	// setting up initial points positions
	int totalPoints = resX * resY;
	float shearRestLength = sqrt(sizeX*sizeX + sizeY * sizeY);
	for (int j = 0; j < resY; ++j)
	{
		for (int i = 0; i < resX; ++i)
		{
			Vec3f newPos = Vec3f((float)i*sizeX, 0.0f, (float)j*sizeY);
			newPos += initPos;
			float newMass = 0.5f;

			// initialize point constraint
			Point newP;
			newP.pos = newPos;
			newP.vel = Vec3f(0.0f, 0.0f, 0.0f);
			newP.accel = Vec3f(0.0f, 0.0f, 0.0f);

			if (hasPosConstr && (j == resY - 1 && (i == 0 || i == resX - 1)))  // fix two points
			{
				newP.mass = INFINITY;
				points.push_back(newP);
				_posConstraintList.push_back(newPos);
			}
			else
			{
				newP.mass = newMass;
				points.push_back(newP);
			}

			// initialize constraint and restLength
			for (int x = -1; x < 2; ++x)
			{
				for (int y = -1; y < 2; ++y)
				{
					if (x == 0 && y == 0)
						continue;
					int currX = i + x;
					int currY = j + y;
					//if (i == 0 && j == 0) printf("currX: %d, currY: %d\n", currX, currY);
					int currIndex = Vec2iToInt(currX, currY);
					int currOrigin = Vec2iToInt(i, j);
					if (isInside(currX, currY) && currIndex > currOrigin)
					{
						//if (i == 0 && j == 0) printf("adding\n");
						distConstraintList.push_back(Vec2i(currOrigin, currIndex));
						if (x == y || x == -y)  // shear
							restLength.push_back(shearRestLength);
						else if (x > y) // horizontal; works only when we restrict that currIndex > currOrigin
							restLength.push_back(sizeX);
						else // verticle
							restLength.push_back(sizeY);
					}
				}
			}
		}
	}
	assert(totalPoints == points.size());
	assert(distConstraintList.size() == restLength.size());
}

void Cloth::update(float deltaTime, float dampingRate, bool hasPosConstr, int solverIter, Vec3f sphereCenter, float sphereRadius)
{
	// printf("updating... %f\n", deltaTime);
	// external forces (gravity ONLY)
	// --------------------------------
	Vec3f gravity = Vec3f(0, -9.8f, 0);
	for (int i = 0; i < this->points.size(); ++i)
	{
		if (this->points[i].mass != 0) // should always be true
		{
			if (!(i == resX * (resY - 1) || i == (resX * resY) - 1))  // do not conserve external forces
			{
				float invMass = 1 / this->points[i].mass;
				this->points[i].vel += deltaTime * invMass*gravity;
				// COARSE: damping velocities
				this->points[i].vel *= pow((1- dampingRate), deltaTime);
			}

			// add the predicted position with velocities
			this->points[i].predPos = this->points[i].pos + deltaTime * this->points[i].vel;
		}
	}

	// project constraints (ONLY distance contraints and position contraints for now)
	// ---------------------------------
	for (int iter = 0; iter < solverIter; iter++)
	{
		// distance contraint
		for (int i = 0; i < distConstraintList.size(); ++i)
		{
			Vec2i currDistConstr = distConstraintList[i]; // point-pair
			float currRestLength = restLength[i];
			Point pt1 = points[currDistConstr[0]];
			Point pt2 = points[currDistConstr[1]];
			Vec3f p1 = pt1.predPos;
			Vec3f p2 = pt2.predPos;

			Vec3f vecP2P1 = p1 - p2;
			float magP2P1 = mag(vecP2P1);
			if (magP2P1 <= M_EPSION)
				return;
			float w1 = 1 / pt1.mass;
			float w2 = 1 / pt2.mass;
			float invMass = w1 + w2;
			if (invMass <= M_EPSION)
				return;

			Vec3f n_val = vecP2P1 / magP2P1;  // direction
			float s_val = (magP2P1 - currRestLength) * (1 / invMass);  // scaler

			Vec3f distProj = s_val * n_val * k_stiff;
			if (w1 > 0.0) // should always be true
				points[currDistConstr[0]].predPos -= (distProj * w1);
			if (w2 > 0.0) // should always be true
				points[currDistConstr[1]].predPos += (distProj * w2);
		}
		// position constraint
		if (hasPosConstr)
			setPositionConstraint();
		
		// collision constraints
		for (int i = 0; i < points.size(); ++i)
		{
			Vec3f p2c = points[i].predPos - sphereCenter; // distance between current predpos to the center of the sphere
			float dist = mag(p2c); 
			// collision detection
			if (dist - sphereRadius < M_EPSION) // collide with the sphere
			{
				float distToGo = sphereRadius - dist; // the distance to set the current predpos to the surface of the sphere
				points[i].predPos += p2c * distToGo;  // direction * distance
			}
		}
	}

	// commit the velocity and the position changes
	// ---------------------------------
	for (int i = 0; i < this->points.size(); ++i)
	{
		// commit velosity based on position changes
		this->points[i].vel = (points[i].predPos - points[i].pos) / deltaTime;
		// printf("vel is %f\n", points[i].vel);
		// commit position
		this->points[i].pos = points[i].predPos;
		/*if (i == 95)
			printf("predPos x: %f, y: %f; currPos x: %f, y: %f; vel is %f\n", predPos[i][0], predPos[i][1], points[i].pos[0], points[i].pos[1], points[i].vel[1]);*/
	}
}

void Cloth::setPositionConstraint()
{
	// top left point = first point in last row
	int tlIndex = resX * (resY - 1);
	this->points[tlIndex].pos = _posConstraintList[0];
	// top right point = last point in last row
	int trIndex = (resX * resY) - 1;
	this->points[trIndex].pos = _posConstraintList[1];
}
