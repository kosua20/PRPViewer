#version 330

// Output: tangent space matrix, position in view space and uv.
in INTERFACE {
	vec3 pos;
	vec2 uv;
	vec3 n;
} In ;

uniform sampler2D texture0;
uniform samplerCube cubemap1;
uniform bool useCubemap;

out vec4 fragColor;

void main(){
	
	fragColor = useCubemap ? texture(cubemap1, In.pos) : texture(texture0, In.uv);//normalize(In.n)*0.5+0.5;
}
