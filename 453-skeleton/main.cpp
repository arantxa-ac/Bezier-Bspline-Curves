#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <limits>
#include <functional>

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Texture.h"
#include "Window.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#define SELECT_RANGE 0.02

//state structure used for global variables
struct State {
	//scene number
	int scene = 1;
	//mouse variables
	glm::vec2 mouseCoord;
	glm::vec2 mouseDownCoord;
	glm::vec2 mouseUpCoord;
	float fov = 90;
	bool mouseClicked = false;
	bool mouseReleased = false;
	bool mouseHeld = false;
	//state variables
	bool stateChanged = true;
	bool resetScreen = false;
	//camera variables
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -0.1f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 0.1f, 0.0f);
	const float cameraSpeed = 0.05f;
};

void updateGPUGeometry(GPU_Geometry &gpuGeom, CPU_Geometry const &cpuGeom) {
	gpuGeom.bind();
	gpuGeom.setVerts(cpuGeom.verts);
	gpuGeom.setCols(cpuGeom.cols);
}

// EXAMPLE CALLBACKS
class Assignment3 : public CallbackInterface {

public:
	Assignment3(int screenWidth, int screenHeight):
		screenDimensions(screenWidth, screenHeight)
	{
	}

	virtual void keyCallback(int key, int scancode, int action, int mods) {
		//key R used to reset the points in the screen
		if (key == GLFW_KEY_R && action == GLFW_PRESS) {
			state.cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
			state.cameraFront = glm::vec3(0.0f, 0.0f, -0.1f);
			state.cameraUp = glm::vec3(0.0f, 0.1f, 0.0f);
			state.resetScreen = true;
		}
		//scene controls
		else if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
			state.scene = 1;
		}
		else if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
			state.scene = 2;
		}
		else if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
			state.scene = 3;
		}
		else if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
			state.scene = 4;
		}
		else if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
			state.scene = 5;
		}
		else if (key == GLFW_KEY_6 && action == GLFW_PRESS) {
			state.scene = 6;
		}
		//move controls
		else if (key == GLFW_KEY_W && action == GLFW_PRESS && state.scene >= 3) {
			state.cameraPos += state.cameraSpeed * state.cameraFront;
		}
		else if (key == GLFW_KEY_S && action == GLFW_PRESS && state.scene >= 3) {
			state.cameraPos -= state.cameraSpeed * state.cameraFront;
		}
		else if (key == GLFW_KEY_A && action == GLFW_PRESS && state.scene >= 3) {
			state.cameraPos -= glm::normalize(glm::cross(state.cameraFront, state.cameraUp)) * state.cameraSpeed;
		}
		else if (key == GLFW_KEY_D && action == GLFW_PRESS && state.scene >= 3) {
			state.cameraPos += glm::normalize(glm::cross(state.cameraFront, state.cameraUp)) * state.cameraSpeed;
		}
		state.stateChanged = true;
	}

	virtual void mouseButtonCallback(int button, int action, int mods) {
		//when mouse buttons are pressed
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			state.mouseDownCoord = state.mouseCoord;
			state.mouseClicked = true;
			state.mouseHeld = true;
			std::cout << "left mouse button pressed" << std::endl;
		}
		else if (button = GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
			std::cout << "right mouse button pressed" << std::endl;
		}
		//when mouse buttons are released
		else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
			state.mouseUpCoord = state.mouseCoord;
			state.mouseHeld == false;
			state.mouseReleased = true;
			std::cout << "left mouse button released" << std::endl;
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
			std::cout << "right mouse button released" << std::endl;
		}
		state.stateChanged = true;
	}

	virtual void cursorPosCallback(double xpos, double ypos) {
		state.mouseCoord = glm::vec2(xpos, ypos);
		state.mouseCoord = state.mouseCoord / (screenDimensions - 1.f);
		state.mouseCoord *= 2;
		state.mouseCoord -= 1.f;
		state.mouseCoord.y = -state.mouseCoord.y;
		//std::cout << state.mouseCoord.x << " " << state.mouseCoord.y << std::endl;
		state.stateChanged = true;
	}

	virtual void scrollCallback(double xoffset, double yoffset) {
	}

	virtual void windowSizeCallback(int width, int height) {
		// The CallbackInterface::windowSizeCallback will call glViewport for us
		CallbackInterface::windowSizeCallback(width,  height);
	}

	bool stateHasChanged() {
		return state.stateChanged;
	}

	void stateHandled() {
		state.stateChanged = false;
		state.mouseClicked = false;
		state.mouseReleased = false;
		state.resetScreen = false;
	}

	const State& getState() {
		return state;
	}

	void setCameraFront(glm::vec3 cameraFront) {
		state.cameraFront = cameraFront;
	}

