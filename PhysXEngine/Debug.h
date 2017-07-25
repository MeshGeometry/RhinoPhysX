#pragma once
#include <fstream>
#include <iostream>

using namespace std;
namespace PhysXEngine
{
	class DebugFile
	{
	private:
		fstream debug_file;
		string path_to_file;
	public:
		void set_path(string path) {path_to_file = path;};
		void log(string msg);
	};
}