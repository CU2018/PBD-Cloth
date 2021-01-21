#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Util.h"
#include "Vec.h"
#include "Cloth.h"

#include <iostream>

// simulation settings
int maxFrames = 240;
int maxSubstep = 10;
float FPS = 24.0f;
float timeStep = 1.0f / (FPS*maxSubstep); //1.0/240f;
int solverIteration = 10;
float dampingRate = 0.9f;

// OpenGL functions
// void copyVertices(Cloth& newCloth);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
GLint gltWriteTGA(const char *szFileName);
void setCloth(Cloth newCloth, unsigned int VAO_1, unsigned int VBO_1, unsigned int EBO);
void setSphere(unsigned int VAO_2);
void renderCloth(Cloth newCloth, unsigned int VAO_1, unsigned int VBO_1, unsigned int EBO);
void renderSphere(unsigned int VAO_2);

// object for demoing collision
#define SPHERE_VEC 3888
void drawSphere();
float sphereVertices[SPHERE_VEC * 3];
Vec3f spherePos(0.0f, 0.0f, 0.0f);
float sphereRadius = 5.0f;

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
glm::vec3 cameraPos = glm::vec3(20.0f, 0.0f, -30.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// cloth
int resX = 51, resY = 51;
float sizeX = 0.45, sizeY = 0.6;
const float DIST_K_STIFF = 1;   // stiffness of the distance constraint
bool hasPosConstraint = true;  // true: fix the top left and right points; false: don't fix
float angle = -90.0f;

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PBD Cloth Simulation", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile our shader zprogram
	// ------------------------------------
	Shader myShader("shader.vs", "shader.fs");

	// create cloth obj
	Vec3f clothPos(-10.0f, 10.0f, -20.0f);  // tranlate to the center
	Cloth newCloth(resX, resY, sizeX, sizeY, DIST_K_STIFF, hasPosConstraint, clothPos);
	// printf("new cloth: %d, %d, %f, %f", resX, resY, sizeX, sizeY);
	

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// --------------------------
	drawSphere();

	unsigned int VBO_1, VBO_2, VAO_1, VAO_2, EBO;
	glGenVertexArrays(1, &VAO_1);
	glGenVertexArrays(2, &VAO_2);
	glGenBuffers(1, &VBO_1);
	glGenBuffers(2, &VBO_2);
	glGenBuffers(1, &EBO);

	// cloth settings
	setCloth(newCloth, VAO_1, VBO_1, EBO);
	glEnableVertexAttribArray(0);

	//// sphere settings
	setSphere(VAO_2);
	glEnableVertexAttribArray(0);

	//// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	//// -------------------------------------------------------------------------------------------
	myShader.use();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// render loop
	// -----------
	
	//while (!glfwWindowShouldClose(window))
	//{	
		for (int frameNum = 1; frameNum <= maxFrames; ++frameNum)
		{
			for(int substep = 1; substep <= maxSubstep; ++substep)
			{
				// per-frame time logic
				// --------------------
				float currentFrame = glfwGetTime();
				deltaTime = currentFrame - lastFrame;
				lastFrame = currentFrame;

				// update cloth state; Physics simulation using fixed deltatime
				newCloth.update(timeStep, dampingRate, hasPosConstraint, solverIteration, spherePos, sphereRadius);
				// input
				// -----
				processInput(window);

				// render
				// ------
				glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				// activate shader
				myShader.use();

				// pass projection matrix to shader (note that in this case it could change every frame)
				glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
				myShader.setMat4("projection", projection);

				// camera/view transformation
				glm::mat4 view = glm::lookAt(cameraPos,
								glm::vec3(0.0f, 0.0f, 0.0f),
								cameraUp);
				myShader.setMat4("view", view);

				// model transformation
				glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
				myShader.setMat4("model", model);

				// render cloth
				renderCloth(newCloth, VAO_1, VBO_1, EBO);

				// render sphere
				glm::mat4 model_s = glm::mat4(1.0f);
				model_s = glm::scale(model_s, glm::vec3(5.0f, 5.0f, 5.0f));
				model_s = glm::translate(model_s, glm::vec3(spherePos[0], spherePos[1], spherePos[2]));
				myShader.setMat4("model", model_s);
				renderSphere(VAO_2);

				// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
				// -------------------------------------------------------------------------------
				glfwSwapBuffers(window);
				glfwPollEvents();
			}
			// save each frame as a targa file
			std::string fileName = std::to_string(frameNum) + "_frame.tga";
			const char * c = fileName.c_str();
			if(gltWriteTGA(c))
				printf("saving %d sucess!\n", frameNum);
		}
	//}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO_1);
	glDeleteBuffers(1, &VBO_1);
	glDeleteVertexArrays(1, &VAO_2);
	glDeleteBuffers(1, &VBO_2);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraSpeed = 2.5 * deltaTime;
	if(glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)  // press u to enable mouse movement
		glfwSetCursorPosCallback(window, mouse_callback);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	fov -= (float)yoffset;
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 45.0f)
		fov = 45.0f;
}

