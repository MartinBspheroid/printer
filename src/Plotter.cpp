#include "Plotter.h"

using namespace ci;
using namespace std;


float Plotter::radians(float degrees)
{
	return degrees * (3.14 / 180);
}

void Plotter::setInputSize(int x1, int y1, int x2, int y2)
{
	send("IP" + toString(x1) + "," + toString(y1) + "," + toString(x2) + "," + toString(y2) + ";\n");
}

void Plotter::setScale(int x, int y)
{
	send("SC0," + toString(x) + ",0," + toString(y) + ";\n");
}

void Plotter::drawLine(float x1, float y1, float x2, float y2)
{
	move(x1, y1);
	penDown();
	move(x2, y2);
	penUp();
}

void Plotter::drawLine(const vec2 &pos1, const vec2 &pos2)
{
	drawLine(pos1.x, pos1.y, pos2.x, pos2.y);
}

void Plotter::drawLineTo(float x, float y)
{
	move(x, y);
}

void Plotter::beginLine(float x, float y)
{
	move(x, y);
	penDown();
}

void Plotter::endLine(float x, float y)
{
	move(x, y);
	penUp();
}

void Plotter::penUp()
{
	send("PU;\n");
}

void Plotter::penDown()
{
	send("PD;\n");
}

void Plotter::init()
{
	send("IN;\n");
}

void Plotter::setCircleTolerance(int tolerance)
{
	send("CT " + toString(tolerance) + ";\n");
}

void Plotter::drawCircle(int x, int y, int radius)
{
	move(x, y);
	penDown();
	send("CI " + toString(radius) + ";\n");
	penUp();
}

void Plotter::drawSpline(int x1, int y1, int x2, int y2, int angle)
{
	move(x1, y1);
	penDown();
	send("AA" + toString(x2) + "," + toString(y2) + "," + toString(angle) + ";\n");
	penUp();
}

void Plotter::move(int x, int y)
{
	send("PA" + toString(x) + "," + toString(y) + ";\n");
}

void Plotter::move(const vec2 &pos)
{
	move(pos.x, pos.y);
}

void Plotter::selectPen(int number)
{

	send("SP" + toString(number) + ";\n");
}

void Plotter::changeLineType(int lineType)
{
	send("LT " + toString(lineType) + ", 25;\n");
}

void Plotter::setFillType(int model, int space, int angle)
{
	send("FT " + toString(model) + "," + toString(space) +/* "," + toString(angle)+ */";\n");
}

void Plotter::setPenThickness(float thicknes)
{
	send("PT " + toString(thicknes) + ";\n");
}

void Plotter::drawFillWedge(int x, int y, int radius, int startingAngle, int centralAngle, int resolution)
{
	move(x, y);
	send("WG " + toString(radius) + "," + toString(startingAngle) + "," + toString(centralAngle) + "," + toString(resolution) + ";\n");
}

void Plotter::drawStrokedWedge(int x, int y, int radius, int startingAngle, int centralAngle, int resolution)
{
	move(x, y);
	send("EW " + toString(radius) + "," + toString(startingAngle) + "," + toString(centralAngle) + "," + toString(resolution) + ";\n");
}

void Plotter::drawRectangle(int x1, int y1, int x2, int y2)
{
	move(x1, y1);
	send("RA" + toString(x2) + "," + toString(y2) + ";\n");
}
