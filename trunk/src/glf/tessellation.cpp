//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glf/tessellation.hpp>
#include <gli/io.hpp>

namespace glf
{
	//-------------------------------------------------------------------------
	TerrainGeometry::Ptr TerrainGeometry::Create()
	{
		return TerrainGeometry::Ptr(new TerrainGeometry());
	}
	//-------------------------------------------------------------------------
	TerrainGeometry::TerrainGeometry():
	program("TerrainShader")
	{
		program.Compile(	glf::LoadFile("../resources/shaders/terrain.vs"),
							glf::LoadFile("../resources/shaders/terrain.cs"),
							glf::LoadFile("../resources/shaders/terrain.es"),
							glf::LoadFile("../resources/shaders/terrain.fs"));

		vbufferVar		= program["Position"].location;
		projVar 		= program["Projection"].location;
		viewVar 		= program["View"].location;
		view2Var 		= program["View2"].location;
		modelVar 		= program["Model"].location;
		tileSizeVar 	= program["TileSize"].location;
		tileCountVar	= program["TileCount"].location;
		heightTexUnit	= program["HeightTex"].unit;
		normalTexUnit	= program["NormalTex"].unit;
		tessFactorVar	= program["TessFactor"].location;
		depthFactorVar	= program["DepthFactor"].location;
		projFactorVar	= program["ProjFactor"].location;

		glProgramUniform1i(program.id,	program["HeightTex"].location, heightTexUnit);
		glProgramUniform1i(program.id,	program["NormalTex"].location, normalTexUnit);

		glm::vec2* vertices;
		vbuffer.Resize(4);
		vertices = vbuffer.Lock();
		vertices[0] = glm::vec2(0,0);
		vertices[1] = glm::vec2(1,0);
		vertices[2] = glm::vec2(1,1);
		vertices[3] = glm::vec2(0,1);
		vbuffer.Unlock();

		tileCount 		= glm::vec2(8,8);
		tileSize  		= glm::ivec2(1,1);
		depthFactor		= 0.2f;
		tessFactor 		= 16.f;
		projFactor 		= 10.f;

		gli::Image img;
//		gli::io::Load("../resources/textures/normalmap.png",img);
		gli::io::Load("../resources/textures/bricks_red.png",img);
//		gli::io::Load("../resources/textures/bricks_red_normal.png",img);
//		gli::io::Load("../resources/textures/pebbles.png",img);
		assert(img.Format()==gli::PixelFormat::RGB);
		assert(img.Type()==gli::PixelFormat::NCHAR);
		normalTex.Allocate(GL_SRGB8,img.Width(),img.Height());
		normalTex.Fill(GL_RGB,GL_UNSIGNED_BYTE,img.Raw());

//		gli::io::Load("../resources/textures/heightmap.png",img);
		gli::io::Load("../resources/textures/bricks_red_displacement.png",img);
//		gli::io::Load("../resources/textures/pebbles_displmap.png",img);
		assert(img.Format()==gli::PixelFormat::RGB);
		assert(img.Type()==gli::PixelFormat::NCHAR);
		heightTex.Allocate(GL_SRGB8,img.Width(),img.Height());
		heightTex.Fill(GL_RGB,GL_UNSIGNED_BYTE,img.Raw());

		assert(glf::CheckError("TerrainGeometry::TerrainGeometry"));
	}
	//-------------------------------------------------------------------------
	void TerrainGeometry::Draw(	const glm::mat4& _proj,
								const glm::mat4& _view,
								const glm::mat4& _model,
								bool _drawWireframe,
								bool _drawFixView)
	{
		if(_drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		if(!_drawFixView)  fixViewMat = _view;

		// Render lighting pass
		glUseProgram(program.id);
		glProgramUniform2i(program.id, 	  	  tileCountVar, 	tileCount.x, tileCount.y);
		glProgramUniform2f(program.id, 	  	  tileSizeVar, 		tileSize.x, tileSize.y);
		glProgramUniformMatrix4fv(program.id, projVar,  		1, GL_FALSE, &_proj[0][0]);
		glProgramUniformMatrix4fv(program.id, viewVar,  		1, GL_FALSE, &_view[0][0]);
		glProgramUniformMatrix4fv(program.id, view2Var,  		1, GL_FALSE, &fixViewMat[0][0]);
		glProgramUniformMatrix4fv(program.id, modelVar, 		1, GL_FALSE, &_model[0][0]);
		glProgramUniform1f(program.id, 	  	  tessFactorVar,	tessFactor);
		glProgramUniform1f(program.id, 	  	  depthFactorVar, 	depthFactor);
		glProgramUniform1f(program.id, 	  	  projFactorVar, 	projFactor);

		heightTex.Bind(heightTexUnit);
		normalTex.Bind(normalTexUnit);

		glBindBuffer(GL_ARRAY_BUFFER, vbuffer.id);
		glVertexAttribPointer(	vbufferVar, 
								2, 
								GL_FLOAT, 
								false,
								sizeof(glm::vec2),
								GLF_BUFFER_OFFSET(0));
		glEnableVertexAttribArray(vbufferVar);
		glPatchParameteri(GL_PATCH_VERTICES, 4);
		glDrawArraysInstanced(GL_PATCHES, 0, 4, tileCount.x*tileCount.y);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if(_drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		assert(glf::CheckError("TerrainGeometry::Draw"));
	}
	//-------------------------------------------------------------------------
	PhongGeometry::Ptr PhongGeometry::Create()
	{
		return PhongGeometry::Ptr(new PhongGeometry());
	}
	//-------------------------------------------------------------------------
	PhongGeometry::PhongGeometry():
	program("PhongShader")
	{
		program.Compile(	glf::LoadFile("../resources/shaders/phongtessellation.vs"),
							glf::LoadFile("../resources/shaders/phongtessellation.cs"),
							glf::LoadFile("../resources/shaders/phongtessellation.es"),
							glf::LoadFile("../resources/shaders/phongtessellation.fs"));

		vbufferVar		= program["Position"].location;
		nbufferVar		= program["Normal"].location;
		projVar 		= program["Projection"].location;
		viewVar 		= program["View"].location;
		modelVar 		= program["Model"].location;
		tessFactorVar	= program["TessFactor"].location;
		curvFactorVar	= program["CurvFactor"].location;

		glm::vec3* vertices;
		vbuffer.Resize(3);
		vertices = vbuffer.Lock();
		vertices[0] = glm::vec3(0,0,0);
		vertices[1] = glm::vec3(1,0,0);
		vertices[2] = glm::vec3(1,1,0);
		vbuffer.Unlock();

		glm::vec3* normals;
		nbuffer.Resize(3);
		normals 	= nbuffer.Lock();
		normals[0] 	= glm::normalize(glm::vec3(-1,-1, 1));
		normals[1] 	= glm::normalize(glm::vec3( 1,-1, 1));
		normals[2] 	= glm::normalize(glm::vec3( 1, 1, 1));
		nbuffer.Unlock();

		tessFactor 		= 16.f;
		curvFactor		= 3.f / 4.f;

		assert(glf::CheckError("PhongGeometry::TerrainGeometry"));
	}
	//-------------------------------------------------------------------------
	void PhongGeometry::Draw(	const glm::mat4& _proj,
								const glm::mat4& _view,
								const glm::mat4& _model,
								bool _drawWireframe)
	{
		if(_drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		// Render lighting pass
		glUseProgram(program.id);
		glProgramUniformMatrix4fv(program.id, projVar,  		1, GL_FALSE, &_proj[0][0]);
		glProgramUniformMatrix4fv(program.id, viewVar,  		1, GL_FALSE, &_view[0][0]);
		glProgramUniformMatrix4fv(program.id, modelVar, 		1, GL_FALSE, &_model[0][0]);
		glProgramUniform1f(program.id, 	  	  tessFactorVar,	tessFactor);
		glProgramUniform1f(program.id, 	  	  curvFactorVar, 	curvFactor);

		glBindBuffer(GL_ARRAY_BUFFER, vbuffer.id);
		glVertexAttribPointer(	vbufferVar, 
								3, 
								GL_FLOAT, 
								false,
								sizeof(glm::vec3),
								GLF_BUFFER_OFFSET(0));
		glEnableVertexAttribArray(vbufferVar);
		glBindBuffer(GL_ARRAY_BUFFER, nbuffer.id);
		glVertexAttribPointer(	nbufferVar, 
								3, 
								GL_FLOAT, 
								false,
								sizeof(glm::vec3),
								GLF_BUFFER_OFFSET(0));
		glEnableVertexAttribArray(nbufferVar);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		//glPatchParameteri(GL_PATCH_VERTICES, 3);
		glDrawArrays(GL_PATCHES, 0, vbuffer.Size());
		glDisableVertexAttribArray(vbufferVar);
		glDisableVertexAttribArray(nbufferVar);

		if(_drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		assert(glf::CheckError("PhongGeometry::Draw"));
	}
	//-------------------------------------------------------------------------
	void ToPhongGeometry(	const std::vector<Object::Ptr>& _objects,
							std::vector<PhongGeometry::Ptr>& _phongs)
	{
		for(unsigned int i=0;i<_objects.size();++i)
		{
			PhongGeometry::Ptr phong = PhongGeometry::Create();

			phong->vbuffer.Resize(_objects[i]->positions->Size());
			glm::vec3* vdst = phong->vbuffer.Lock();
			glm::vec3* vsrc = _objects[i]->positions->Lock();
			for(int j=0;j<_objects[i]->positions->Size();++j)
			{
				vdst[j] = vsrc[j];
			}
			phong->vbuffer.Unlock();
			_objects[i]->positions->Unlock();

			phong->nbuffer.Resize(_objects[i]->normals->Size());
			glm::vec3* ndst = phong->nbuffer.Lock();
			glm::vec3* nsrc = _objects[i]->normals->Lock();
			for(int j=0;j<_objects[i]->normals->Size();++j)
			{
				ndst[j] = nsrc[j];
			}
			phong->nbuffer.Unlock();
			_objects[i]->normals->Unlock();	

			_phongs.push_back(phong);
		}
	}
}

