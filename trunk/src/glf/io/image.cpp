//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <glf/io/image.hpp>
#include <glf/utils.hpp>
#include <string>
#include <cstring>
#include <iostream>
#include <IL/il.h>
#include <IL/ilu.h>

namespace glf
{
	namespace io
	{
		//---------------------------------------------------------------------
		void LoadTexture(	const std::string& _filename,
							Texture2D& _texture,
							bool _srgb,
							bool _allocateMipmap,
							bool _verbose)
		{
			try
			{
				ilInit();
				
				ILuint imgH;
				ilGenImages(1, &imgH);
				ilBindImage(imgH);
				if(!ilLoadImage((const ILstring)_filename.c_str()))
				{
					Error("Load image error : file does not exist (%s)",_filename.c_str());
				}
				
				if(_verbose)
				{
					Info("Load image : %s",_filename.c_str());
				}

				// Convert all to RGBA. TODO Need improvement ...
				GLenum format;
				bool convert = false;
				ILenum target;
				switch(ilGetInteger(IL_IMAGE_FORMAT))
				{
					case IL_RGB  			: format = GL_RGBA; convert=true; target = IL_RGBA; break;
					case IL_RGBA 			: format = GL_RGBA; break;
					case IL_BGR  			: format = GL_RGBA; convert=true; target = IL_RGBA; break;
					case IL_BGRA 			: format = GL_RGBA; convert=true; target = IL_RGBA; break;
					case IL_LUMINANCE 		: format = GL_RGBA; convert=true; target = IL_RGBA; break;
					case IL_COLOUR_INDEX	: format = GL_RGBA; convert=true; target = IL_RGBA; break;
					case IL_ALPHA			: format = GL_RGBA; convert=true; target = IL_RGBA; break;
					case IL_LUMINANCE_ALPHA	: format = GL_RGBA; convert=true; target = IL_RGBA; break;
					default 				: Error("Load image error : unsupported format (%s)",_filename.c_str());
				}

				GLenum type;
				switch(ilGetInteger(IL_IMAGE_TYPE))
				{
					case IL_UNSIGNED_BYTE	: type = GL_UNSIGNED_BYTE; break;
					case IL_FLOAT			: type = GL_FLOAT; break;
					//case IL_BYTE			: type = GL_BYTE; break;
					//case IL_SHORT			: type = GL_SHORT; break;
					//case IL_UNSIGNED_SHORT: type = GL_UNSIGNED_SHORT; break;
					//case IL_INT			: type = GL_INTEGER; break;
					//case IL_UNSIGNED_INT	: type = GL_UNSIGNED_INT; break;
					//case IL_DOUBLE		: Error("Load image error : double data are not supported (%s)",_filename.c_str()); break;
					default 				: Error("Load image error : unsupported data (%s)",_filename.c_str());
				}

				if(convert)
					 ilConvertImage(target, ilGetInteger(IL_IMAGE_TYPE));

				// Flip image
				ILinfo ImageInfo;
				iluGetImageInfo(&ImageInfo);
				if( ImageInfo.Origin == IL_ORIGIN_UPPER_LEFT )
				{
					iluFlipImage();
					if(_verbose) Info("Flip image");
				}

				switch(type)
				{
					case GL_UNSIGNED_BYTE	:
					{
						if(_srgb)
						{
							_texture.Allocate( GL_SRGB8_ALPHA8, ilGetInteger(IL_IMAGE_WIDTH),ilGetInteger(IL_IMAGE_HEIGHT),_allocateMipmap);
							if(_verbose) Info("Allocate texture - format:GL_SRGB8_ALPHA8, w:%d, h:%d, mipmap:%s",ilGetInteger(IL_IMAGE_WIDTH),ilGetInteger(IL_IMAGE_HEIGHT), (_allocateMipmap?"TRUE":"FALSE") );
						}
						else
						{
							_texture.Allocate( GL_RGBA8, ilGetInteger(IL_IMAGE_WIDTH),ilGetInteger(IL_IMAGE_HEIGHT),_allocateMipmap);
							if(_verbose) Info("Allocate texture - format:GL_RGBA8, w:%d, h:%d, mipmap:%s",ilGetInteger(IL_IMAGE_WIDTH),ilGetInteger(IL_IMAGE_HEIGHT), (_allocateMipmap?"TRUE":"FALSE") );
						}
					}
					break;
					case GL_FLOAT			:
					{
						if(_srgb)
							Warning("Try to convert to SRGB, but texture format is not compatible");
						_texture.Allocate( GL_RGBA32F, ilGetInteger(IL_IMAGE_WIDTH),ilGetInteger(IL_IMAGE_HEIGHT),_allocateMipmap);
						if(_verbose) Info("Allocate texture - format:GL_RGBA32F, w:%d, h:%d, mipmap:%s",ilGetInteger(IL_IMAGE_WIDTH),ilGetInteger(IL_IMAGE_HEIGHT), (_allocateMipmap?"TRUE":"FALSE") );
					}
					break;
					default 				:
					{
						Error("Load image error : unsupported data (%s)",_filename.c_str());
					}
				}
				_texture.Fill(format,type,ilGetData());

				ilDeleteImages(1, &imgH);
				ilShutDown();
			}
			catch (const std::exception &e) 
			{
				Error("Unable to read image file \"%s\": %s",_filename.c_str(),e.what());
			}
		}
		//----------------------------------------------------------------------
	}
}
