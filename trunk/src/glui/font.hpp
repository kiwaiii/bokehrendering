#ifndef GLUI_FONT_HPP
#define GLUI_FONT_HPP

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <glf/wrapper.hpp>
#include <glf/buffer.hpp>
#include <glf/texture.hpp>

using namespace glf;

namespace glui
{
	//-------------------------------------------------------------------------
	// Class Font 
	//-------------------------------------------------------------------------	
	class Font
	{
		public:
			//------------------------------------------------------------------
			// Constructor
			//------------------------------------------------------------------
			Font();

			//------------------------------------------------------------------
			// Destructor
			//------------------------------------------------------------------
			~Font();

			//------------------------------------------------------------------
			// Load a font base on the template parameter
			//------------------------------------------------------------------
			template<class SerializedFont>
			void Load();

			//------------------------------------------------------------------
			// Return font description in a formated string
			//------------------------------------------------------------------
			std::string ToString() const;

			//------------------------------------------------------------------
			// Return width of a character
			//------------------------------------------------------------------
			inline unsigned int Width(char _c) const;

			//------------------------------------------------------------------
			// Return height of a character
			//------------------------------------------------------------------
			inline unsigned int Height(char _c) const;

			//------------------------------------------------------------------
			// Return size of a character
			//------------------------------------------------------------------
			inline glm::uvec2 Size(char _c) const;

			//------------------------------------------------------------------
			// Return size of font
			//------------------------------------------------------------------
			inline unsigned int Size() const;

			//------------------------------------------------------------------
			// Return name of font
			//------------------------------------------------------------------
			inline std::string Name() const;

			//------------------------------------------------------------------
			// Return stride of a character
			//------------------------------------------------------------------
			inline unsigned int Stride(char _c) const;

			Texture2D Characters;		// Texture which stores characters

		private:
			//------------------------------------------------------------------
			// Forbidden methods
			//-----------------------------------------------------------------
			Font(const Font& _copy);
			Font& operator=(const Font& _copy);

			//------------------------------------------------------------------
			// Attributes
			//------------------------------------------------------------------
			std::string  name;			// Font name
			unsigned int size;			// Font size
			unsigned int* strides;		// Stride in texture to acces to a character
			glm::uvec2*  sizes;		// Size of a character
			unsigned int  nCharacters;	// Number of a character
	};


	class FontRender
	{
		public:
			//------------------------------------------------------------------
			// Constructor
			//------------------------------------------------------------------
			FontRender();
			
			//------------------------------------------------------------------
			// Initialize font renderer
			//------------------------------------------------------------------
			void Initialize();

			//------------------------------------------------------------------
			// Destructor
			//------------------------------------------------------------------
			~FontRender();

			//------------------------------------------------------------------
			// Draw a color string at position (x,y)
			// _x : start position
			// _y : start position
			// _font : font 
			// _message : message 
			// _color : color 
			//------------------------------------------------------------------
			void DrawString(int _x, int _y, const Font& _font, const std::string _message, const glm::vec4& _color);

			//------------------------------------------------------------------
			// Return string width in pixel
			// _font : font 
			// _message : message 
			//------------------------------------------------------------------
			unsigned int StringWidth(const Font& _font, const std::string _message) const;

			//------------------------------------------------------------------
			// Set view matrix
			// _view : new view matrix
			//------------------------------------------------------------------
			//void View(const glm::mat4& _view);

			//------------------------------------------------------------------
			// Set reshape window boundaries
			// _w : new window width 
			// _h : new window height
			//------------------------------------------------------------------
			void Reshape(unsigned int _w, unsigned int _h);

		private:
			//------------------------------------------------------------------
			// Forbidden methods
			//------------------------------------------------------------------
			FontRender(const FontRender& _copy);
			FontRender& operator=(const FontRender& _copy);
			
			glm::mat4 							projection, view;

			VertexBuffer4F						vbuffer;
			IndexBuffer							ibuffer;
			VertexArray							primitive;

			Program								program;
			GLint								projectionVar,
												viewVar,
												colorVar,
												translateVar,
												scaleVar,
												texStrideVar,
												texWidthVar,
												textureVar,
												unitVar;
	};
}

//-----------------------------------------------------------------------------
// Includes inline definition
//-----------------------------------------------------------------------------
#include <glui/font.inl>

#endif
