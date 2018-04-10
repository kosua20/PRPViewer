#version 330

// Attributes
layout(location = 0) in vec3 v;
layout(location = 1) in vec3 n;
layout(location = 2) in vec2 uv;

// Uniform: the MVP, MV and normal matrices
uniform mat4 mvp;
uniform mat3 normalMatrix;

// Output: tangent space matrix, position in view space and uv.
out INTERFACE {
	vec2 uv;
	vec3 n;
} Out ;


void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.uv = uv;
	Out.n = normalMatrix*n;
}
