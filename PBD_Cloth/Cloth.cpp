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
	// printf("# of points are %d\n", countVer);
	// printf("last point is %f, %f, %f", clothVertices[3 * 485], clothVertices[3 * 485 + 1], clothVertices[3 * 485 + 2]);
	//printf("size of the index array: %d\n", indexArray.size());
	/*printf("point seq is: ");
	for (int i = 0; i < indexArray.size(); ++i)
	{
		printf(" %d ", indexArray[i]);
	}*/
}

void Cloth::init()
{
	createCloth(resX, resY, sizeX, sizeY, hasPosConstr);
	initIndexArray();
	//bindBuffers();
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
			Vec3f newPos = Vec3f((float)i*sizeX, (float)j*sizeY, 0.0f);
			float newMass = 0.01f;

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

			/*if (i == 2 && j == 0)
				printf("position of point2 is %f, %f\n", newP.pos[0], newP.pos[1]);*/

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
	// check  position constraint
	/*printf("position constr pos %d: pos: %f,%f,%f; %f,%f,%f\n", posConstraintList.size(), posConstraintList[0][0], posConstraintList[0][1],
		posConstraintList[0][2],posConstraintList[1][0],posConstraintList[1][1], posConstraintList[1][2]);*/

	/*for (int k = 25; k < 50; ++k)
	{
		printf("constr (%d, %d) -- rest length is %f \n", distConstraintList[k][0], distConstraintList[k][1], restLength[k]);
	}*/
	assert(totalPoints == points.size());
	assert(distConstraintList.size() == restLength.size());
	// printf("total points is %d\n ", points.size());
	//printf("total constraint is %d\n ", constraintList.size());
}

void Cloth::update(float deltaTime, bool hasPosConstr, int solverIter)
{
	// printf("updating... %f\n", deltaTime);
	// external forces (gravity ONLY)
	// --------------------------------
	Vec3f gravity = Vec3f(0, -9.8f, 0);
	std::vector<Vec3f> predPos;
	for (int i = 0; i < this->points.size(); ++i)
	{
		if (this->points[i].mass != 0) // should always be true
		{
			if (!(i == resX * (resY - 1) || i == (resX * resY) - 1))  // do not conserve external forces
			{
				float invMass = 1 / this->points[i].mass;
				this->points[i].vel += deltaTime * invMass*gravity;
				// COARSE: damping velocities
				this->points[i].vel *= 0.9;
			}

			// add the predicted position with velocities
			predPos.push_back(this->points[i].pos + deltaTime * this->points[i].vel);
		}
	}
	assert(predPos.size() == points.size());
	// TODO: generate Collision contraints
	// ---------------------------------

	// project constraints (ONLY distance contraints and position contraints for now)
	// ---------------------------------
	for (int iter = 0; iter < solverIter; iter++)
	{
		// distance contraint
		for (int i = 0; i < distConstraintList.size(); ++i)
		{
			//printf("calculating constraint..\n");
			Vec2i currDistConstr = distConstraintList[i]; // point-pair
			float currRestLength = restLength[i];
			Point pt1 = points[currDistConstr[0]];
			Point pt2 = points[currDistConstr[1]];
			Vec3f p1 = predPos[currDistConstr[0]];
			Vec3f p2 = predPos[currDistConstr[1]];

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
				predPos[currDistConstr[0]] -= (distProj * w1);
			if (w2 > 0.0) // should always be true
				predPos[currDistConstr[1]] += (distProj * w2);
		}
		// position constraint
		if (hasPosConstr)
			setPositionConstraint();
	}


	// commit the velosity and the position changes
	// ---------------------------------
	for (int i = 0; i < this->points.size(); ++i)
	{
		// commit velosity based on position changes
		this->points[i].vel = (predPos[i] - points[i].pos) / deltaTime;
		// printf("vel is %f\n", points[i].vel);
		// commit position
		this->points[i].pos = predPos[i];
		/*if (i == 95)
			printf("predPos x: %f, y: %f; currPos x: %f, y: %f; vel is %f\n", predPos[i][0], predPos[i][1], points[i].pos[0], points[i].pos[1], points[i].vel[1]);*/
	}

	// TODO: velocity update due to the friction
	// ---------------------------------
	/*for (int i = distConstraintList.size() - 5; i < distConstraintList.size(); ++i)
	{
		printf("constraint %d(%d,%d) pos1: %f, %f, %f; pos2: %f, %f, %f\n", i, distConstraintList[i][0], distConstraintList[i][1], 
			points[distConstraintList[i][0]].pos[0], points[distConstraintList[i][0]].pos[1], points[distConstraintList[i][0]].pos[2],
			points[distConstraintList[i][1]].pos[0], points[distConstraintList[i][1]].pos[1], points[distConstraintList[i][1]].pos[2]);
	}*/
}

void Cloth::save(std::string path)
{

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
//
//void Cloth::bindBuffers()
//{
//	glGenBuffers(1, &_vertexBuffer);
//	glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(Point)*points.size(), &points[0], GL_STATIC_DRAW);
//
//	glGenBuffers(1, &_indexBuffer);
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexArray.size() * sizeof(indexArray[0]), &indexArray[0], GL_STATIC_DRAW);
//	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
//	/*glBindVertexArray(VAO);
//
//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(Point)*points.size(), &points[0], GL_STATIC_DRAW);
//
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexArray.size() * sizeof(indexArray[0]), &indexArray[0], GL_STATIC_DRAW);
//
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
//	glEnableVertexAttribArray(0);
//
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	glBindVertexArray(0);*/
//}
//
//void Cloth::render(Shader myShader, glm::mat4 model, glm::mat4 view, glm::mat4 projection)
//{
//	// printf("calling rendering");
//	/*glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
//	glClear(GL_COLOR_BUFFER_BIT);
//
//	glBindVertexArray(VAO); 
//	glDrawElements(GL_TRIANGLES, indexArray.size(), GL_UNSIGNED_INT, NULL);*/
//
//	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
//	glEnable(GL_BLEND);
//	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
//
//	myShader.use();
//
//	glBindBuffer(GL_ARRAY_BUFFER, this->_vertexBuffer);//pos0 pos1 pos2 color0 color2 color3
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBuffer);
//
//	myShader.setMat4("model", model);
//	myShader.setMat4("view", view);
//	myShader.setMat4("projection", projection);
//
//	//look @ me */
//	//render
//	glDrawElements(GL_TRIANGLES, indexArray.size(), GL_UNSIGNED_INT, NULL);
//
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
//
//	glUseProgram(0);
//}

