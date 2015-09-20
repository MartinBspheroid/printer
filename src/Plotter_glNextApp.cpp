#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "CinderImGui.h"
#include "Cinder-Serial.h"
#include "Console.h"

#include "Cinder-Serial.h"
#include "bulldog.h"
#include "script.h"
#include "plotterExporter.h"
#include "cinder/svg/Svg.h"
#include "cinder/Vector.h"


#include "Plotter.h"
// freestanding message callback...

void MessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if (msg->type == asMSGTYPE_WARNING)
		type = "WARN";
	else if (msg->type == asMSGTYPE_INFORMATION)
		type = "INFO";
	ci::app::console() << "in " << msg->section << " line " << msg->row << " " << msg->col << " " << type << " " << msg->message << " " << endl;
}


using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Cinder::Serial;

class Plotter_glNextApp : public App {
public:
	void setup() override;
	void mouseDown(MouseEvent event) override;
	void update() override;
	void draw() override;
	void cleanup() override;
	void keyUp(KeyEvent event) override;
	ConsoleWindow terminal;
	SerialDeviceRef mSerial;
	vector<string> serialPorts;
	void pushSerialCommand(const string &cmd);
	BullDog bd;
	as::ScriptRef mScript;
	PlotterExporter plot;
	void SVG(const fs::path file);
	void print(const string &data);
	void handleErrors(const asSMessageInfo &msg);
	void fileDrop(FileDropEvent event) override;
	fs::path svg_path;


	Plotter mPlotter;

};

void Plotter_glNextApp::setup()
{

	ui::initialize();
	terminal.AddCallback([=](string data){
		console() << data << endl;
		pushSerialCommand(data);
	});

	auto ports = SerialPort::getPorts();
	for (auto port : ports){
		console() << "SERIAL DEVICE" << endl;
		console() << "\tNAME: " << port->getName() << endl;
		console() << "\tDESCRIPTION: " << port->getDescription() << endl;
		console() << "\tHARDWARE IDENTIFIER: " << port->getHardwareIdentifier() << endl;
		serialPorts.push_back(port->getName());
	}
	if (!ports.empty())
	{
		auto s = SerialPort::findPortByNameMatching(regex(ports.front()->getName()), true);

		mSerial = SerialDevice::create(s, 9600);
		if (mSerial->isOpen()){
			terminal.AddLog(string("Serial port " + mSerial->getPortName() + " opened").c_str());
		}

	}
	plot.setPath(getAssetPath(fs::path()) / "plot.plot");


	asIScriptEngine *engine = as::Script::getEngine();
	int r = engine->SetMessageCallback(asMETHOD(Plotter_glNextApp, handleErrors), this, asCALL_THISCALL);
	assert(r >= 0);
	r = engine->RegisterGlobalFunction("void send(const string& in)", asMETHOD(Plotter_glNextApp, pushSerialCommand), asCALL_THISCALL_ASGLOBAL, this);
	assert(r >= 0);
	r = engine->RegisterGlobalFunction("void print(const string& in)", asMETHOD(Plotter_glNextApp, print), asCALL_THISCALL_ASGLOBAL, this);
	assert(r >= 0);

	bd.watch(getAssetPath(fs::path()) / "test.asc", [&]{
		mScript = as::Script::create(loadAsset("test.asc"), "");
		if (mScript){
			console() << "\n\tSCRIPT RELOADED\n\n" << endl;
			mScript->call("void setup()");

		}
		else{
			console() << "\n\tSCRIPT RELOADED with ERRORS!\n\n" << endl;
		}
	});
	mPlotter.setCallback([&](string data){pushSerialCommand(data); });
}


void Plotter_glNextApp::mouseDown(MouseEvent event)
{

}

void Plotter_glNextApp::update()
{
	bd.check();
	ui::ScopedMainMenuBar menuBar;

	// add a file menu
	if (ui::BeginMenu("File")){

		for (auto port : serialPorts){
			if (ui::MenuItem(port.c_str())){
				SerialPortRef s_Port = SerialPort::findPortByNameMatching(regex(port));
				if (s_Port){
					try{
						mSerial = SerialDevice::create(s_Port, 9600);
						if (mSerial->isOpen()){
							terminal.AddLog(string("Serial port " + mSerial->getPortName() + "opened").c_str());
						}
					}
					catch (serial::IOException &e){
						terminal.AddLog(string("error while opening port: " + s_Port->getName()).c_str());

					}
				}
			}
		}
		ui::EndMenu();
	}

	/// read serial and push it to console

	if (mSerial){
		uint8_t buffer[16];
		while (mSerial->getNumberOfAvailableBytes() > 0)
		{
			size_t size = mSerial->readBytes(buffer, 16);
			if (size > 0){
				string textBuffer = "->";
				for (size_t i = 0; i < 16; i++)
				{
					textBuffer += char(buffer[i]);
				}
				terminal.AddLog(textBuffer.c_str());
			}
		}
	}

	ui::SameLine();

	if (ui::Button("Function")){

		try{
			mScript->call("void test()");
		}
		catch (...){
			terminal.AddLog("[error]");
		}
	}
	ui::SameLine();
	if (ui::Button("SVG")){
		if (svg_path.string() != ""){
			SVG(svg_path);
		}
		else{
			terminal.AddLog("file is not yet loaded");
		}
	}
	ui::SameLine();
	if (ui::Button("PenTest")){
		mPlotter.selectPen(1);
		mPlotter.selectPen(2);
		mPlotter.selectPen(3);
	}


}

