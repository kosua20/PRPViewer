#version 330

in vec2 screenPos;


out vec4 fragColor;

void main(){
	
	float len = length(screenPos);
	if(len>0.01){
		discard;
	} else if(len > 0.008){
		fragColor.rgb = vec3(1.0);
	} else {
		fragColor.rgb = vec3(0.0);
	}
	
	fragColor.a = 1.0;
}
