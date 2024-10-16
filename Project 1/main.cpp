#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cfloat>
#include <GL/glew.h>
#include <GL/glut.h>

#include "utility.h"

#define _USE_MATH_DEFINES
#include <math.h>

// Input data
std::string filePath = "Data/";
std::string fileName1 = "5.dat";
std::string fileName2 = "18.dat";
float* dataset = nullptr;
int numDataPoints;
float minimum, maximum;

// Histogram
int numIntervals = 30;
float* endPoints = nullptr;
float* prob = nullptr;
float maxProb = -1;

// Theoretical distributions
int curveType = 0;
int numCurvePoints = 100;
float* curveX = new float[numCurvePoints];
float* curveY = new float[numCurvePoints];
float maxCurveY = -1;

// Parameters
float mu = 0, sigma = 1; // Normal distribution
float lambda = 1; // Exponential distribution
float parameterStep = 0.05; // Step size for changing parameter values

// Drawing parameters
int width = 800, height = 600;
float world_x_min, world_x_max, world_y_min, world_y_max;
float axis_x_min, axis_x_max, axis_y_min, axis_y_max;

// Compute all the points for normal distribution
void computeNormalFunc(float mu, float sigma) {
	// Determine the step size and compute the arrays curveX and curveY
	// (numCurvePoints).
	float xRange = maximum - minimum;
	float xStep = xRange / (numCurvePoints - 1);

	maxCurveY = -FLT_MAX;

	for (int i = 0; i < numCurvePoints; ++i) {
		curveX[i] = minimum + i * xStep;
		float exponent = -0.5f * pow((curveX[i] - mu) / sigma, 2);
		curveY[i] = (1.0f / (sigma * sqrt(2 * M_PI))) * exp(exponent);

		if (curveY[i] > maxCurveY) {
			maxCurveY = curveY[i];
		}

	}
}

// Compute all the points for exponential distribution
void computeExponentialFunc(float lambda) {
	// Determine the step size and compute the arrays curveX and curveY
	// (numCurvePoints).
	float xRange = maximum - minimum;
	float xStep = xRange / (numCurvePoints - 1);

	maxCurveY = -FLT_MAX;

	for (int i = 0; i < numCurvePoints; ++i) {
		curveX[i] = minimum + i * xStep;
		curveY[i] = lambda * exp(-lambda * (curveX[i] - minimum));

		if (curveY[i] > maxCurveY) {
			maxCurveY = curveY[i];
		}
	}
}

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glLineWidth(2.0f);

	// Draw x and y axes
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
		glVertex2f(0.05f, 0.05f);
		glVertex2f(0.95f, 0.05f); // Extend to the right (positive X)

		glVertex2f(0.05f, 0.05f);
		glVertex2f(0.05f, 0.95f); // Extend upwards (positive Y)
	glEnd();

	printString(0.90f, 0.02f, "Data"); // X
	printString(0.06f, 0.93f, "Probability Density"); // Y

	glColor3f(0.2f, 0.7f, 0.3f);
	printString(0.70f, 0.93f, "File: " + fileName1);
	printString(0.70f, 0.90f, "Min: " + std::to_string(minimum));
	printString(0.70f, 0.87f, "Max: " + std::to_string(maximum));
	printString(0.70f, 0.84f, "Num of Intervals: " + std::to_string(numIntervals));

	float globalMaxY = (maxProb > maxCurveY) ? maxProb : maxCurveY;

	// Calculate bar dimensions
	float xStart = 0.1f; // Start drawing after some margin
	float xEnd = 0.9f;
	float plotWidth = xEnd - xStart;
	float barWidth = plotWidth / numIntervals;

	for (int i = 0; i < numIntervals; ++i) {
		float x = xStart + i * barWidth;
		float y = 0.05f; // Bottom margin
		float barHeight = (prob[i] / maxProb) * 0.8f; // Scale to fit within the plot area

		// Draw the bars
		glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + barWidth, y);
			glVertex2f(x + barWidth, y + barHeight);
			glVertex2f(x, y + barHeight);
		glEnd();
	}

	// Draw the theoretical curve
	glColor3f(1.0f, 0.2f, 0.2f);
	if (curveType == 0) {
		printString(0.70f, 0.78f, "Distribution: Normal");
	} else {
		printString(0.70f, 0.78f, "Distribution: Exponential");
	}

	std::stringstream ssMu, ssSigma;
	ssMu << std::fixed << std::setprecision(2) << mu;
	ssSigma << std::fixed << std::setprecision(2) << sigma;

	printString(0.70f, 0.75f, "Mu: " + ssMu.str());
	printString(0.70f, 0.72f, "Sigma: " + ssSigma.str());

	glLineWidth(2.0f);

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < numCurvePoints; ++i) {
		// Map curveX and curveY to the plotting area		
		float x = xStart + ((curveX[i] - minimum) / (maximum - minimum)) * plotWidth;
		float y = 0.1f + (curveY[i] / globalMaxY) * 0.8f; // Scale and position within the plot area
		glVertex2f(x, y);
	}
	glEnd();

	// Display the maximum probability value
	// Draw probability histogram
	// Draw the theoretical distribution using thicker lines.
	// Compute the tbarWidthop-left position of the annotation
	// Draw theoretical distributions
	glFlush();
	glutSwapBuffers();
}

