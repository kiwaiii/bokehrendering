//-----------------------------------------------------------------------------
#version 330

uniform sampler2D   InputTex;
uniform float       Exposure;
out vec4            FragColor;

//------------------------------------------------------------------------------
// Applies the filmic curve from John Hable's presentation
// Use a filmic tone mapping 
// More details at : http://filmicgames.com/archives/75
// Normally, we do not need gamma correction with this tone-map operator !
vec3 ToneMapFilmicALU(vec3 _color)
{
	_color = max(vec3(0), _color - vec3(0.004f));
	_color = (_color * (6.2f*_color + vec3(0.5f))) / (_color * (6.2f * _color + vec3(1.7f)) + vec3(0.06f));

	// Result has 1/2.2 baked in
	return pow(_color, vec3(2.2f));
//	return _color;
}
//------------------------------------------------------------------------------
void main()
{
	vec2  pix 		 = gl_FragCoord.xy/vec2(textureSize(InputTex,0).xy);
	vec3  color		 = textureLod(InputTex, pix, 0).xyz;
	vec3 expoColor	 = color * Exposure;
	vec3 toneColor	 = ToneMapFilmicALU(expoColor);
	FragColor		 = vec4(toneColor,1);
}
