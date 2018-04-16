#version 330

// Attributes
layout(location = 0) in vec3 v;

uniform mat4 mvp;

void main(){
	gl_Position = mvp * vec4(v, 1.0);
}
