//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <sstream>

namespace glf
{
	//-------------------------------------------------------------------------
	template<typename T>
	void ProgramOptions::AddDefine(	const std::string& _name, 
									const T& _value)
	{
		std::stringstream option;
		//option << "#define " << _name << " " << _value;
		//option << "#define ";
		options.push_back(option.str());
	}
	//-------------------------------------------------------------------------
}

