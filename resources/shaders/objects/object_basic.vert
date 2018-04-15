#version 330

// Attributes
layout(location = 0) in vec3 v;
layout(location = 1) in vec3 n;
layout(location = 2) in vec4 col;
layout(location = 3) in vec3 uv0;
layout(location = 4) in vec3 uv1;
layout(location = 5) in vec3 uv2;
layout(location = 6) in vec3 uv3;
layout(location = 7) in vec3 uv4;
layout(location = 8) in vec3 uv5;
layout(location = 9) in vec3 uv6;
layout(location = 10) in vec3 uv7;

// Uniform: the MVP, MV and normal matrices
uniform mat4 mvp;
uniform mat3 normalMatrix;
uniform int uvId = 0;

// Output: tangent space matrix, position in view space and uv.
out INTERFACE {
	vec4 col;
	vec3 pos;
	vec3 uv;
	vec3 n;
} Out ;


void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.col = col;
	Out.pos = v;
	Out.uv = (uvId == 0 ? uv0 : (uvId == 1 ? uv1 : (uvId == 2 ? uv2 : (uvId == 3 ? uv3 : (uvId == 4 ? uv4 : (uvId == 5 ? uv5 : (uvId == 6 ? uv6 : uv7 )))))));
	Out.n = n;
}
