#version 330

// Output: tangent space matrix, position in view space and uv.
in INTERFACE {
	vec4 color;
	vec4 camPos;
	vec4 camNor;
	vec3 uv;
} In ;
uniform int layerCount;


uniform int useTexture;
uniform sampler2D textures;
uniform samplerCube cubemaps;
uniform bool blendInvertColor;
uniform bool blendNoTexColor;
uniform bool blendInvertAlpha;
uniform bool blendNoVtxAlpha;
uniform bool blendNoTexAlpha;
uniform bool blendAlphaAdd;
uniform bool blendAlphaMult;

uniform int blendMode; //1 kBlendAddColorTimesAlpha, 2 kBlendAlpha, 3 kBlendAdd, 4 kBlendMult, 5 kBlendDot3, 6 kBlendAddSigned, 7 kBlendAddSigned2X
uniform float alphaThreshold;


out vec4 fragColor;

void main(){
	vec3 fCurrColor = vec3(0.0);
	vec3 fPrevColor = vec3(0.0);
	float fCurrAlpha = 0.0;
	float fPrevAlpha = 0.0;
	
	// For each layer...
	//for(int i = 0; i < layerCount; ++i){
	
		float fBaseAlpha = In.color.a;
		
		
		
		if (useTexture==0) {
			// passthrough.
			fCurrColor = fPrevColor;
			fCurrAlpha = fPrevAlpha;
			
		} else {
		
		
		vec4 img = (useTexture==1) ?  texture(textures, In.uv.xy) : texture(cubemaps, In.uv.xyz);
		
		// Local variable to store the color value
		
		vec3 texCol = blendInvertColor ? (1.0 - img.rgb) : img.rgb;
		vec3 col; float alpha;
		
		
		//if(layerCount == 0){
		if(blendNoTexColor){
			fCurrColor = fPrevColor;
		} else {
			col = texCol;
			fCurrColor = col;
		}
		
		float alphaVal = blendInvertAlpha ? (1.0-img.a) : img.a;
		if(blendNoVtxAlpha || fPrevAlpha == 0.0){
			alpha = alphaVal;
			fCurrAlpha = alpha;
		} else if(blendNoTexAlpha){
			fCurrAlpha = fPrevAlpha;
		} else {
			alpha = alphaVal * fPrevAlpha;
			fCurrAlpha = alpha;
		}
		/*} else {
			switch (blendMode){
				case 1:
					// Unsupported on upper layers.
					break;
				case 2:
					if (blendNoTexColor) {
						fCurrColor = fPrevColor;
					} else {
						if (blendInvertAlpha) {
							col = texCol + img.a * fPrevColor;
							fCurrColor = col;
						} else {
							col = mix(fPrevColor, texCol, img.a);
							fCurrColor = col;
						}
					}
					
					float alphaVal = blendInvertAlpha ? (1.0 - img.a) : img.a;
					
					
					if (blendAlphaAdd) {
						alpha = alphaVal + fPrevAlpha;
						fCurrAlpha = alpha;
					} else if (blendAlphaMult) {
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
			}*/
		}
		
		// end of layer loop.
		if(fCurrAlpha < alphaThreshold){
			discard;
		}
		fPrevColor = fCurrColor;
		fPrevAlpha = fCurrAlpha;
		
	
	//}
	

	vec3 finalColor;
	if(fCurrColor == vec3(0.0)){
		finalColor = In.color.rgb;
	} else {
		finalColor = In.color.rgb * fCurrColor;
	}
	
	fragColor = vec4(finalColor, fCurrAlpha);
	
	
	fragColor = vec4(normalize(In.camNor.xyz)*0.5+0.5, 1.0);
	//vec4 img = (useTexture==1) ?  texture(textures[0], In.uv[0].xy) : texture(cubemaps[0], normalize(In.uv[0].xyz));
	
	// Local variable to store the color value
	
	//vec3 texCol = In.color[layerCount-1].rgb*(blendInvertColor[0] ? (1.0 - img.rgb) : img.rgb);
	//fragColor = vec4(texCol, 1.0);
}
