//-----------------------------------------------------------------------------
#version 410

//-----------------------------------------------------------------------------
//uniform sampler2D		BokehTex;
//------------------------------------------------------------------------------
in  vec2				TexCoord;
out vec4 				FragColor;

//------------------------------------------------------------------------------
void main()
{
	// Shader
	//coherent uniform layout(size1x32) uimage1D counterImage;
	//coherent uniform layout(size1x32) uimage1D positionImage;

	//uint current = imageAtomicIncWrap(counterImage, 0, 1);
	//imageStore(positionImage,current,vec4(pix.x,pix.y,Coc));


//	FragColor = texture(BokehTex,gTexCoord);
	FragColor = vec4(TexCoord,1,1);
}

