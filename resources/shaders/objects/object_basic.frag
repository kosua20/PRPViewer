#version 330

// Output: tangent space matrix, position in view space and uv.
in INTERFACE {
	vec4 col;
	vec3 pos;
	vec3 uv;
	vec3 n;
} In ;

uniform sampler2D texture0;
uniform samplerCube cubemap1;
uniform bool useCubemap;

out vec4 fragColor;

void main(){
	
	vec4 texColor = useCubemap ? texture(cubemap1, In.uv) : texture(texture0, In.uv.xy);
	fragColor.rgb = texColor.rgb;////In.col.rgb;//normalize(In.n)*0.5+0.5;
	fragColor.a = 1.0;//texColor.a;
}