void init(void) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

// Compute the probability for the histogram (vertical axis)
void computeProbability(int numIntervals) {
	if (endPoints != nullptr) {
		delete[] endPoints;
		endPoints = nullptr;
	}
	if (prob != nullptr) {
		delete[] prob;
		prob = nullptr;
	}
	endPoints = new float[numIntervals + 1];
	prob = new float[numIntervals];
	maxProb = -1;

	// Determine the endpoints for each interval (bins)
	float binWidth = (maximum - minimum) / numIntervals;
	for (int i = 0; i <= numIntervals; ++i) {
		endPoints[i] = minimum + i * binWidth;
	}

	// Initialize the probabilities to zero
	for (int i = 0; i < numIntervals; ++i) {
		prob[i] = 0;
	}

	// Count data points in each bin
	for (int i = 0; i < numDataPoints; ++i) {
		float dataValue = dataset[i];
		int binIndex = (int)((dataValue - minimum) / binWidth);
		if (binIndex == numIntervals) {
			binIndex = numIntervals - 1; // Edge case for maximum value
		}
		prob[binIndex] += 1.0f;
	}

	// Normalize probabilities and find the maximum probability
	for (int i = 0; i < numIntervals; ++i) {
		prob[i] /= numDataPoints;
		if (prob[i] > maxProb) {
			maxProb = prob[i];
		}
	}
}

void readFile(std::string fileName) {
	std::ifstream inFile(fileName);
	if (!inFile.is_open()) {
		std::cout << fileName << " couldn't be opened.\n";
		system("pause");
		exit(1);
	}
	inFile >> numDataPoints;

	dataset = new float[numDataPoints];
	minimum = FLT_MAX;
	maximum = -FLT_MAX;

	// Read the data and compute minimum and maximum.
	for (int i = 0; i < numDataPoints; i++) {
		inFile >> dataset[i];
		if (dataset[i] < minimum)
			minimum = dataset[i];
		if (dataset[i] > maximum)
			maximum = dataset[i];
	}

	std::cout << "Number of Data Points: " << numDataPoints << std::endl;
	std::cout << "Minimum: " << minimum << std::endl;
	std::cout << "Maximum: " << maximum << std::endl;
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
		case 'q':
		case 'Q':
		case 27: // ESC key
			exit(0);
			break;
		case '+':
			numIntervals += 5;
			break;
		case '-':
			if (numIntervals > 5) {
				numIntervals -= 5;
			}
			break;
	}

	computeProbability(numIntervals);
	glutPostRedisplay();
}

// Update the parameters and theoretical distributions
void specialKey(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_LEFT:
		mu -= parameterStep;
		break;
	case GLUT_KEY_RIGHT:
		mu += parameterStep;
		break;
	case GLUT_KEY_UP:
		sigma += parameterStep;
		break;
	case GLUT_KEY_DOWN:
		sigma -= parameterStep;
		if (sigma <= 0) sigma = parameterStep;
		break;
	}
	computeNormalFunc(mu, sigma);
	glutPostRedisplay();
}

