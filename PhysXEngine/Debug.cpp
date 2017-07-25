#include "stdafx.h"
#include "Debug.h"

namespace PhysXEngine
{
	void DebugFile::log(string msg)
	{
		debug_file.open(path_to_file, fstream::out);
		if(debug_file.good())
		{
			debug_file << msg.c_str();
			debug_file.close();
		}
	}
}