void drawSphere() {
	int k = 0;
	//sphere
	// phi = degree of angle
	float DegreesToRadians = M_PI / 180.0;
	for (float phi = -90.0; phi < 90.0; phi += 10.0)
	{
		// the <math.h>'s sin, cos, and tan work with radians only.
		// In each loop, draw two triangle
		float phiR = phi * DegreesToRadians;
		float phiR10 = (phi + 10) * DegreesToRadians;

		for (float theta = -180.0; theta < 180.0; theta += 10.0)
		{
			float thetaR = theta * DegreesToRadians;
			float thetaR10 = (theta + 10) * DegreesToRadians;
			// In every square
			// A1 - B1
			// | \  |
			// |  \ |
			// A2 - B2

			// A1
			sphereVertices[k+0] = sin(thetaR)*cos(phiR);
			sphereVertices[k+1] = cos(thetaR)*cos(phiR);
			sphereVertices[k+2] = sin(phiR);
			k += 3;
			// B1
			sphereVertices[k+0] = sin(thetaR)*cos(phiR10);
			sphereVertices[k+1] = cos(thetaR)*cos(phiR10);
			sphereVertices[k+2] = sin(phiR10);
			k += 3;
			// B2
			sphereVertices[k+0] = sin(thetaR10)*cos(phiR10);
			sphereVertices[k+1] = cos(thetaR10)*cos(phiR10);
			sphereVertices[k+2] = sin(phiR10);
			k += 3;
			// B2
			sphereVertices[k+0] = sphereVertices[k - 3 + 0];
			sphereVertices[k+1] = sphereVertices[k - 3 + 1];
			sphereVertices[k+2] = sphereVertices[k - 3 + 2];
			k += 3;
			// A2		   
			sphereVertices[k+0] = sin(thetaR10)*cos(phiR);
			sphereVertices[k+1] = cos(thetaR10)*cos(phiR);
			sphereVertices[k+2] = sin(phiR);
			k += 3;
			// A1		   
			sphereVertices[k+0] = sphereVertices[k - 15 + 0];
			sphereVertices[k+1] = sphereVertices[k - 15 + 1];
			sphereVertices[k+2] = sphereVertices[k - 15 + 2];
			k += 3;
		}
	}
	assert(SPHERE_VEC * 3);
}

