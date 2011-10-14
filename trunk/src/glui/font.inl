//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

namespace glui
{
	//-------------------------------------------------------------------------
	// Font
	//-------------------------------------------------------------------------
	template<class SerializedFont>
	void Font::Load()
	{ 	
		SerializedFont font;

		name 		= font.FontName;
		size 		= font.FontSize;
		strides 	= new unsigned int[font.NumberOfCharacters];
		sizes 		= new glm::uvec2[font.NumberOfCharacters];
		nCharacters = font.NumberOfCharacters;

		for(unsigned int i=0; i<nCharacters; ++i)
		{
			sizes[i].x	= font.CharWidth[i];
			sizes[i].y	= font.CharHeight[i];
			strides[i] 	= font.CharStride[i];
		}

        // Create texture from character pixel          
		Characters.Allocate(GL_RGBA8,font.Width,font.Height);
		Characters.SetFiltering(GL_LINEAR,GL_LINEAR);
		Characters.SetWrapping(GL_REPEAT,GL_REPEAT);
		Characters.Fill(GL_RGBA,GL_UNSIGNED_BYTE,font.Pixels);
	}
	//--------------------------------------------------------------------------
	inline unsigned int Font::Width(char _c) const
	{
		assert(sizes!=NULL);
		return sizes[static_cast<unsigned int>(_c)].x;
	}
	//--------------------------------------------------------------------------
	inline unsigned int Font::Height(char _c) const
	{
		assert(sizes!=NULL);
		return sizes[static_cast<unsigned int>(_c)].y;
	}
	//--------------------------------------------------------------------------
	inline glm::uvec2 Font::Size(char _c) const
	{
		assert(sizes!=NULL);
		return sizes[static_cast<unsigned int>(_c)];
	}
	//--------------------------------------------------------------------------
	inline unsigned int Font::Size() const
	{
		return size;
	}
	//--------------------------------------------------------------------------
	inline std::string Font::Name() const
	{
		return name;
	}
	//--------------------------------------------------------------------------
	inline unsigned int Font::Stride(char _c) const
	{
		assert(strides!=NULL);
		return strides[static_cast<unsigned int>(_c)];
	}
}