void Plotter_glNextApp::draw()
{
	gl::clear(Color(0, 0, 0));
	static bool terminalOpened = true;
	terminal.Draw("Serial Terminal", &terminalOpened);

}

void Plotter_glNextApp::pushSerialCommand(const string &cmd)
{

	if (cmd == "end"){
		//console() << "bar" << endl;
		plot.exportFile();
	}
	else{
		plot.record(cmd);
	}

	if (mSerial){
		const int bufferSize = cmd.size();
		auto buffer = reinterpret_cast<const uint8_t*>(&cmd[0]);
		mSerial->writeBytes(buffer, bufferSize);
		sleep(25);
	}
}

void Plotter_glNextApp::cleanup()
{
	mSerial->flush();
	mSerial->close();
	ui::Shutdown();
}

void Plotter_glNextApp::SVG(const fs::path file)
{
	svg::DocRef	mDoc;
	mDoc = svg::Doc::create(loadFile(file));

	mPlotter.init();


	for (auto s : mDoc->getChildren()){
		auto c = s->getShapeAbsolute().getContours();
		for (auto &m : c){
			for (size_t i = 0; i < m.getNumSegments(); i++)

				// types MOVETO, LINETO, QUADTO, CUBICTO, CLOSE
				switch (m.getSegmentType(i))
			{
				case Path2d::MOVETO:
					mScript->call("void svgMoveTo()");
					break;
				case Path2d::LINETO:
					mScript->call("void drawLine(float x1, float y1, float x2, float y2)", m.getSegmentPosition(i, 0.0f).x, m.getSegmentPosition(i, 0.0f).y, m.getSegmentPosition(i, 1.0f).x, m.getSegmentPosition(i, 1.0f).y);
					break;
				case Path2d::QUADTO:
					mScript->call("void svgQuadTo()");
					break;
				case Path2d::CUBICTO:
					mScript->call("void svgCubicTo()");
					for (float pos = 0.1; pos <= 1;){
						vec2 v1 = m.getSegmentPosition(i, pos);
						mScript->call("void drawLineTo(float x, float y)", v1.x, v1.y);
						pos += 0.1;
					}
					break;
				case Path2d::CLOSE:
					mScript->call("void drawLine(float x1, float y1, float x2, float y2)", m.getSegmentPosition(i, 0.0f).x, m.getSegmentPosition(i, 0.0f).y, m.getSegmentPosition(i, 1.0f).x, m.getSegmentPosition(i, 1.0f).y);
					break;
				default:
					mScript->call("void svgDefault()");
					break;
			}

		}
	}
}

void Plotter_glNextApp::print(const string &data)
{
	terminal.AddLog(data.c_str());
}

void Plotter_glNextApp::handleErrors(const asSMessageInfo &msg)
{
	const char *type = "ERR ";
	if (msg.type == asMSGTYPE_WARNING)
		type = "WARN";
	else if (msg.type == asMSGTYPE_INFORMATION)
		type = "INFO";
	string errorMessage = "in "
		+ string(msg.section)
		+ " line "
		+ toString(msg.row)
		+ " "
		+ toString(msg.col)
		+ " "
		+ type
		+ " "
		+ msg.message;
	terminal.AddLog(errorMessage.c_str());
}

CINDER_APP(Plotter_glNextApp, RendererGl)

void Plotter_glNextApp::keyUp(KeyEvent event)
{
	switch (event.getCode())
	{
	case KeyEvent::KEY_F5:
		if (!svg_path.empty())SVG(svg_path);
		else
		{
			terminal.AddLog("file is not yet loaded");
		}
		break;
	default:
		break;
	}
}

void Plotter_glNextApp::fileDrop(FileDropEvent event)
{
	string text = "File " + event.getFile(0).string() + " loaded";
	terminal.AddLog(text.c_str());
	string ext = event.getFile(0).extension().string();
	terminal.AddLog(ext.c_str());
	svg_path = event.getFile(0);

}
