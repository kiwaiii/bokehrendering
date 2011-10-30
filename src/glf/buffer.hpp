#ifndef GLF_BUFFER_HPP
#define GLF_BUFFER_HPP

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <glf/utils.hpp>
#include <glf/memory.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	namespace semantic
	{
		extern GLint Position;
		extern GLint Normal;
		extern GLint TexCoord;
		extern GLint Tangent;
		extern GLint Color;
		extern GLint Bitangent;
	};
	//--------------------------------------------------------------------------
	template<GLenum B, typename T>
	class IBuffer
	{
	public:
		//------------------------------------------------------------------
		typedef T 		DataType;	// Buffer element type
		const GLenum 	BufferType;	// Buffer type
		//------------------------------------------------------------------
						~IBuffer(	);
		explicit 		IBuffer(	);
		explicit 		IBuffer(	std::size_t _nElements,
										GLenum _update = GL_DYNAMIC_DRAW);
		void 	 		Allocate(	std::size_t _nElements);
		void 			Allocate(	std::size_t _nElements, 
									GLenum _update);
		inline T* 		Lock(		GLenum _access=GL_READ_WRITE);
		inline void 	Unlock(		);
		inline void 	Fill(		T* _data, 
									std::size_t _count);

	private:
		// Forbiddent methods
						IBuffer(IBuffer<B,T>&);
		IBuffer<B,T>& 	operator=(IBuffer<B,T>&);

	public:
		GLuint			id;
		GLenum 			update;		// Frequence of update
		std::size_t		count;		// Number of element in buffer
		bool 			lock;		// True if buffer is locked, false otherwise
	};

	//-------------------------------------------------------------------------
	// Indirect buffers
	//-------------------------------------------------------------------------
	// Indirect buffer structure 
	struct DrawArraysIndirectCommand
	{
		GLuint count;
		GLuint primCount;
		GLuint first;
		GLuint reservedMustBeZero;
	};
	//--------------------------------------------------------------------------
	// Indirect indexed buffer structure 
	struct DrawElementsIndirectCommand
	{
		GLuint count;
		GLuint primCount;
		GLuint firstIndex;
		GLint  baseVertex;
		GLuint reservedMustBeZero;
	};

	//-------------------------------------------------------------------------
	// GL Buffer definition
	//-------------------------------------------------------------------------
	template<class T> struct VertexBuffer
	{
		typedef T 												Data;
		typedef IBuffer<GL_ARRAY_BUFFER,T>						Buffer;
	};
	//-------------------------------------------------------------------------
	template<class T> struct UniformBuffer
	{
		typedef T 												Data;
		typedef IBuffer<GL_UNIFORM_BUFFER,T>					Buffer;
	};
	//-------------------------------------------------------------------------
	template<class T> struct CopyReadBuffer		// GPU -> CPU only
	{
		typedef T								 				Data;
		typedef IBuffer<GL_COPY_READ_BUFFER,Data>				Buffer;
	};
	//-------------------------------------------------------------------------
	template<class T> struct CopyWriteBuffer	// CPU -> GPU only
	{
		typedef T									 			Data;
		typedef IBuffer<GL_COPY_WRITE_BUFFER,Data>				Buffer;
	};
	//-------------------------------------------------------------------------
	template<class T> struct PixelPackBuffer	// GPU -> CPU only
	{
		typedef T								 				Data;
		typedef IBuffer<GL_PIXEL_PACK_BUFFER,Data>				Buffer;
	};
	//-------------------------------------------------------------------------
	template<class T> struct PixelUnpackBuffer	// CPU -> GPU only
	{
		typedef T									 			Data;
		typedef IBuffer<GL_PIXEL_UNPACK_BUFFER,Data>			Buffer;
	};
	//--------------------------------------------------------------------------
	typedef IBuffer<GL_DRAW_INDIRECT_BUFFER,DrawArraysIndirectCommand>		IndirectArrayBuffer;
	typedef IBuffer<GL_DRAW_INDIRECT_BUFFER,DrawElementsIndirectCommand>	IndirectElementBuffer;
	typedef IBuffer<GL_ELEMENT_ARRAY_BUFFER,unsigned int>		IndexBuffer;
	typedef IBuffer<GL_ATOMIC_COUNTER_BUFFER,unsigned int>		AtomicCounterBuffer;
	//--------------------------------------------------------------------------
	typedef VertexBuffer<float>::Buffer							VertexBuffer1F;
	typedef VertexBuffer<glm::vec2>::Buffer						VertexBuffer2F;
	typedef VertexBuffer<glm::vec3>::Buffer						VertexBuffer3F;
	typedef VertexBuffer<glm::vec4>::Buffer						VertexBuffer4F;
	//--------------------------------------------------------------------------
	struct VertexArray
	{
	public:
		//----------------------------------------------------------------------
			 VertexArray(	);
			~VertexArray(	);
		template<typename T>
		void Add(		//typename const VertexBuffer<T>::Buffer& _buffer,
						const T& 			_buffer,
						GLint    			_location, 
						int      			_nComponents,
						GLenum   			_componentType,
						bool     			_normalize=false,
						std::size_t 		_offset=0);
		void Draw( 		GLenum				_primitiveType,
						const IndexBuffer&	_buffer) const;
		void Draw( 		GLenum				_primitiveType,
						const IndexBuffer&	_buffer,
						std::size_t			_count,
						std::size_t			_first) const;
		void Draw(		GLenum				_primitiveType, 
						std::size_t			_count,
						std::size_t			_first=0) const;
		void Draw(		GLenum 				_primitiveType,
						std::size_t 		_count,
						std::size_t 		_first,
						std::size_t 		_primCount) const;
		void Draw(		GLenum				_primitiveType, 
						const IndirectArrayBuffer& _indirectBuffer) const;
		//----------------------------------------------------------------------
		GLuint 			id;
	};
	//--------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------
// Include inline definitions
//-----------------------------------------------------------------------------
#include <glf/buffer.inl>

#endif
