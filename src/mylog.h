#ifndef MYLOG_H
#define MYLOG_H
#include <fstream>

//using namespaces std;

class MyLog
{
public:
	MyLog(std::string filepath)
	{
		path = filepath;
		SaveFile.open(path.c_str());
	}
	~MyLog()
	{
		SaveFile.close();
	}
	void Write(std::string str)
	{
		SaveFile << str << std::endl;
	}

	void Open()
	{
		SaveFile.open(path.c_str(), std::ios::app);
	}
	void Close()
	{
		SaveFile.close();
	}
public:
	std::ofstream SaveFile;
private:
	std::string path;

};


#endif
