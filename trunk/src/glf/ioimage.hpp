#ifndef GLF_IO_LIGHT_HPP
#define GLF_IO_LIGHT_HPP

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/texture.hpp>
#include <string>

namespace glf
{
	namespace io
	{
		void LoadTexture(	const std::string& _filename,
							Texture2D& _texture,
							bool _srgb,
							bool _verbose=true);
	};
}
#endif
