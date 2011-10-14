#ifndef GLF_WRAPPER_HPP
#define GLF_WRAPPER_HPP

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <glf/utils.hpp>
#include <map>

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------

namespace glf
{
	struct Variable
	{
		//---------------------------------------------------------------------
		enum Type { UNIFORM, ATTRIBUTE, BLOCK, MAX };
		//---------------------------------------------------------------------
		std::string name;
		Type   		category;			// Uniform, attribute or block 
		GLenum		type;				// Type
		GLint  		location;			// Location of variable
		GLint  		unit;				// For texture/sampler : Texture unit of variable
										// For uniform buffer  : Bind index of variable 
										// For others	       : Nothing
		//---------------------------------------------------------------------
					Variable();
		std::string ToString() const;
	};

	struct Program
	{
	public:
		//---------------------------------------------------------------------
		GLuint 				id;
		std::string 		name;
		bool 				compiled;
		std::map<std::string,Variable> variables;
		//---------------------------------------------------------------------
			 	Program(	const std::string& _name);
			   ~Program(	);
		bool	Compile(	const std::string& _vFile,
							const std::string& _fFile);
		bool	Compile(	const std::string& _vFile,
							const std::string& _gFile,
							const std::string& _fFile);
		bool	Compile(	const std::string& _vFile,
							const std::string& _cFile,
							const std::string& _eFile,
							const std::string& _fFile);
		bool	Compile(	const std::string& _vFile,
							const std::string& _cFile,
							const std::string& _eFile,
							const std::string& _gFile,
							const std::string& _fFile);
		GLint 	Output(		const std::string& _outName) const;

		const Variable& 	operator[](const std::string& _varName) const;
		std::string			ToString() const;
		//---------------------------------------------------------------------
	public:
		//---------------------------------------------------------------------
		static bool 		IsTextureSampler(	GLenum _type);
		static void			AnalyzeProgram(		const std::string& _name, 
												GLuint _id, 
												std::map<std::string,Variable>& _variables);
	};
}

#endif