void topMenuFunc(int id) {
	if (id == 0) {
		exit(0);
	}
}

// Read file.
// Update projection since the data has changed.
void fileMenuFunction(int id) {
	switch (id) {
		case 1:
			fileName1 = "normal.dat";
			break;
		case 2:
			fileName1 = "expo.dat";
			break;
		case 3:
			fileName1 = "5.dat";
			break;
		case 4:
			fileName1 = "18.dat";
			break;
	}
	readFile(filePath + fileName1);
	computeProbability(numIntervals);

	if (curveType == 0) {
		computeNormalFunc(mu, sigma);
	} else if (curveType == 1) {
		computeExponentialFunc(lambda);
	}

	glutPostRedisplay();
}

// Update curveType based on the menu selection and recompute the theoretical distribution.
void funcMenuFunction(int id) {
	switch (id) {
		case 1: 
			curveType = 0;
			break;
		case 2:
			curveType = 1;
			break;
	}

	if (curveType == 0) {
		computeNormalFunc(mu, sigma);
	} else if (curveType == 1) {
		computeExponentialFunc(lambda);
	}

	glutPostRedisplay();
}

// Update the number of intervals and recomputed the histogram.
// Update projection since the histogram has changed due to the change of number
// of bars.
void histogramMenuFunction(int id) {
	switch (id) {
		case 1:
			numIntervals = 30;
			break;
		case 2:
			numIntervals = 40;
			break;
		case 3:
			numIntervals = 50;
			break;
	}

	computeProbability(numIntervals);
	glutPostRedisplay();
}

// Update the parameter step size.
void parameterStepMenuFunction(int id) {
	switch (id) {
		case 1:
			parameterStep = 0.01;
			break;
		case 2:
			parameterStep = 0.02;
			break;
		case 3:
			parameterStep = 0.05;
			break;
	}
}

void createMenu() {
	int fileMenu = glutCreateMenu(fileMenuFunction);
	glutAddMenuEntry("normal.dat", 1);
	glutAddMenuEntry("expo.dat", 2);
	glutAddMenuEntry("File 1", 3);
	glutAddMenuEntry("File 2", 4);

	int distributionMenu = glutCreateMenu(funcMenuFunction);
	glutAddMenuEntry("Normal", 1);
	glutAddMenuEntry("Exponential", 2);

	int histogramMenu = glutCreateMenu(histogramMenuFunction);
	glutAddMenuEntry("30", 1);
	glutAddMenuEntry("40", 2);
	glutAddMenuEntry("50", 3);

	int parameterStepMenu = glutCreateMenu(parameterStepMenuFunction);
	glutAddMenuEntry("0.01", 1);
	glutAddMenuEntry("0.02", 2);
	glutAddMenuEntry("0.05", 3);

	int mainMenu = glutCreateMenu(topMenuFunc);
	glutAddSubMenu("File", fileMenu);
	glutAddSubMenu("Distribution", distributionMenu);
	glutAddSubMenu("Histogram", histogramMenu);
	glutAddSubMenu("Parameter Step", parameterStepMenu);
	glutAddMenuEntry("Exit", 0);

	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void reshape(int w, int h) {
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Setup a 2D orthographic projection (left, right, bottom, top)
	gluOrtho2D(0.0, 1.0, 0.0, 1.0); // Bottom-left corner is (0,0), top-right is (1,1)

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

int main(int argc, char** argv) {
	dataset = nullptr;
	endPoints = nullptr;
	prob = nullptr;

	readFile(filePath + fileName1);
	computeProbability(numIntervals);

	if (curveType == 0) {
		computeNormalFunc(mu, sigma);
	} else if (curveType == 1) {
		computeExponentialFunc(lambda);
	}

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Input Analysis");

	glewInit();
	init();

	glEnable(GL_DEPTH_TEST);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKey);

	createMenu();

	glutMainLoop();

	if (dataset != nullptr) delete[] dataset;
	if (endPoints != nullptr) delete[] endPoints;
	if (prob != nullptr) delete[] prob;
	if (curveX != nullptr) delete[] curveX;
	if (curveY != nullptr) delete[] curveY;

	return 0;
}