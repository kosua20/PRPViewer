#version 330

// Output: tangent space matrix, position in view space and uv.
in INTERFACE {
	vec2 uv;
	vec3 n;
} In ;

out vec3 fragColor;

void main(){
	fragColor = normalize(In.n)*0.5+0.5; 
}
