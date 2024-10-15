#include "utility.h"

void printString(float x, float y, std::string str) {
	glRasterPos2f(x, y);
	for (int i = 0; i < str.length(); i++)
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, str[i]);
}