void setCloth(Cloth newCloth, unsigned int VAO_1, unsigned int VBO_1, unsigned int EBO)
{
	glBindVertexArray(VAO_1);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(newCloth.points[0])*newCloth.points.size(), &newCloth.points[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(newCloth.indexArray[0])*newCloth.indexArray.size(), &newCloth.indexArray[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(newCloth.points[0]), (void*)0);
}

void renderCloth(Cloth newCloth, unsigned int VAO_1, unsigned int VBO_1, unsigned int EBO)
{
	setCloth(newCloth, VAO_1, VBO_1, EBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(newCloth.points[0]), (void*)0);
	glDrawElements(GL_TRIANGLES, newCloth.indexArray.size(), GL_UNSIGNED_INT, 0);
}

void setSphere(unsigned int VAO_2)
{
	glBindVertexArray(VAO_2);
	glBindBuffer(GL_ARRAY_BUFFER, VAO_2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * SPHERE_VEC * 3, sphereVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
}

void renderSphere(unsigned int VAO_2)
{
	setSphere(VAO_2);
	glDrawArrays(GL_TRIANGLES, 0, SPHERE_VEC * 3);
}

// Define targa header. This is only used locally.
#pragma pack(1)
typedef struct
{
	GLbyte	identsize;              // Size of ID field that follows header (0)
	GLbyte	colorMapType;           // 0 = None, 1 = paletted
	GLbyte	imageType;              // 0 = none, 1 = indexed, 2 = rgb, 3 = grey, +8=rle
	unsigned short	colorMapStart;          // First colour map entry
	unsigned short	colorMapLength;         // Number of colors
	unsigned char 	colorMapBits;   // bits per palette entry
	unsigned short	xstart;                 // image x origin
	unsigned short	ystart;                 // image y origin
	unsigned short	width;                  // width in pixels
	unsigned short	height;                 // height in pixels
	GLbyte	bits;                   // bits per pixel (8 16, 24, 32)
	GLbyte	descriptor;             // image descriptor
} TGAHEADER;
#pragma pack(8)


////////////////////////////////////////////////////////////////////
// Capture the current viewport and save it as a targa file.
// Be sure and call SwapBuffers for double buffered contexts or
// glFinish for single buffered contexts before calling this function.
// Returns 0 if an error occurs, or 1 on success.
GLint gltWriteTGA(const char *szFileName)
{
	FILE *pFile;                // File pointer
	TGAHEADER tgaHeader;		// TGA file header
	unsigned long lImageSize;   // Size in bytes of image
	GLbyte	*pBits = NULL;      // Pointer to bits
	GLint iViewport[4];         // Viewport in pixels
	GLenum lastBuffer;          // Storage for the current read buffer setting

	// Get the viewport dimensions
	glGetIntegerv(GL_VIEWPORT, iViewport);

	// How big is the image going to be (targas are tightly packed)
	lImageSize = iViewport[2] * 3 * iViewport[3];

	// Allocate block. If this doesn't work, go home
	pBits = (GLbyte *)malloc(lImageSize);
	if (pBits == NULL)
		return 0;

	// Read bits from color buffer
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);

	// Get the current read buffer setting and save it. Switch to
	// the front buffer and do the read operation. Finally, restore
	// the read buffer state
	glGetIntegerv(GL_READ_BUFFER, (GLint *)&lastBuffer);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, iViewport[2], iViewport[3], GL_BGR, GL_UNSIGNED_BYTE, pBits);
	glReadBuffer(lastBuffer);

	// Initialize the Targa header
	tgaHeader.identsize = 0;
	tgaHeader.colorMapType = 0;
	tgaHeader.imageType = 2;
	tgaHeader.colorMapStart = 0;
	tgaHeader.colorMapLength = 0;
	tgaHeader.colorMapBits = 0;
	tgaHeader.xstart = 0;
	tgaHeader.ystart = 0;
	tgaHeader.width = iViewport[2];
	tgaHeader.height = iViewport[3];
	tgaHeader.bits = 24;
	tgaHeader.descriptor = 0;

	// Do byte swap for big vs little endian
#ifdef __APPLE__
	LITTLE_ENDIAN_WORD(&tgaHeader.colorMapStart);
	LITTLE_ENDIAN_WORD(&tgaHeader.colorMapLength);
	LITTLE_ENDIAN_WORD(&tgaHeader.xstart);
	LITTLE_ENDIAN_WORD(&tgaHeader.ystart);
	LITTLE_ENDIAN_WORD(&tgaHeader.width);
	LITTLE_ENDIAN_WORD(&tgaHeader.height);
#endif

	// Attempt to open the file
	pFile = fopen(szFileName, "wb");
	if (pFile == NULL)
	{
		free(pBits);    // Free buffer and return error
		return 0;
	}

	// Write the header
	fwrite(&tgaHeader, sizeof(TGAHEADER), 1, pFile);

	// Write the image data
	fwrite(pBits, lImageSize, 1, pFile);

	// Free temporary buffer and close the file
	free(pBits);
	fclose(pFile);

	// Success!
	return 1;
}