#version 330

// Attributes
layout(location = 0) in vec3 v;

uniform mat4 mvp;


uniform vec2 screenSize;
out vec2 screenPos;

void main(){
	gl_Position = mvp * vec4(v, 1.0);
	screenPos = gl_Position.xy / gl_Position.w * vec2(1.0, screenSize.y/screenSize.x);
}
