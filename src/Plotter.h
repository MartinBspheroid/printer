
#pragma once
#include "cinder/app/App.h"
#include "cinder/Utilities.h"



class Plotter
{
public:
	Plotter(){};
	~Plotter(){};


	float radians(float degrees);

	void  setInputSize(int x1, int y1, int x2, int y2);
	void  setScale(int  x, int y);
	void drawLine(float x1, float y1, float x2, float y2);
	void drawLine(const glm::vec2 &pos1, const glm::vec2 &pos2);
	void drawLineTo(float x, float y);
	void beginLine(float x, float y);
	void endLine(float x, float y);
	void penUp();
	void penDown();
	void init();
	void setCircleTolerance(int tolerance);
	void drawCircle(int x, int y, int radius);
	void drawSpline(int x1, int y1, int x2, int y2, int angle);

	void  move(int x, int y);
	void move(const glm::vec2 &pos);
	void  selectPen(int number);

	void  changeLineType(int lineType);
	// FT (model (,space (,angle)));
	void setFillType(int model, int space, int angle);
	//  -PT(Pen Thickness)
	void setPenThickness(float thicknes);

	void drawFillWedge(int x, int y, int radius, int startingAngle, int centralAngle, int resolution);

	//-EW(Edge Wedge)
	void drawStrokedWedge(int x, int y, int radius, int startingAngle, int centralAngle, int resolution);

	//-RA(Fill Rectangle Absolute)
	void drawRectangle(int x1, int y1, int x2, int y2);
	void quadTo(){
		ci::app::console() << "QUAD TO is not yet implemented" << std::endl;
	}
	void moveTo(){
		ci::app::console() << "MOVE TO is not yet implemented" << std::endl;
	}

	void setCallback(std::function<void(const std::string)> func){
		callback = func;
	}



private:
	std::function<void(const std::string)> callback;

	void send(std::string data){
		callback(data);
	}
};

