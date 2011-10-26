#version 410
#extension GL_NV_gpu_shader5 : enable
#extension GL_EXT_shader_image_load_store : enable

layout(size1x32) coherent uniform uimage1D 	BokehCountTex;
out vec4 FragColor;

void main()
{
	imageStore(BokehCountTex,1,uvec4(0));
	FragColor = vec4(0,0,0,0);
}
