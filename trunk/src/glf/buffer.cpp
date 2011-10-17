//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <glf/buffer.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	namespace semantic
	{
		GLint Position 	= 0;
		GLint Normal 	= 1;
		GLint TexCoord 	= 2;
		GLint Tangent 	= 3;
		GLint Color	 	= 4;
		GLint Bitangent	= 5;
	};
	//-------------------------------------------------------------------------
	VertexArray::VertexArray()
	{
		glGenVertexArrays(1, &id);
	}
	//-------------------------------------------------------------------------
	VertexArray::~VertexArray()
	{
		glDeleteVertexArrays(1, &id);
	}
	//-------------------------------------------------------------------------
	void VertexArray::Draw( 	GLenum _primitiveType,
								const IndexBuffer& _buffer) const
	{
		glBindVertexArray(id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffer.id);
		glDrawElements(_primitiveType, _buffer.count, GL_UNSIGNED_INT, GLF_BUFFER_OFFSET(0) );
		glBindVertexArray(0);
	}
	//-------------------------------------------------------------------------
	void VertexArray::Draw( 	GLenum _primitiveType,
								const IndexBuffer& _buffer,
								std::size_t _count,
								std::size_t _first) const
	{
		glBindVertexArray(id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffer.id);
		glDrawElements(_primitiveType, _count, GL_UNSIGNED_INT, GLF_BUFFER_OFFSET(_first*sizeof(unsigned int)) );
		glBindVertexArray(0);
	}
	//-------------------------------------------------------------------------
	void VertexArray::Draw(		GLenum _primitiveType, 
								std::size_t _count,
								std::size_t _first) const
	{
		glBindVertexArray(id);
		glDrawArrays(_primitiveType, _first, _count);
		glBindVertexArray(0);
	}
}