private:
	State state;
	glm::vec2 screenDimensions;
};

//BEZIER FUNCTION
//Returns the bezier curve of the given points in a CPU_Geometry
CPU_Geometry bezier(CPU_Geometry points, int d) {
	CPU_Geometry bezierCurve;
	//pass the points to a vector
	std::vector<glm::vec3> P;
	for (glm::vec3 point : points.verts) {
		P.push_back(point);
	}
	//DeCasteljau algorithm
	for (float u = 0.0; u < 1.0; u += 0.01) {
		for (int i = 0; i < d; i++) {
			for (int j = 0; j < (d - 1); j++) {
				P[j] = (1 - u) * P[j] + u * P[j + 1];
			}
		}
		bezierCurve.verts.push_back(P[0]);
	}
	//set color for the bezier curve
	bezierCurve.cols.resize(bezierCurve.verts.size(), glm::vec3{ 0.0, 0.0, 0.0 });
	return bezierCurve;
}

//BSLPINE FUNCTION
//Returns the b-spline curve of the given points in a CPU_Geometry
CPU_Geometry bspline(std::vector<glm::vec3> C, int n) {
	CPU_Geometry bsplineCurve;
	int N = n;
	//loop to repeat algorithm multiple times
	int k = 0;
	while (k < 5) {
		std::vector<glm::vec3> F(2*N);
		F[0] = C[0];
		F[1] = 0.5f * C[0] + 0.5f * C[1];
		for (int i = 1; i <= N - 2; i+= 1) {
			F[2*i] = 0.75f * C[i] + 0.25f * C[i + 1];
			F[2*i + 1] = 0.25f * C[i] + 0.75f * C[i + 1];
		}
		F[2 * N - 2] = 0.5f * C[N - 2] + 0.5f * C[N - 1];
		F[2 * N - 1] = C[N - 1];
		//update variables for next loop iteration
		C.clear();
		for (glm::vec3 fpoint : F) {
			C.push_back(fpoint);
		}
		N *= 2;
		k++;
	}
	
	//add calculated points to the cpu geom
	for (glm::vec3 fpoint : C)
		bsplineCurve.verts.push_back(fpoint);
	//set color for the bspline curve
	bsplineCurve.cols.resize(bsplineCurve.verts.size(), glm::vec3{ 0.0, 0.0, 0.0 });
	return bsplineCurve;
}

//HAS SELECTED FUNCTION
//Checks if the given mouse coordinate of a click is selecting a point to move
bool hasSelected(CPU_Geometry points, glm::vec2 mouseCoord, glm::vec3& selectedPoint) {
	for (glm::vec3 point : points.verts) {
		if ((mouseCoord.x > point.x - SELECT_RANGE) && (mouseCoord.x < point.x + SELECT_RANGE)
			&& (mouseCoord.y > point.y - SELECT_RANGE) && (mouseCoord.y < point.y + SELECT_RANGE)) {
			selectedPoint = point;
			return true;
		}
	}
	return false;
}

