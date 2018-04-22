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



uniform vec4 ambient[10];
uniform float ambientSrc[10];
uniform vec4 emissive[10];
uniform float emissiveSrc[10];
uniform vec4 diffuse[10];
uniform float diffuseSrc[10];
uniform vec4 specular[10];
uniform float specularSrc[10];

uniform bool invertAlpha[10];

uniform vec4 globalAmbient[10];

uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 normalMatrix;
uniform int layerCount;

uniform mat4 invV;
uniform mat4 uvMatrix[10];
uniform int uvSource[10];// normal -1, position -2, relect -3, else
uniform int useTexture[10];
uniform bool clampedTexture[10];
uniform bool useReflectionXform[10];
uniform bool useRefractionXform[10];

// Output: tangent space matrix, position in view space and uv.
out INTERFACE {
	vec4 color[10];
	vec4 camPos;
	vec4 camNor;
	vec3 uv[10];
} Out ;


void main(){
	
	vec4 viewPos = mv * vec4(v, 1.0);
	Out.camPos = viewPos;
	Out.camNor = vec4(normalMatrix * n, 1.0);
	
	vec4 clipPos = mvp * vec4(v, 1.0);
	gl_Position = clipPos;
	gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
	
	for(int i = 0; i < layerCount; ++i){
		vec4 vertexCol = col;//.bgra;
		vec4 MAmbient = mix(vertexCol, ambient[i], ambientSrc[i]);
		vec4 MDiffuse = mix(vertexCol, diffuse[i], diffuseSrc[i]);
		vec4 MEmissive = mix(vertexCol, emissive[i], emissiveSrc[i]);
		vec4 MSpecular = mix(vertexCol, specular[i], specularSrc[i]);
		
		vec4 LAmbient = vec4(0.0);
		vec4 LDiffuse = vec4(0.0);
		
		/*TODO:
		 for (size_t i = 0; i < 8; i++) {
		 vec3 NDirection = normalize(vec3(worldToLocal*vec4(anor,1.0)));
			LampStruct lamp = uLampSources[i];
			
			vec3 v2l = vec3(lamp.position - mL2W*vec4(apos,1.0)*lamp.position.w);
			float distance = length(v2l);
			vec3 direction = normalize(v2l);
			float attenuatuin = mix(1.0, 1.0/(lamp.constAtten+lamp.linAtten*distance + lamp.quadAtten * distance * distance), lamp.position.w);
			LAmbient = LAmbient + attenuation*(lamp.ambient*lamp.scale);
			LDiffuse = LDiffuse + MDiffuse*(lamp.diffuse*lamp.scale)*max(0.0, dot(Ndirection, direction)*attenuation);
			
		}*/
		
		vec4 ambientFinal = clamp(MAmbient*(globalAmbient[i]+LAmbient), 0.0, 1.0);
		vec4 diffuseFinal = clamp(LDiffuse, 0.0, 1.0);
		vec4 material = clamp(ambientFinal + diffuseFinal + MEmissive, 0.0, 1.0);
		
		float baseAlpha = invertAlpha[i] ? (1.0 - MDiffuse.a) : MDiffuse.a;
		
		Out.color[i] = vec4(material.rgb, baseAlpha);
		
		
		// Compute UV coordinates.
		if(useTexture[i]>0){
			mat4 matrix;
			if (useReflectionXform[i] || useRefractionXform[i]) {
				matrix = invV;
				matrix[0][3] = matrix[1][3] = matrix[2][3] = 0.0;
				
				float temp = matrix[1][0];
				matrix[1][0] = matrix[2][0];
				matrix[2][0] = temp;
				
				temp = matrix[1][1];
				matrix[1][1] = matrix[2][1];
				matrix[2][1] = temp;
				
				temp = matrix[1][2];
				matrix[1][2] = matrix[2][2];
				matrix[2][2] = temp;
				
				if (useRefractionXform[i]) {
					matrix[0][2] = -matrix[0][2];
					matrix[1][2] = -matrix[1][2];
					matrix[2][2] = -matrix[2][2];
				}
			} else {
				matrix = uvMatrix[i];
			}
			
			
			vec4 coords;
			switch (uvSource[i]) {
				case -1:
					// Should probably normalize.
					coords = matrix * Out.camNor;
					break;
				case -2:
					coords = matrix * Out.camPos;
					break;
				case -3:
					coords = matrix * invV * reflect(normalize(Out.camPos), normalize(Out.camNor));
					break;
				default:
					int uvId = uvSource[i];
					vec3 selectedUV = (uvId == 0 ? uv0 : (uvId == 1 ? uv1 : (uvId == 2 ? uv2 : (uvId == 3 ? uv3 : (uvId == 4 ? uv4 : (uvId == 5 ? uv5 : (uvId == 6 ? uv6 : uv7 )))))));
					coords = matrix * vec4(selectedUV, 1.0);
					break;
			}
			
			if(clampedTexture[i]){
				coords = clamp(coords, 0.0, 1.0);
			}
			
			Out.uv[i] = coords.xyz;
		}
	}
	
	
	
}
