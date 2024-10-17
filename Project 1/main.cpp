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

/**
 * @brief Path to the data directory.
 */
std::string filePath = "Data/";

/**
 * @brief Name of the data file to read.
 */
std::string fileName = "5.dat";

/**
 * @brief Pointer to the dataset array.
 */
float* dataset = nullptr;

/**
 * @brief Number of data points in the dataset.
 */
int numDataPoints;

/**
 * @brief Minimum value in the dataset.
 */
float minimum;

/**
 * @brief Maximum value in the dataset.
 */
float maximum;

/**
 * @brief Number of intervals (bins) in the histogram.
 */
int numIntervals = 30;

/**
 * @brief Endpoints of the histogram intervals.
 */
float* endPoints = nullptr;

/**
 * @brief Probabilities for each histogram interval.
 */
float* prob = nullptr;

/**
 * @brief Maximum probability value in the histogram.
 */
float maxProb = -1;

/**
 * @brief Type of the theoretical distribution curve (0 for normal, 1 for exponential).
 */
int curveType = 0;

/**
 * @brief Number of points to compute in the theoretical curve.
 */
int numCurvePoints = 100;

/**
 * @brief X-values of the theoretical curve.
 */
float* curveX = new float[numCurvePoints];

/**
 * @brief Y-values of the theoretical curve.
 */
float* curveY = new float[numCurvePoints];

/**
 * @brief Maximum Y-value in the theoretical curve.
 */
float maxCurveY = -1;

/**
 * @brief Mean (mu) parameter for the normal distribution.
 */
float mu = 0;

/**
 * @brief Standard deviation (sigma) parameter for the normal distribution.
 */
float sigma = 1;

/**
 * @brief Lambda parameter for the exponential distribution.
 */
float lambda = 1;

/**
 * @brief Step size for changing parameter values.
 */
float parameterStep = 0.05;

/**
 * @brief Window width.
 */
int width = 800;

/**
 * @brief Window height.
 */
int height = 800;

/**
 * @brief Display callback function for rendering the histogram and theoretical distribution.
 */
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

	glColor3f(0.05f, 0.57f, 0.96f);
	printString(0.90f, 0.02f, "Data"); // X
	printString(0.06f, 0.93f, "Probability Density"); // Y
	printString(0.06f, 0.89f, std::to_string(maxCurveY));

	printString(0.70f, 0.93f, "File: " + fileName);
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
		glColor3f(0.98f, 0.33f, 0.33f);
		glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + barWidth, y);
			glVertex2f(x + barWidth, y + barHeight);
			glVertex2f(x, y + barHeight);
		glEnd();
	}

	// Draw the theoretical curve
	glColor3f(0.78f, 0.18f, 0.18f);
	if (curveType == 0) {
		printString(0.70f, 0.78f, "Distribution: Normal");
	} else {
		printString(0.70f, 0.78f, "Distribution: Exponential");
	}

	std::stringstream ssMu, ssSigma, ssBeta;
	ssMu << std::fixed << std::setprecision(2) << mu;
	ssSigma << std::fixed << std::setprecision(2) << sigma;
	ssBeta << std::fixed << std::setprecision(2) << 1 / lambda;

	if (fileName != "expo.dat") {
		printString(0.70f, 0.75f, "Mu: " + ssMu.str());
		printString(0.70f, 0.72f, "Sigma: " + ssSigma.str());
	} else {
		printString(0.70f, 0.75f, "Beta: " + ssBeta.str());
	}

	glLineWidth(2.0f);

	glColor3f(0.47f, 0.80f, 1.0f);
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < numCurvePoints; ++i) {
		// Map curveX and curveY to the plotting area		
		float x = xStart + ((curveX[i] - minimum) / (maximum - minimum)) * plotWidth;
		float y = 0.1f + (curveY[i] / globalMaxY) * 0.8f; // Scale and position within the plot area
		glVertex2f(x, y);
	}
	glEnd();

	// Compute the tbarWidthop-left position of the annotation
	glFlush();
	glutSwapBuffers();
}

/**
 * @brief Initializes OpenGL settings.
 */
void init(void) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

/**
 * @brief Computes the probabilities for the histogram based on the dataset and number of intervals.
 * @param numIntervals The number of intervals (bins) in the histogram.
 */
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

/**
 * @brief Computes the points for the normal distribution curve.
 * @param mu The mean value of the normal distribution.
 * @param sigma The standard deviation of the normal distribution.
 */
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

/**
 * @brief Computes the points for the exponential distribution curve.
 * @param lambda The lambda parameter of the exponential distribution.
 */
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

/**
 * @brief Computes the mean (average) of the given data array.
 * @param data Pointer to the data array.
 * @param size Number of elements in the data array.
 * @return The computed mean value.
 */
float computeMean(float* data, int size) {
	float sum = 0.0f;
	for (int i = 0; i < size; ++i) {
		sum += data[i];
	}
	return sum / size;
}