//MAKE VIEWING MATRIX FUNCTION
//Returns the viewing matrix
//code retrieved from https://learnopengl.com/Getting-started/Camera
glm::mat4 makeViewingMatrix(State state) {
	//camera position and direction
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraDirection = glm::normalize(state.cameraPos - cameraTarget);
	//right axis
	glm::vec3 up = glm::vec3(0.0f, 0.1f, 0.0f);
	glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
	//up axis
	glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);

	glm::mat4 view;
	//if the scene is 1 or 2 the matrix should not be modified from the original
	if (state.scene <= 2) {
		view = glm::lookAt(cameraPos, cameraTarget, up);
	}
	//if the scene is 3 4 or 5 then change the viewing matrix based on input
	else {
		//set view matrix
		view = glm::lookAt(state.cameraPos, state.cameraPos + state.cameraFront, state.cameraUp);
	}
	return view;
}

//SURFACE OF REVOLUTION FUNCTION
//Returns the surface of revolution of a curve in a CPU geometry
CPU_Geometry surfaceOfRevolution(std::vector<glm::vec3> C, int n) {
	CPU_Geometry srfRev;
	for (int u = 0; u < n; u++) {
		for (float v = 0; v < (2 * glm::pi<float>()); v += 0.1) {
			srfRev.verts.push_back(glm::vec3{ C[u].x * cos(v), C[u].y, C[u].x * sin(v) });
		}
	}
	srfRev.cols.resize(srfRev.verts.size(), glm::vec3{ 0.f, 1.f, 0.9f });
	return srfRev;
}

