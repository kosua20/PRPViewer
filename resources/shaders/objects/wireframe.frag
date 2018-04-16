#version 330


out vec4 fragColor;
uniform vec3 color = vec3(0.0);

void main(){
	
	fragColor.rgb = color;
	fragColor.a = 1.0;
}
