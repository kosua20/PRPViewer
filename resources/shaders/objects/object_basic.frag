#version 330

// Output: tangent space matrix, position in view space and uv.
in INTERFACE {
	vec4 col;
	vec3 pos;
	vec3 n;
	vec3 uv[8];
} In ;

uniform sampler2D textures[8];
uniform samplerCube cubemaps[8];
uniform int layerCount;
uniform int useTexture[8];
uniform int blendMode[8];

uniform vec3 ambient[8];
uniform vec3 prediff[8];
uniform vec4 diffuse[8];
uniform vec4 specular[8];

out vec4 fragColor;

void main(){
	
	vec4 finalColor = vec4(0.0);
	bool first = true;
	for(int i = 0; i < 1;++i){
		if(useTexture[i]>0 && first){
			vec4 texColor = useTexture[i]==2 ? texture(cubemaps[i], In.uv[i]) : texture(textures[i], In.uv[i].xy);
			finalColor.rgba = texColor.rgba;
			first = false;
		}
	}
	fragColor.rgb = finalColor.rgb;//*In.col.rgb;//
	
	fragColor.a = finalColor.a;//texColor.a;
}
