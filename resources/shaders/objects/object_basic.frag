#version 330

// Output: tangent space matrix, position in view space and uv.
in INTERFACE {
	vec4 color;
	vec4 camPos;
	vec4 camNor;
	vec3 uv;
} In ;


uniform int useTexture;
uniform sampler2D textures;
uniform samplerCube cubemaps;

uniform bool blendInvertColor;
uniform bool blendInvertAlpha;

uniform bool blendNoTexColor;
uniform bool blendNoVtxAlpha;
uniform bool blendNoTexAlpha;

//uniform bool blendAlphaAdd;
//uniform bool blendAlphaMult;

uniform float alphaThreshold;
uniform bool forceVertexColor = false;

uniform bool fogEnabled = false;
uniform int fogMode = 3;
uniform vec3 fogInfos = vec3(-1500.0, 2000.0, 1.0);
uniform vec3 fogColor = vec3(0.4, 0.3, 0.1);



out vec4 fragColor;

void main(){
	vec3 fCurrColor = vec3(0.0);
	float fCurrAlpha = 0.0;
	
	if (useTexture==0 || forceVertexColor) {
		// passthrough.
		// discard;
		fCurrColor = In.color.rgb;
		fCurrAlpha = In.color.a;
	} else {
		
		vec4 img = (useTexture==1) ?  texture(textures, In.uv.xy) : texture(cubemaps, normalize(In.uv.xyz));
		vec3 texColor = blendInvertColor ? (1.0 - img.rgb) : img.rgb;
		float texAlpha = blendInvertAlpha ? (1.0 - img.a) : img.a;
		// Vertex alpha inversion is handled in the vertex shader.
		
		if(blendNoVtxAlpha){
			fCurrAlpha = texAlpha;
		} else if(blendNoTexAlpha){
			fCurrAlpha = In.color.a;
		} else {
			fCurrAlpha = In.color.a * texAlpha;
		}
		
		if(blendNoTexColor){
			fCurrColor = In.color.rgb;
		} else {
			fCurrColor = In.color.rgb * texColor;
		}

	}
		
	// end of layer loop.
	if(fCurrAlpha < alphaThreshold){
		// if the test doesn't pass, discard.
		discard;
	}

	if(fogEnabled){
		if(fogMode == 0){
			float fogFactor = (fogInfos.y - length(In.camPos.xyz))/(fogInfos.y - fogInfos.x); 
			fCurrColor = mix(fogColor, fCurrColor, fogFactor); 
		} else if (fogMode == 1){
			float d = length(In.camPos.xyz) * fogInfos.z;
			float fogFactor = 1.0 / exp(d); 
			fCurrColor = mix(fogColor, fCurrColor, fogFactor); 
		} else if (fogMode == 2){
			float d = length(In.camPos.xyz) * fogInfos.z;
			float fogFactor = 1.0 / exp(d*d); 
			fCurrColor = mix(fogColor, fCurrColor, fogFactor); 
		}
	}
	

	fragColor = vec4(fCurrColor, fCurrAlpha);
	
	
}
