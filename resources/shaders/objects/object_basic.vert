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



uniform vec4 ambient;
uniform float ambientSrc;
uniform vec4 emissive;
uniform float emissiveSrc;
uniform vec4 diffuse;
uniform float diffuseSrc;
uniform vec4 specular;
uniform float specularSrc;

uniform vec4 globalAmbient;

uniform bool invertVertexAlpha;

uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 normalMatrix;
uniform mat4 invV;

uniform bool useReflectionXform;
uniform bool useRefractionXform;

uniform mat4 uvMatrix;
uniform int uvSource;// normal -1, position -2, relect -3, else
uniform int useTexture;
uniform bvec2 clampedTexture;

uniform bool forceLighting = false;
uniform bool forceNoLighting = false;

// Output: tangent space matrix, position in view space and uv.
out INTERFACE {
	vec4 color;
	vec4 camPos;
	vec4 camNor;
	vec3 uv;
} Out ;


uniform bool noLights;

struct Light {
	vec4 posdir;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 attenuations;
	float scale;
	bool enabled;
};

uniform Light lights[8];

void main(){
	
	vec4 viewPos = mv * vec4(v, 1.0);
	Out.camPos = viewPos;
	Out.camNor = vec4(normalMatrix * n, 1.0);
	
	vec4 clipPos = mvp * vec4(v, 1.0);
	gl_Position = clipPos;
	// TODO: reenable.
	//gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
	
	vec4 vertexCol = col;//.bgra;
	vec4 MAmbient = mix(vertexCol, ambient, ambientSrc);
	vec4 MDiffuse = mix(vertexCol, diffuse, diffuseSrc);
	vec4 MEmissive = mix(vertexCol, emissive, emissiveSrc);
	vec4 MSpecular = mix(vertexCol, specular, specularSrc);
		
	vec4 LAmbient = vec4(0.0);
	vec4 LDiffuse = vec4(0.0);
	
	if(!noLights && !forceNoLighting){
		vec3 NDirection = normalize(Out.camNor.xyz);
		for (int i = 0; i < 8; i++) {
			if(!lights[i].enabled){
				break;
			}
			vec3 v2l = vec3(lights[i].posdir - Out.camPos*lights[i].posdir.w);
			float distance = length(v2l);
			vec3 direction = normalize(v2l);
			float attenuation = mix(1.0, 1.0/(lights[i].attenuations.x+lights[i].attenuations.y*distance + lights[i].attenuations.z * distance * distance), lights[i].posdir.w);
			LAmbient.xyz = LAmbient.xyz + attenuation*(lights[i].ambient*lights[i].scale);
			LDiffuse.xyz = LDiffuse.xyz + MDiffuse.xyz*(lights[i].diffuse*lights[i].scale)*max(0.0, dot(NDirection, direction)*attenuation);
		 	
		}

	}
	
		
	vec4 ambientFinal = forceLighting ? vec4(1.0) : clamp(MAmbient*(globalAmbient+LAmbient), 0.0, 1.0);
	vec4 diffuseFinal = clamp(LDiffuse, 0.0, 1.0);
	vec4 material = clamp(ambientFinal + diffuseFinal + MEmissive, 0.0, 1.0);
		
	float baseAlpha = invertVertexAlpha ? (1.0 - MDiffuse.a) : MDiffuse.a;
		
	Out.color = vec4(material.rgb, baseAlpha);
		

	// Compute UV coordinates.
	if(useTexture>0){
		mat4 matrix;
		if (useReflectionXform || useRefractionXform) {
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
			
			if (useRefractionXform) {
				matrix[0][2] = -matrix[0][2];
				matrix[1][2] = -matrix[1][2];
				matrix[2][2] = -matrix[2][2];
			}
		} else {
			matrix = uvMatrix;
		}
			
			
		vec4 coords;
		switch (uvSource) {
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
				int uvId = uvSource;
				vec3 selectedUV = (uvId == 0 ? uv0 : (uvId == 1 ? uv1 : (uvId == 2 ? uv2 : (uvId == 3 ? uv3 : (uvId == 4 ? uv4 : (uvId == 5 ? uv5 : (uvId == 6 ? uv6 : uv7 )))))));
				coords = matrix * vec4(selectedUV, 1.0);
				break;
		}
			
		coords.x = clampedTexture.x ? clamp(coords.x, 0.0, 0.99) : coords.x;
		coords.y = clampedTexture.y ? clamp(coords.y, 0.0, 0.99) : coords.y;
		
			
		Out.uv = coords.xyz;
	} else {
		Out.uv = vec3(0.0);
	}
	
}