/**
 * @brief Computes the standard deviation of the given data array.
 * @param data Pointer to the data array.
 * @param size Number of elements in the data array.
 * @param mean The mean value of the data array.
 * @return The computed standard deviation.
 */
float computeStandardDeviation(float* data, int size, float mean) {
	float sum = 0.0f;
	for (int i = 0; i < size; ++i) {
		sum += pow(data[i] - mean, 2);
	}
	return sqrt(sum / size);
}

/**
 * @brief Reads the dataset from a file and computes the minimum, maximum, mean, and standard deviation.
 * @param fileName The name of the file to read.
 */
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

	mu = computeMean(dataset, numDataPoints);
	sigma = computeStandardDeviation(dataset, numDataPoints, mu);

	if (fileName == filePath + "expo.dat") {
		lambda = 1.0 / mu;
		std::cout << "Lambda: " << lambda << std::endl;
	}

	std::cout << "Number of Data Points: " << numDataPoints << std::endl;
	std::cout << "Minimum: " << minimum << std::endl;
	std::cout << "Maximum: " << maximum << std::endl;
	std::cout << "Mean: " << mu << std::endl;
	std::cout << "Standard Deviation: " << sigma << std::endl;
}

/**
 * @brief Keyboard callback function to handle standard key presses.
 * @param key The key that was pressed.
 * @param x The x-coordinate of the mouse when the key was pressed.
 * @param y The y-coordinate of the mouse when the key was pressed.
 */
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
			if (numIntervals > 5)
				numIntervals -= 5;
			break;
	}

	computeProbability(numIntervals);
	glutPostRedisplay();
}

/**
 * @brief Special keyboard callback function to handle special key presses (e.g., arrow keys).
 * @param key The special key that was pressed.
 * @param x The x-coordinate of the mouse when the key was pressed.
 * @param y The y-coordinate of the mouse when the key was pressed.
 */
void specialKey(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_LEFT:
		mu -= parameterStep;
		if (mu < 0.0) mu = 0.0;
		break;
	case GLUT_KEY_RIGHT:
		mu += parameterStep;
		if (mu >= 5.0) mu = 4.99;
		break;
	case GLUT_KEY_UP:
		sigma += parameterStep;
		if (sigma >= 3.0) sigma = 2.99;
		break;
	case GLUT_KEY_DOWN:
		sigma -= parameterStep;
		if (sigma <= 0.02) sigma = 0.03;
		break;
	}
	computeNormalFunc(mu, sigma);
	glutPostRedisplay();
}

/**
 * @brief Callback function for the top-level menu.
 * @param id The menu item identifier.
 */
void topMenuFunc(int id) {
	if (id == 0) {
		exit(0);
	}
}

/**
 * @brief Callback function for the file menu.
 * @param id The menu item identifier.
 */
void fileMenuFunction(int id) {
	switch (id) {
		case 1:
			fileName = "normal.dat";
			break;
		case 2:
			fileName = "expo.dat";
			break;
		case 3:
			fileName = "5.dat";
			break;
		case 4:
			fileName = "18.dat";
			break;
	}
	readFile(filePath + fileName);
	computeProbability(numIntervals);

	if (curveType == 0) {
		computeNormalFunc(mu, sigma);
	} else if (curveType == 1) {
		computeExponentialFunc(lambda);
	}

	glutPostRedisplay();
}

/**
 * @brief Callback function for the distribution menu.
 * @param id The menu item identifier.
 */
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

/**
 * @brief Callback function for the histogram menu.
 * @param id The menu item identifier.
 */
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

/**
 * @brief Callback function for the parameter step menu.
 * @param id The menu item identifier.
 */
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

/**
 * @brief Creates the right-click context menu for the application.
 */
void createMenu() {
	int fileMenu = glutCreateMenu(fileMenuFunction);
	glutAddMenuEntry("normal.dat", 1);
	glutAddMenuEntry("expo.dat", 2);
	glutAddMenuEntry("5.dat", 3);
	glutAddMenuEntry("18.dat", 4);

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

/**
 * @brief Reshape callback function to handle window resizing.
 * @param w The new width of the window.
 * @param h The new height of the window.
 */
void reshape(int w, int h) {
	float aspectRatio = (float)w / (float)h;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (aspectRatio >= 1.0f) {
		gluOrtho2D(0.0, aspectRatio, 0.0, 1.0);
	} else {
		gluOrtho2D(0.0, 1.0, 0.0, 1.0 / aspectRatio);
	}

	glViewport(0, 0, w, h);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/**
 * @brief The main function initializes the program and enters the GLUT main loop.
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return Exit status.
 */
int main(int argc, char** argv) {
	dataset = nullptr;
	endPoints = nullptr;
	prob = nullptr;

	readFile(filePath + fileName);
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