//MAIN FUNCTION
int main() {
	Log::debug("Starting main");

	// WINDOW
	glfwInit();
	int screenWidth = 800;
	int screenHeight = 800;
	Window window(screenWidth, screenHeight, "CPSC 453"); // can set callbacks at construction if desired


	GLDebug::enable();

	// CALLBACKS
	auto a3 = std::make_shared<Assignment3>(screenWidth, screenHeight);
	window.setCallbacks(a3);


	ShaderProgram shader("shaders/test.vert", "shaders/test.frag");

	// set starting points
	CPU_Geometry square;
	square.verts.push_back(glm::vec3{-0.5, 0.5, 0});
	square.verts.push_back(glm::vec3{-0.5, -0.5, 0});
	square.verts.push_back(glm::vec3{0.5, -0.5, 0});
	square.verts.push_back(glm::vec3{0.5, 0.5, 0});
	square.cols.resize(square.verts.size(), glm::vec3{1.0, 0.0, 0.0});
	GPU_Geometry pointsGPUGeom;
	updateGPUGeometry(pointsGPUGeom, square);

	// Reset the colors to green
	square.cols.clear();
	square.cols.resize(square.verts.size(), glm::vec3{0.0, 1.0, 0.0});

	// set the gpu geom for the lines
	GPU_Geometry linesGPUGeom;
	updateGPUGeometry(linesGPUGeom, square);

	// set the bezier curve
	CPU_Geometry bezierCurve = bezier(square, square.verts.size());
	GPU_Geometry bezierGPUGeom;
	updateGPUGeometry(bezierGPUGeom, bezierCurve);

	//set the bspline curve
	CPU_Geometry bsplineCurve = bspline(square.verts, square.verts.size());
	GPU_Geometry bsplineGPUGeom;
	updateGPUGeometry(bsplineGPUGeom, bsplineCurve);

	//set the surface of revolution
	CPU_Geometry surfOfRev = surfaceOfRevolution(bsplineCurve.verts, bsplineCurve.verts.size());
	GPU_Geometry revolutionGPUGeom;
	updateGPUGeometry(revolutionGPUGeom, surfOfRev);

	glPointSize(10.0f);

	//set state
	const State& state = a3->getState();

	//variables for while loop
	bool pointRelocated = true;
	glm::vec3 selectedPoint;
	float yaw = -90.f;
	float pitch = 0;
	float lastX = 0;
	float lastY = 0;
	float xoffset, yoffset;
	const float sensitivity = 50.f;
	bool firstClick = true;

	// Note this call only work on some systems, unfortunately.
	// In order for them to work, you have to comment out line 60
	// If you're on a mac, you can't comment out line 60, so you
	// these will have no effect. :(
	// glLineWidth(5.0f);

	// RENDER LOOP
	while (!window.shouldClose()) {
		glm::mat4 viewing = makeViewingMatrix(state);
		auto viewMat = glGetUniformLocation(shader, "V");
		glUniformMatrix4fv(viewMat, 1, GL_FALSE, glm::value_ptr(viewing));

		glm::mat4 projection = glm::perspective(glm::radians(state.fov), (float)screenWidth / (float)screenHeight, 0.1f, 100.f);
		auto projMat = glGetUniformLocation(shader, "P");
		glUniformMatrix4fv(projMat, 1, GL_FALSE, glm::value_ptr(projection));

		glfwPollEvents();

		//check for events in scenes 1 and 2
		if (a3->stateHasChanged() && (state.scene <= 2)) {
			//check for reset button
			if (state.resetScreen) {
				//delete vertices and colors
				square.verts.clear();
				square.cols.clear();
				updateGPUGeometry(pointsGPUGeom, square);
				updateGPUGeometry(linesGPUGeom, square);
				//delete bezier curve
				bezierCurve.verts.clear();
				bezierCurve.cols.clear();
				updateGPUGeometry(bezierGPUGeom, bezierCurve);
				//delete bspline curve
				bsplineCurve.verts.clear();
				bsplineCurve.cols.clear();
				updateGPUGeometry(bsplineGPUGeom, bsplineCurve);
				//delete surface of revolution
				surfOfRev.verts.clear();
				surfOfRev.cols.clear();
				updateGPUGeometry(revolutionGPUGeom, surfOfRev);
			}
			//check for click
			else if (state.mouseClicked) {
				//if click is selecting a point and it has not been handled
				if (hasSelected(square, state.mouseDownCoord, selectedPoint) && pointRelocated) {
					pointRelocated = false;
				}
				//else add a point and color
				else {
					square.verts.push_back(glm::vec3{ state.mouseCoord, 0.f });
					square.cols.clear();
					square.cols.resize(square.verts.size(), glm::vec3{ 1.0, 0.0, 0.0 });
					updateGPUGeometry(pointsGPUGeom, square);
					//reset colors to green
					square.cols.clear();
					square.cols.resize(square.verts.size(), glm::vec3{ 0.0, 1.0, 0.0 });
					updateGPUGeometry(linesGPUGeom, square);
					//update bezier curve
					bezierCurve.verts.clear();
					bezierCurve.cols.clear();
					bezierCurve = bezier(square, square.verts.size());
					updateGPUGeometry(bezierGPUGeom, bezierCurve);
					//update bspline curve
					if (square.verts.size() > 2) {
						bsplineCurve.verts.clear();
						bsplineCurve.cols.clear();
						bsplineCurve = bspline(square.verts, square.verts.size());
						updateGPUGeometry(bsplineGPUGeom, bsplineCurve);
					}
					//update surface of revolution
					surfOfRev.verts.clear();
					surfOfRev.cols.clear();
					surfOfRev = surfaceOfRevolution(bsplineCurve.verts, bsplineCurve.verts.size());
					updateGPUGeometry(revolutionGPUGeom, surfOfRev);
				}
			}
			//check for click release
			else if (state.mouseReleased) {
				if (!pointRelocated) {
					std::cout << "selected point: " << selectedPoint << std::endl;
					for (int i = 0; i < square.verts.size(); i++) {
						if ((selectedPoint.x > square.verts[i].x - SELECT_RANGE) && (selectedPoint.x < square.verts[i].x + SELECT_RANGE) &&
							(selectedPoint.y > square.verts[i].y - SELECT_RANGE) && (selectedPoint.y < square.verts[i].y + SELECT_RANGE)) {
							square.verts[i] = glm::vec3{ state.mouseUpCoord, 0.f };
							square.cols.clear();
							square.cols.resize(square.verts.size(), glm::vec3{ 1.0, 0.0, 0.0 });
							updateGPUGeometry(pointsGPUGeom, square);
							square.cols.clear();
							square.cols.resize(square.verts.size(), glm::vec3{ 0.0, 1.0, 0.0 });
							updateGPUGeometry(linesGPUGeom, square);
							bezierCurve.verts.clear();
							bezierCurve.cols.clear();
							bezierCurve = bezier(square, square.verts.size());
							updateGPUGeometry(bezierGPUGeom, bezierCurve);
							bsplineCurve.verts.clear();
							bsplineCurve.cols.clear();
							bsplineCurve = bspline(square.verts, square.verts.size());
							updateGPUGeometry(bsplineGPUGeom, bsplineCurve);
							surfOfRev.verts.clear();
							surfOfRev.cols.clear();
							surfOfRev = surfaceOfRevolution(bsplineCurve.verts, bsplineCurve.verts.size());
							updateGPUGeometry(revolutionGPUGeom, surfOfRev);
						}
					}
					pointRelocated = true;
				}
			}
			//check for mouse drag
			else if (state.mouseHeld) {
				//change color of selected point
				if (!pointRelocated) {
					for (int i = 0; i < square.verts.size(); i++) {
						if ((selectedPoint.x > square.verts[i].x - SELECT_RANGE) && (selectedPoint.x < square.verts[i].x + SELECT_RANGE) &&
							(selectedPoint.y > square.verts[i].y - SELECT_RANGE) && (selectedPoint.y < square.verts[i].y + SELECT_RANGE)) {
							square.cols.clear();
							square.cols.resize(square.verts.size(), glm::vec3{ 1.0, 0.0, 0.0 });
							square.cols[i] = glm::vec3{ 0.0, 0.0, 1.0 };
							updateGPUGeometry(pointsGPUGeom, square);
						}
					}
				}
			}
			a3->stateHandled();
		}
		//mouse movement for scenes 3, 4 and 5
		else if (a3->stateHasChanged() && (state.scene >= 3)) {
			if (state.resetScreen) {
				yaw = -90.f;
				pitch = 0;
				lastX = 0;
				lastY = 0;
			}
			else if (state.mouseClicked && firstClick) {
				lastX = 0;
				lastY = 0;
				firstClick = false;
			}
			else if (state.mouseClicked) {
				firstClick = true;
			}
			else if (!firstClick) {
				xoffset = state.mouseCoord.x - lastX;
				yoffset = state.mouseCoord.y - lastY;
				lastX = state.mouseCoord.x;
				lastY = state.mouseCoord.y;
				xoffset *= sensitivity;
				yoffset *= sensitivity;
				yaw += xoffset;
				pitch += yoffset;
				if (pitch > 89.f)
					pitch = 89.f;
				if (pitch < -89.f)
					pitch = -89.f;
				glm::vec3 direction;
				direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
				direction.y = sin(glm::radians(pitch));
				direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
				a3->setCameraFront(glm::normalize(direction));
			}
			a3->stateHandled();
		}

		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();

		if (state.scene < 5) {
			linesGPUGeom.bind();
			glDrawArrays(GL_LINE_STRIP, 0, GLsizei(square.verts.size()));
			pointsGPUGeom.bind();
			glDrawArrays(GL_POINTS, 0, GLsizei(square.verts.size()));
		}

		//if statements for type of line: bezier/b-spline
		if (state.scene == 1 || state.scene == 3) {
			bezierGPUGeom.bind();
			glDrawArrays(GL_LINE_STRIP, 0, GLsizei(bezierCurve.verts.size()));
		}
		else if (state.scene == 2 || state.scene == 4) {
			bsplineGPUGeom.bind();
			glDrawArrays(GL_LINE_STRIP, 0, GLsizei(bsplineCurve.verts.size()));
		}
		else if (state.scene == 5) {
			revolutionGPUGeom.bind();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawArrays(GL_POINTS, 0, GLsizei(surfOfRev.verts.size()));
		}
		else if (state.scene == 6) {
			revolutionGPUGeom.bind();
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawArrays(GL_LINE_STRIP, 0, GLsizei(surfOfRev.verts.size()));
		}

		glDisable(GL_FRAMEBUFFER_SRGB); // disable sRGB for things like imgui

		window.swapBuffers();
	}

	glfwTerminate();
	return 0;
}
