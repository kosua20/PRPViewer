#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size.
uniform sampler2D screenTexture;

// Output: the fragment color
out vec4 fragColor;


void main(){
	
	fragColor = texture(screenTexture, In.uv);
	if(fragColor.a == 0.0){
		fragColor.rgb = vec3(1.0,0.0,0.0);
	}
}
