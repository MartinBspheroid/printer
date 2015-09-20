#pragma  once
#include "cinder/app/App.h"

class PlotterExporter
{
public:
	PlotterExporter(){
		mRecording = false;
		buffer = "";
	}
	~PlotterExporter(){};
	void record(const string data){
		buffer += data;
	}
	void setPath(const fs::path datafile){
		mFilePath = datafile;
	}
	void exportFile(){
		auto file = writeFileStream(mFilePath);
		file->writeData(buffer.data(), buffer.length());
		buffer = "";
	}

private:
	bool mRecording;
	fs::path mFilePath;
	string buffer;
};
