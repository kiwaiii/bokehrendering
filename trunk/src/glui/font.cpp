//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <glui/font.hpp>
#include <glm/glm.hpp>
//#include <glm/gtc/matrix_projection.hpp>
#include <glm/gtc/matrix_transform.hpp>

using std::min;
using std::max;

#ifdef WIN32
	#pragma warning( disable : 4244 )
#endif

namespace glui
{

	//-------------------------------------------------------------------------
	// Font
	//-------------------------------------------------------------------------
	Font::Font()
	{
		name 		= "Invalid";
		size 		= 0;
		strides 	= NULL;
		sizes 		= NULL;
		nCharacters = 0;
	}
	//-------------------------------------------------------------------------
	Font::~Font()
	{
		delete[] sizes;
		delete[] strides;
	}
	//-------------------------------------------------------------------------
	std::string Font::ToString() const
	{
		std::stringstream info;
		info << "Font description [Name:"<< name << ", Size:" << size << "]\n";
		for(unsigned int i=0;i<nCharacters;++i)
			info << "\tChar:" << static_cast<char>(i) << " [Stride:"<< strides[i] << ", Size:(" << sizes[i].x << "," << sizes[i].y << ")]\n";
		return info.str();
	}
	//-------------------------------------------------------------------------
    //std::string Font::FontID(const std::string& _font, int _size) const
    //{
    //    std::stringstream fontID;
    //    fontID << _font << "_" << _size;
    //    return fontID.str();
    //}

	
    //-------------------------------------------------------------------------
    // Font renderer
    //-------------------------------------------------------------------------
    FontRender::FontRender():
	program("FontRenderer")
    {

    }
    //-------------------------------------------------------------------------
    FontRender::~FontRender()
    {
     
    }
	//-------------------------------------------------------------------------
	void FontRender::Initialize()
	{
	std::string vsSource = "\n\
			#version 130\n\
			\n\
			uniform float TexCharacterWidth;\n\
			uniform float TexCharacterStride;\n\
			uniform vec2  Scale;\n\
			uniform vec2  Translate;\n\
			uniform mat4  Projection;\n\
			uniform mat4  View;\n\
			\n\
			in  vec4 Position;\n\
			out vec2 ipTexCoord;\n\
			\n\
			void main()\n\
			{\n\
				gl_Position = Projection  * View * (Position*vec4(Scale.x,Scale.y,1.0,1.0) + vec4(Translate.x,Translate.y,0.0,0.0));\n\
				ipTexCoord  = Position.xy * vec2(TexCharacterWidth,1.0) + vec2(TexCharacterStride,0.0);\n\
			}";

		std::string fsSource = "\n\
			#version 130\n\
			uniform vec4 	  Color;\n\
			uniform sampler2D Texture;\n\
			\n\
			in  vec2 ipTexCoord;\n\
			out vec4 FragColor;\n\
			\n\
			void main()\n\
			{\n\
				vec4 value = texture2D(Texture,ipTexCoord).xxxw * vec4(Color.xyz,1.0);\n\
				FragColor = value;\n\
			}";

		vbuffer.Allocate(4,GL_STATIC_DRAW);
		glm::vec4* vertices = vbuffer.Lock();
		vertices[0] = glm::vec4(0.f,0.f,0.f,1.f);
		vertices[1] = glm::vec4(1.f,0.f,0.f,1.f);
		vertices[2] = glm::vec4(0.f,1.f,0.f,1.f);
		vertices[3] = glm::vec4(1.f,1.f,0.f,1.f);
		vbuffer.Unlock();

		ibuffer.Allocate(6,GL_STATIC_DRAW);
		unsigned int* indices = ibuffer.Lock();
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 3;
		indices[3] = 0;
		indices[4] = 3;
		indices[5] = 2;
		ibuffer.Unlock();

        // Create component for Render
		program.Compile(vsSource,fsSource);
		texWidthVar 	= program["TexCharacterWidth"].location;
		texStrideVar 	= program["TexCharacterStride"].location;
		scaleVar 		= program["Scale"].location;
		translateVar 	= program["Translate"].location;
		projectionVar 	= program["Projection"].location;
		viewVar 		= program["View"].location;
		textureVar 		= program["Texture"].location;
		unitVar 		= program["Texture"].unit;
		colorVar 		= program["Color"].location;

		primitive.Add(vbuffer,program["Position"].location,4,GL_FLOAT);

		//Tools::Logger::Info(primitiveVar.ToString());
		//Tools::Logger::Info(effect.GetDescriptor().ToString());


		//projection = glm::ortho2D(0.f,512.f,0.f,512.f);
		projection = glm::ortho(0.f,512.f,0.f,512.f);
	}
    //-------------------------------------------------------------------------
    //void FontRender::View(const glm::mat4& _view)
    //{
	//	view = _view;
    //}
    //-------------------------------------------------------------------------
    void FontRender::Reshape(unsigned int _w, unsigned int _h)
    {
		//projection = glm::ortho2D(0.f,(float)_w,0.f,(float)_h);
		projection = glm::ortho(0.f,(float)_w,0.f,(float)_h);
    }
    //-------------------------------------------------------------------------
    void FontRender::DrawString(int _x, int _y, const Font& _font, const std::string _text, const glm::vec4& _color)
    {
	    int nChar, stride;
		char currentChar;
        stride = 0;
        nChar  = _text.size();
        float texWidth, texStride;
        glm::vec2 translate, scale;
        for(int i=0; i<nChar; ++i)
        {
            // Update value
            currentChar  = _text[i];
            translate.x  = _x + stride;
            translate.y  = _y;
            scale.x      = _font.Width(currentChar);
            scale.y      = _font.Height(currentChar);
            texWidth     = _font.Width(currentChar)  / static_cast<float>(_font.Characters.size.x);
            texStride    = _font.Stride(currentChar) / static_cast<float>(_font.Characters.size.x);
            stride      += _font.Width(currentChar);

			glActiveTexture(GL_TEXTURE0 + unitVar);
			glBindTexture(_font.Characters.target,_font.Characters.id);

            // Set Data
			glProgramUniformMatrix4fv(program.id, 	projectionVar, 	1, GL_FALSE, &projection[0][0]);
			glProgramUniformMatrix4fv(program.id, 	viewVar,  	 	1, GL_FALSE, &view[0][0]);
			glProgramUniform4fv(program.id, 		colorVar,  	  	1, &_color[0]);
			glProgramUniform2fv(program.id, 		translateVar, 	1, &translate[0]);
			glProgramUniform2fv(program.id, 		scaleVar,  	  	1, &scale[0]);
			glProgramUniform1fEXT(program.id, 		texStrideVar,  	texStride);
			glProgramUniform1fEXT(program.id, 		texWidthVar,   	texWidth);
			glProgramUniform1iEXT(program.id, 		textureVar,   	0);

			glUseProgram(program.id);
			primitive.Draw(GL_TRIANGLES,ibuffer);
        }
    }
	//-------------------------------------------------------------------------
	unsigned int FontRender::StringWidth(const Font& _font, const std::string _message) const
	{
		unsigned int width 		= 0;
		const unsigned int size = _message.length();
		for(unsigned int i=0;i<size;++i)
			width += _font.Width(_message[i]);
		return width;
	}
}

