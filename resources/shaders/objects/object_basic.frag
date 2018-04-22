#version 330

// Output: tangent space matrix, position in view space and uv.
in INTERFACE {
	vec4 color[10];
	vec4 camPos;
	vec4 camNor;
	vec3 uv[10];
} In ;
uniform int layerCount;


uniform int useTexture[10];
uniform sampler2D textures[8];
uniform samplerCube cubemaps[8];
uniform bool blendInvertColor[10];
uniform bool blendNoTexColor[10];
uniform bool blendInvertAlpha[10];
uniform bool blendNoVtxAlpha[10];
uniform bool blendNoTexAlpha[10];
uniform bool blendAlphaAdd[10];
uniform bool blendAlphaMult[10];

uniform int blendMode[10]; //1 kBlendAddColorTimesAlpha, 2 kBlendAlpha, 3 kBlendAdd, 4 kBlendMult, 5 kBlendDot3, 6 kBlendAddSigned, 7 kBlendAddSigned2X
uniform float alphaThreshold[10];


out vec4 fragColor;

void main(){
	vec3 fCurrColor = vec3(0.0);
	vec3 fPrevColor = vec3(0.0);
	float fCurrAlpha = 0.0;
	float fPrevAlpha = 0.0;
	/*
	// For each layer...
	for(int i = 0; i < layerCount; ++i){
		float fBaseAlpha = In.color[i].a;
		
		
		
		if (useTexture[i]==0) {
			// passthrough.
			fCurrColor = fPrevColor;
			fCurrAlpha = fPrevAlpha;
			continue;
		}
		
		
		vec4 img = (useTexture[i]==1) ?  texture(textures[i], In.uv[i].xy) : texture(cubemaps[i], In.uv[i].xyz);
		
		// Local variable to store the color value
		
		vec3 texCol = blendInvertColor[i] ? (1.0 - img.rgb) : img.rgb;
		vec3 col; float alpha;
		
		
		if(i == 0){
			if(blendNoTexColor[i]){
				fCurrColor = fPrevColor;
			} else {
				col = texCol;
				fCurrColor = col;
			}
			
			float alphaVal = blendInvertAlpha[i] ? (1.0-img.a) : img.a;
			if(blendNoVtxAlpha[i] || fPrevAlpha == 0.0){
				alpha = alphaVal;
				fCurrAlpha = alpha;
			} else if(blendNoTexAlpha[i]){
				fCurrAlpha = fPrevAlpha;
			} else {
				alpha = alphaVal * fPrevAlpha;
				fCurrAlpha = alpha;
			}
		} else {
			switch (blendMode[i]){
				case 1:
					// Unsupported on upper layers.
					break;
				case 2:
					if (blendNoTexColor[i]) {
						fCurrColor = fPrevColor;
					} else {
						if (blendInvertAlpha[i]) {
							col = texCol + img.a * fPrevColor;
							fCurrColor = col;
						} else {
							col = mix(fPrevColor, texCol, img.a);
							fCurrColor = col;
						}
					}
					
					float alphaVal = blendInvertAlpha[i] ? (1.0 - img.a) : img.a;
					
					
					if (blendAlphaAdd[i]) {
						alpha = alphaVal + fPrevAlpha;
						fCurrAlpha = alpha;
					} else if (blendAlphaMult[i]) {
						alpha = alphaVal * fPrevAlpha;
						fCurrAlpha = alpha;
					} else {
						fCurrAlpha = fPrevAlpha;
					}
					break;
					
				case 3:
					col = texCol + fPrevColor;
					fCurrColor = col;
					fCurrAlpha = fPrevAlpha;
					break;
				case 4:
					col = texCol * fPrevColor;
					fCurrColor = col;
					fCurrAlpha = fPrevAlpha;
					break;
				case 5:
					col = vec3(dot(texCol.rgb, fPrevColor.rgb));
					fCurrColor = col;
					fCurrAlpha = fPrevAlpha;
					break;
				case 6:
					col = texCol+fPrevColor - 0.5;
					fCurrColor = col;
					fCurrAlpha = fPrevAlpha;
					break;
				case 7:
					col = 2*(texCol+fPrevColor - 0.5);
					fCurrColor = col;
					fCurrAlpha = fPrevAlpha;
					break;
				case 0:
					col = texCol;
					fCurrColor = col;
					alpha = img.a;
					fCurrAlpha = alpha;
					break;
			}
		}
		
		// end of layer loop.
		if(fCurrAlpha < alphaThreshold[i]){
			continue;
		}
	
		fPrevColor = fCurrColor;
		fPrevAlpha = fCurrAlpha;
	
	}
	

	vec3 finalColor;
	if(fCurrColor == vec3(0.0)){
		finalColor = In.color[layerCount-1].rgb;
	} else {
		finalColor = In.color[layerCount-1].rgb * fCurrColor;
	}
	
	fragColor = vec4(finalColor, fCurrAlpha);
	 */
	
	
	vec4 img = (useTexture[0]==1) ?  texture(textures[0], In.uv[0].xy) : texture(cubemaps[0], normalize(In.uv[0].xyz));
	
	// Local variable to store the color value
	
	vec3 texCol = In.color[layerCount-1].rgb*(blendInvertColor[0] ? (1.0 - img.rgb) : img.rgb);
	fragColor = vec4(texCol, 1.0);
}
