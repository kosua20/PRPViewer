#include "Age.hpp"
#include "helpers/Logger.hpp"
#include "resources/ResourcesManager.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <ResManager/plResManager.h>
#include <Debug/hsExceptions.hpp>
#include <Stream/hsStdioStream.h>
#include <Stream/hsStream.h>
#include <PRP/plSceneNode.h>
#include <PRP/Object/plSceneObject.h>
#include <PRP/Object/plDrawInterface.h>
#include <PRP/Object/plCoordinateInterface.h>
#include <PRP/Geometry/plDrawableSpans.h>
#include <PRP/Animation/plViewFaceModifier.h>
#include <PRP/Surface/plLayer.h>
#include <PRP/Surface/plBitmap.h>
#include <PRP/Surface/plMipmap.h>
#include <PRP/Surface/plCubicEnvironmap.h>
#include <Stream/plEncryptedStream.h>
#include <Debug/plDebug.h>
#include <Util/plPNG.h>
#include <Util/plJPEG.h>
#include <string_theory/stdio>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <fstream>

unsigned int  kLiteMask = (plSpan::kLiteMaterial | plSpan::kLiteVtxPreshaded | plSpan::kLiteVtxNonPreshaded | plSpan::kLiteProjection | plSpan::kLiteShadowErase | plSpan::kLiteShadow);

Age::Age(const std::string & path){
	
	const PlasmaVer plasmaVersion = PlasmaVer::pvMoul;
	_rm.reset(new plResManager());
	//plResManager _rm;
	
	plAgeInfo* age = _rm->ReadAge(path, true);
	
	_name = age->getAgeName().to_std_string();
	
	Log::Info() << "Age " << _name << ":" << std::endl;
	
	const size_t pageCount = age->getNumPages();
	Log::Info() << pageCount << " pages." << std::endl;
	for(int i = 0 ; i < pageCount; ++i){
		Log::Info() << "Page: " << age->getPageFilename(i, plasmaVersion) << " ";
		const plLocation ploc = age->getPageLoc(i, plasmaVersion);
		loadMeshes(*_rm, ploc);
		Log::Info() << std::endl;
	}
	Log::Info() << std::endl;
	
	const size_t commmonCount = age->getNumCommonPages(plasmaVersion);
	Log::Info() << commmonCount << " common pages." << std::endl;
	for(int i = 0 ; i < commmonCount; ++i){
		Log::Info() << "Page: " << age->getCommonPageFilename(i, plasmaVersion) << " ";
		const plLocation ploc = age->getCommonPageLoc(i, plasmaVersion);
		loadMeshes(*_rm, ploc);
		Log::Info() << std::endl;
	}
	Log::Info() << std::endl;
	
	checkGLError();
	Log::Info() << "Objects: " << _objects.size() << std::endl;
	//_rm.DelAge(age->getAgeName());
	
	
}

Age::~Age(){

	for(const auto & obj : _objects){
		obj->clean();
	}
}
unsigned char reverse(unsigned char b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

std::shared_ptr<ProgramInfos> Age::generateShaders(hsGMaterial * mat){
	std::vector<std::string> vertexLines = { "#version 330" };
	std::vector<std::string> fragmentLines = { "#version 330" };
	
	
	vertexLines.emplace_back("layout(location = 0) in vec3 v;");
	vertexLines.emplace_back("layout(location = 1) in vec3 n;");
	vertexLines.emplace_back("layout(location = 2) in vec4 col;");
	/*layout(location = 3) in vec3 uv0;
	layout(location = 4) in vec3 uv1;
	layout(location = 5) in vec3 uv2;
	layout(location = 6) in vec3 uv3;
	layout(location = 7) in vec3 uv4;
	layout(location = 8) in vec3 uv5;
	layout(location = 9) in vec3 uv6;
	layout(location = 10) in vec3 uv7;
	*/
	
	/*
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
	*/
	vertexLines.emplace_back("uniform mat4 mvp;");
	vertexLines.emplace_back("uniform mat4 mv;");
	vertexLines.emplace_back("uniform mat3 normalMatrix;");
	//uniform int layerCount;
	
	vertexLines.emplace_back("uniform mat4 invV;");
	/*uniform mat4 uvMatrix[10];
	uniform int uvSource[10];// normal -1, position -2, relect -3, else
	uniform int useTexture[10];
	uniform bool clampedTexture[10];
	uniform bool useReflectionXform[10];
	uniform bool useRefractionXform[10];*/
	
	// Output: tangent space matrix, position in view space and uv.
	vertexLines.emplace_back("out INTERFACE {");
	vertexLines.emplace_back("	vec4 color;");
	vertexLines.emplace_back("	vec4 camPos;");
	vertexLines.emplace_back("	vec4 camNor;");
	//vertexLines.emplace_back("	vec3 uv[10];
	vertexLines.emplace_back("} Out ;");
	
	
	vertexLines.emplace_back("void main(){");
		
	vertexLines.emplace_back("vec4 viewPos = mv * vec4(v, 1.0);");
	vertexLines.emplace_back("Out.camPos = viewPos;");
	vertexLines.emplace_back("Out.camNor = vec4(normalMatrix * n, 1.0);");
	
	vertexLines.emplace_back("vec4 clipPos = mvp * vec4(v, 1.0);");
	vertexLines.emplace_back("gl_Position = clipPos;");
	vertexLines.emplace_back("gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;");
	vertexLines.emplace_back("Out.color = col;");
		/*
		for(int i = 0; i < layerCount; ++i){
			vec4 vertexCol = col;//.bgra;
			vec4 MAmbient = mix(vertexCol, ambient[i], ambientSrc[i]);
			vec4 MDiffuse = mix(vertexCol, diffuse[i], diffuseSrc[i]);
			vec4 MEmissive = mix(vertexCol, emissive[i], emissiveSrc[i]);
			vec4 MSpecular = mix(vertexCol, specular[i], specularSrc[i]);
			
			vec4 LAmbient = vec4(0.0);
			vec4 LDiffuse = vec4(0.0);
			
			//TODO:
			 //for (size_t i = 0; i < 8; i++) {
			 //vec3 NDirection = normalize(vec3(worldToLocal*vec4(anor,1.0)));
			 //LampStruct lamp = uLampSources[i];
			 
			 vec3 v2l = vec3(lamp.position - mL2W*vec4(apos,1.0)*lamp.position.w);
			 float distance = length(v2l);
			 vec3 direction = normalize(v2l);
			 float attenuatuin = mix(1.0, 1.0/(lamp.constAtten+lamp.linAtten*distance + lamp.quadAtten * distance * distance), lamp.position.w);
			 LAmbient = LAmbient + attenuation*(lamp.ambient*lamp.scale);
			 LDiffuse = LDiffuse + MDiffuse*(lamp.diffuse*lamp.scale)*max(0.0, dot(Ndirection, direction)*attenuation);
			 
			 }
			
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
		
		*/
	vertexLines.emplace_back("}");
	
	// Output: tangent space matrix, position in view space and uv.
	fragmentLines.emplace_back("in INTERFACE {");
	fragmentLines.emplace_back("	vec4 color;");
	fragmentLines.emplace_back("	vec4 camPos;");
	fragmentLines.emplace_back("	vec4 camNor;");
	//	fragmentLines.emplace_back("vec3 uv[10];
	fragmentLines.emplace_back("} In ;");
	/*uniform int layerCount;
	
	
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
	uniform float alphaThreshold[10];*/
	
	
	fragmentLines.emplace_back("out vec4 fragColor;");
	
	fragmentLines.emplace_back("void main(){");
		//vec3 fCurrColor = vec3(0.0);
		//vec3 fPrevColor = vec3(0.0);
		//float fCurrAlpha = 0.0;
		//float fPrevAlpha = 0.0;
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
		
		
		//vec4 img = (useTexture[0]==1) ?  texture(textures[0], In.uv[0].xy) : texture(cubemaps[0], normalize(In.uv[0].xyz));
		
		// Local variable to store the color value
		
		//vec3 texCol = In.color[layerCount-1].rgb*(blendInvertColor[0] ? (1.0 - img.rgb) : img.rgb);
	fragmentLines.emplace_back("fragColor = In.color;");
	fragmentLines.emplace_back("}");
	
	std::string programName = "default";
	std::string vertexContent;
	for(const auto & line : vertexLines){
		vertexContent.append(line);
		vertexContent.append("\n");
	}
	std::string fragmentContent;
	for(const auto & line : fragmentLines){
		fragmentContent.append(line);
		fragmentContent.append("\n");
	}
	return Resources::manager().registerProgram(programName, vertexContent, fragmentContent);
}

void Age::loadMeshes(plResManager & rm, const plLocation& ploc){
	plSceneNode* scene = rm.getSceneNode(ploc);
	
	if(scene){
		int i = 0;
		for(const auto & objKey : scene->getSceneObjects()){
			
			plSceneObject* obj = plSceneObject::Convert(rm.getObject(objKey));
			
			if(objKey->getName().substr(0, 11) == "LinkInPoint" && obj->getCoordInterface().Exists()){
				plCoordinateInterface* coord = plCoordinateInterface::Convert(obj->getCoordInterface()->getObj());
				const auto matCenter = coord->getLocalToWorld();
				if(objKey->getName() == "LinkInPointDefault"){
					_linkingNamesCache.insert( _linkingNamesCache.begin(), objKey->getName().substr(11).to_std_string());
					_linkingPoints[_linkingNamesCache.front()] = glm::vec3(matCenter(0,3), matCenter(2,3)+ 5.0f, -matCenter(1,3));
				} else {
					_linkingNamesCache.push_back(objKey->getName().substr(11).to_std_string());
					_linkingPoints[_linkingNamesCache.back()] = glm::vec3(matCenter(0,3), matCenter(2,3)+ 5.0f, -matCenter(1,3));
				}
			}
			
			if(!obj->getDrawInterface().Exists()){
				continue;
			}
			plDrawInterface* draw = plDrawInterface::Convert(obj->getDrawInterface()->getObj());
			
			Object::Type type = Object::Default;
			
			for(const auto & modKey : obj->getModifiers()){
				if(modKey->getType() == kViewFaceModifier){
					plViewFaceModifier* modif = plViewFaceModifier::Convert(modKey->getObj());
					if(modif->getFlag(plViewFaceModifier::kPivotY)){
						type = Object::BillboardY;
					} else {
						type = Object::Billboard;
					}
				}
			}
			
			glm::mat4 model = glm::mat4(1.0f);
			bool bakePosition = !obj->getCoordInterface().Exists();
			if(!bakePosition ){
				plCoordinateInterface* coord = plCoordinateInterface::Convert(obj->getCoordInterface()->getObj());
				hsMatrix44 localToWorld = coord->getLocalToWorld();
				model[0][0] = localToWorld(0,0); model[0][1] = localToWorld(1,0); model[0][2] = localToWorld(2,0); model[0][3] = localToWorld(3,0);
				model[1][0] = localToWorld(0,1); model[1][1] = localToWorld(1,1); model[1][2] = localToWorld(2,1); model[1][3] = localToWorld(3,1);
				model[2][0] = localToWorld(0,2); model[2][1] = localToWorld(1,2); model[2][2] = localToWorld(2,2); model[2][3] = localToWorld(3,2);
				model[3][0] = localToWorld(0,3); model[3][1] = localToWorld(1,3); model[3][2] = localToWorld(2,3); model[3][3] = localToWorld(3,3);
				
				
			}
			// Swap axis.
			model = glm::rotate(glm::mat4(1.0f), -float(M_PI_2), glm::vec3(1.0f,0.0f,0.0f)) * model;
//			++i;
//			if(i<74){
//				continue;
//			}
			//Log::Info() << objKey->getName() << std::endl;
			//generateShaders(mat);
			_objects.emplace_back(new Object(type, Resources::manager().getProgram("object_basic"), model, objKey->getName().to_std_string()));
			
			
			
			Log::Info() << objKey->getName() << std::endl;
			
			for (size_t i = 0; i < draw->getNumDrawables(); ++i) {
				if (draw->getDrawableKey(i) == -1){
					continue;
				}
			
				// A span group multiple objects data/assets.
				plDrawableSpans* span = plDrawableSpans::Convert(draw->getDrawable(i)->getObj());
				plDISpanIndex di = span->getDIIndex(draw->getDrawableKey(i));
				
				// Ignore matrix-only span element. probably internal nodes ?
				if ((di.fFlags & plDISpanIndex::kMatrixOnly) != 0){
					continue;
				}
					
				for(size_t id = 0; id < di.fIndices.size(); ++id){
					const std::string fileName = scene->getKey()->getName().to_std_string() + "_" + obj->getKey()->getName().to_std_string() + "_" + std::to_string(i) + "_" + std::to_string(id) ;

					// Get the mesh internal representation.
					plIcicle* ice = span->getIcicle(di.fIndices[id]);
					const hsMatrix44 transfoMatrix = (bakePosition ? ice->getLocalToWorld() : hsMatrix44::Identity());
					//Log::Info() << (ice->getWorldBounds().getMins()+ice->getWorldBounds().getMaxs())*0.5f << std::endl;
					//Log::Info() << (ice->getWorldBounds().getCorner()) << std::endl;
					// if we have to bake the icicle specific position, use it.
					/*auto refbounds = ice->getWorldBounds();
					refbounds.updateCenter();
					
					hsBounds3Ext bounds;
					bounds.setType(hsBounds3Ext::kAxisAligned);
					glm::vec4 resMax = model * glm::vec4(refbounds.getMaxs().X, refbounds.getMaxs().Y, refbounds.getMaxs().Z, 1.0f);
					glm::vec4 resMin = model * glm::vec4(refbounds.getMins().X, refbounds.getMins().Y, refbounds.getMins().Z, 1.0f);
					bounds.setMaxs(hsVector3(resMax.x, resMax.y, resMax.z));
					bounds.setMins(hsVector3(resMin.x, resMin.y, resMin.z));
					bounds.updateCenter();
					Log::Info() << bounds.getCenter() << std::endl;*/
					
					std::vector<plGBufferVertex> verts = span->getVerts(ice);
					std::vector<unsigned short> indices = span->getIndices(ice);
					const unsigned int uvCount = span->getBuffer(ice->getGroupIdx())->getNumUVs();
					Log::Info() << "UV Count" << uvCount << std::endl;
					std::vector<unsigned int> meshIndices(indices.size());
					std::vector<glm::vec3> meshPositions(verts.size());
					std::vector<glm::vec3> meshNormals(verts.size());
					std::vector<std::vector<glm::vec3>> meshUVs(uvCount, std::vector<glm::vec3>(verts.size()));
					std::vector<glm::u8vec4> meshColors(verts.size());
					
					for (size_t j = 0; j < indices.size(); ++j) {
						meshIndices[j] = (unsigned int)(indices[j]);
					}
					
					
					for (size_t j = 0; j < verts.size(); j++) {
						const hsVector3 pos = transfoMatrix.multPoint(verts[j].fPos);
						const auto fnorm = verts[j].fNormal;
						meshPositions[j] = glm::vec3(pos.X, pos.Y, pos.Z);
						meshNormals[j] = glm::vec3(fnorm.X, fnorm.Y, fnorm.Z);
						const unsigned int color = verts[j].fColor;
						unsigned char red = char((color >> 16) & 0xFF);
						unsigned char gre = char((color >> 8) & 0xFF);
						unsigned char blu = char((color) & 0xFF);
						unsigned char alp = char((color >> 24) & 0xFF);
						meshColors[j] = glm::u8vec4(red, gre, blu, alp);
						//Log::Info() << std::bitset<32>(color) << ", ";
						
						for(size_t uvid = 0; uvid < uvCount; ++uvid){
							const hsVector3 uvCoords = verts[j].fUVWs[uvid];
							meshUVs[uvid][j] = glm::vec3(uvCoords.X, uvCoords.Y, uvCoords.Z);
							
						}
					}
					
					Log::Info() << std::endl;
					
					
					plKey matKey = span->getMaterials()[ice->getMaterialIdx()];
					
					std::string textureName = "";
					
					const unsigned int iceflags = span->getProps();//ice->getProps();
					
					if(iceflags!=0){
					Log::Info() << "Span flags: ";
					if(iceflags & plSpan::kLiteMaterial){ Log::Info() << "kLiteMaterial" << ", "; }
					if(iceflags & plSpan::kPropNoDraw){ Log::Info() << "kPropNoDraw" << ", "; }
					if(iceflags & plSpan::kPropNoShadowCast){ Log::Info() << "kPropNoShadowCast" << ", "; }
					if(iceflags & plSpan::kPropFacesSortable){ Log::Info() << "kPropFacesSortable" << ", "; }
					if(iceflags & plSpan::kPropVolatile){ Log::Info() << "kPropVolatile" << ", "; }
					if(iceflags & plSpan::kWaterHeight){ Log::Info() << "kWaterHeight" << ", "; }
					if(iceflags & plSpan::kPropRunTimeLight){ Log::Info() << "kPropRunTimeLight" << ", "; }
					if(iceflags & plSpan::kPropReverseSort){ Log::Info() << "kPropReverseSort" << ", "; }
					if(iceflags & plSpan::kPropHasPermaLights){ Log::Info() << "kPropHasPermaLights" << ", "; }
					if(iceflags & plSpan::kPropHasPermaProjs){ Log::Info() << "kPropHasPermaProjs" << ", "; }
					if(iceflags & plSpan::kLiteVtxPreshaded){ Log::Info() << "kLiteVtxPreshaded" << ", "; }
					if(iceflags & plSpan::kLiteVtxNonPreshaded){ Log::Info() << "kLiteVtxNonPreshaded" << ", "; }
					if(iceflags & plSpan::kLiteProjection){ Log::Info() << "kLiteProjection" << ", "; }
					if(iceflags & plSpan::kLiteShadowErase){ Log::Info() << "kLiteShadowErase" << ", "; }
					if(iceflags & plSpan::kLiteShadow){ Log::Info() << "kLiteShadow" << ", "; }
					if(iceflags & plSpan::kPropMatHasSpecular){ Log::Info() << "kPropMatHasSpecular" << ", "; }
					if(iceflags & plSpan::kPropProjAsVtx){ Log::Info() << "kPropProjAsVtx" << ", "; }
					if(iceflags & plSpan::kPropSkipProjection){ Log::Info() << "kPropSkipProjection" << ", "; }
					if(iceflags & plSpan::kPropNoShadow){ Log::Info() << "kPropNoShadow" << ", "; }
					if(iceflags & plSpan::kPropForceShadow){ Log::Info() << "kPropForceShadow" << ", "; }
					if(iceflags & plSpan::kPropDisableNormal){ Log::Info() << "kPropDisableNormal" << ", "; }
					if(iceflags & plSpan::kPropCharacter){ Log::Info() << "kPropCharacter" << ", "; }
					if(iceflags & plSpan::kPartialSort){ Log::Info() << "kPartialSort" << ", "; }
					if(iceflags & plSpan::kVisLOS){ Log::Info() << "kVisLOS" << ", "; }
					Log::Info() << std::endl;
					}
					const unsigned int shadingMode = iceflags & kLiteMask;
					
					if (matKey.Exists()) {
					
						hsGMaterial* mat = hsGMaterial::Convert(matKey->getObj(), false);
						if (mat != NULL && mat->getLayers().size() > 0) {
							_maxLayer = std::max(_maxLayer, mat->getLayers().size()-1);
							Log::Info() << "Material " << mat->getKey()->getName() << std::endl;
							const unsigned int cflags = mat->getCompFlags();
							if(cflags!=0){
							Log::Info() << "\tCompflags: ";
							if(cflags & hsGMaterial::kCompShaded){ Log::Info() << "kCompShaded" << ", ";}
							if(cflags & hsGMaterial::kCompEnvironMap){ Log::Info() << "kCompEnvironMap" << ", ";}
							if(cflags & hsGMaterial::kCompProjectOnto){ Log::Info() << "kCompProjectOnto" << ", ";}
							if(cflags & hsGMaterial::kCompSoftShadow){ Log::Info() << "kCompSoftShadow" << ", ";}
							if(cflags & hsGMaterial::kCompSpecular){ Log::Info() << "kCompSpecular" << ", ";}
							if(cflags & hsGMaterial::kCompTwoSided){ Log::Info() << "kCompTwoSided" << ", ";}
							if(cflags & hsGMaterial::kCompDrawAsSplats){ Log::Info() << "kCompDrawAsSplats" << ", ";}
							if(cflags & hsGMaterial::kCompAdjusted){ Log::Info() << "kCompAdjusted" << ", ";}
							if(cflags & hsGMaterial::kCompNoSoftShadow){ Log::Info() << "kCompNoSoftShadow" << ", ";}
							if(cflags & hsGMaterial::kCompDynamic){ Log::Info() << "kCompDynamic" << ", ";}
							if(cflags & hsGMaterial::kCompDecal){ Log::Info() << "kCompDecal" << ", ";}
							if(cflags & hsGMaterial::kCompIsEmissive){ Log::Info() << "kCompIsEmissive" << ", ";}
							if(cflags & hsGMaterial::kCompIsLightMapped){ Log::Info() << "kCompIsLightMapped" << ", ";}
							if(cflags & hsGMaterial::kCompNeedsBlendChannel){ Log::Info() << "kCompNeedsBlendChannel" << ", ";}
							Log::Info() << std::endl;
							}
							Log::Info() << "\tLayers: " <<  mat->getLayers().size() << std::endl;
							for(size_t lid = 0; lid < mat->getLayers().size(); ++lid){
								plLayerInterface* lay = plLayerInterface::Convert(mat->getLayers()[lid]->getObj(), false);
								if(lay->getTexture() != NULL){
									Log::Info() << "\t\tTexture: " << lay->getTexture()->getName() << ", using UV " << (lay->getUVWSrc() & plLayerInterface::kUVWIdxMask) << " and LOD bias " << lay->getLODBias() << (!lay->getTransform().IsIdentity() ? ", custom" : "") << "." << std::endl;
									
									
								}
								
								Log::Info() << "\t\tOpacity: " << lay->getOpacity() <<", ";
								Log::Info() << "Power: " << lay->getSpecularPower() <<", ";
								Log::Info() << std::endl;
								Log::Info() << "\t\tAmb: " << lay->getAmbient() <<", ";
								Log::Info() << "Pres: " << lay->getPreshade() <<", ";
								Log::Info() << "Spec: " << lay->getSpecular() <<", ";
								Log::Info() << "Run: " << lay->getRuntime() <<", ";
								Log::Info() << std::endl;
								
								const unsigned int fblend = lay->getState().fBlendFlags;
								const unsigned int fclamp = lay->getState().fClampFlags;
								const unsigned int fmisc = lay->getState().fMiscFlags;
								const unsigned int fshade = lay->getState().fShadeFlags;
								const unsigned int fz = lay->getState().fZFlags;
								
								if(fblend != 0){
								Log::Info() << "\t\tBlend: ";
								if(fblend & hsGMatState::kBlendTest){ Log::Info() << "kBlendTest" << ", "; }
								if(fblend & hsGMatState::kBlendAlpha){ Log::Info() << "kBlendAlpha" << ", "; }
								if(fblend & hsGMatState::kBlendMult){ Log::Info() << "kBlendMult" << ", "; }
								if(fblend & hsGMatState::kBlendAdd){ Log::Info() << "kBlendAdd" << ", "; }
								if(fblend & hsGMatState::kBlendAddColorTimesAlpha){ Log::Info() << "kBlendAddColorTimesAlpha" << ", "; }
								if(fblend & hsGMatState::kBlendAntiAlias){ Log::Info() << "kBlendAntiAlias" << ", "; }
								if(fblend & hsGMatState::kBlendDetail){ Log::Info() << "kBlendDetail" << ", "; }
								if(fblend & hsGMatState::kBlendNoColor){ Log::Info() << "kBlendNoColor" << ", "; }
								if(fblend & hsGMatState::kBlendMADD){ Log::Info() << "kBlendMADD" << ", "; }
								if(fblend & hsGMatState::kBlendDot3){ Log::Info() << "kBlendDot3" << ", "; }
								if(fblend & hsGMatState::kBlendAddSigned){ Log::Info() << "kBlendAddSigned" << ", "; }
								if(fblend & hsGMatState::kBlendAddSigned2X){ Log::Info() << "kBlendAddSigned2X" << ", "; }
								//if(fblend & kBlendMask){ Log::Info() << "kBlendMask" << ", "; }
								if(fblend & hsGMatState::kBlendInvertAlpha){ Log::Info() << "kBlendInvertAlpha" << ", "; }
								if(fblend & hsGMatState::kBlendInvertColor){ Log::Info() << "kBlendInvertColor" << ", "; }
								if(fblend & hsGMatState::kBlendAlphaMult){ Log::Info() << "kBlendAlphaMult" << ", "; }
								if(fblend & hsGMatState::kBlendAlphaAdd){ Log::Info() << "kBlendAlphaAdd" << ", "; }
								if(fblend & hsGMatState::kBlendNoVtxAlpha){ Log::Info() << "kBlendNoVtxAlpha" << ", "; }
								if(fblend & hsGMatState::kBlendNoTexColor){ Log::Info() << "kBlendNoTexColor" << ", "; }
								if(fblend & hsGMatState::kBlendNoTexAlpha){ Log::Info() << "kBlendNoTexAlpha" << ", "; }
								if(fblend & hsGMatState::kBlendInvertVtxAlpha){ Log::Info() << "kBlendInvertVtxAlpha" << ", "; }
								if(fblend & hsGMatState::kBlendAlphaAlways){ Log::Info() << "kBlendAlphaAlways" << ", "; }
								if(fblend & hsGMatState::kBlendInvertFinalColor){ Log::Info() << "kBlendInvertFinalColor" << ", "; }
								if(fblend & hsGMatState::kBlendInvertFinalAlpha){ Log::Info() << "kBlendInvertFinalAlpha" << ", "; }
								if(fblend & hsGMatState::kBlendEnvBumpNext){ Log::Info() << "kBlendEnvBumpNext" << ", "; }
								if(fblend & hsGMatState::kBlendSubtract){ Log::Info() << "kBlendSubtract" << ", "; }
								if(fblend & hsGMatState::kBlendRevSubtract){ Log::Info() << "kBlendRevSubtract" << ", "; }
								if(fblend & hsGMatState::kBlendAlphaTestHigh){ Log::Info() << "kBlendAlphaTestHigh" << ", "; }
								Log::Info() << std::endl;
								}
								if(fclamp!=0){
								Log::Info() << "\t\tClamp: ";
								if(fclamp == hsGMatState::kClampTextureU){ Log::Info() << "kClampTextureU" << ", "; }
								if(fclamp == hsGMatState::kClampTextureV){ Log::Info() << "kClampTextureV" << ", "; }
								if(fclamp == hsGMatState::kClampTexture){ Log::Info() << "kClampTexture" << ", "; }
								Log::Info() << std::endl;
								}
								
								if(fmisc!=0){
								Log::Info() << "\t\tMisc: ";
								if(fmisc & hsGMatState::kMiscWireFrame){ Log::Info() << "kMiscWireFrame" << ", "; }
								if(fmisc & hsGMatState::kMiscDrawMeshOutlines){ Log::Info() << "kMiscDrawMeshOutlines" << ", "; }
								if(fmisc & hsGMatState::kMiscTwoSided){ Log::Info() << "kMiscTwoSided" << ", "; }
								if(fmisc & hsGMatState::kMiscDrawAsSplats){ Log::Info() << "kMiscDrawAsSplats" << ", "; }
								if(fmisc & hsGMatState::kMiscAdjustPlane){ Log::Info() << "kMiscAdjustPlane" << ", "; }
								if(fmisc & hsGMatState::kMiscAdjustCylinder){ Log::Info() << "kMiscAdjustCylinder" << ", "; }
								if(fmisc & hsGMatState::kMiscAdjustSphere){ Log::Info() << "kMiscAdjustSphere" << ", "; }
								//if(fmisc & kMiscAdjust){ Log::Info() << "kMiscAdjust" << ", "; }
								if(fmisc & hsGMatState::kMiscTroubledLoner){ Log::Info() << "kMiscTroubledLoner" << ", "; }
								if(fmisc & hsGMatState::kMiscBindSkip){ Log::Info() << "kMiscBindSkip" << ", "; }
								if(fmisc & hsGMatState::kMiscBindMask){ Log::Info() << "kMiscBindMask" << ", "; }
								if(fmisc & hsGMatState::kMiscBindNext){ Log::Info() << "kMiscBindNext" << ", "; }
								if(fmisc & hsGMatState::kMiscLightMap){ Log::Info() << "kMiscLightMap" << ", "; }
								if(fmisc & hsGMatState::kMiscUseReflectionXform){ Log::Info() << "kMiscUseReflectionXform" << ", "; }
								if(fmisc & hsGMatState::kMiscPerspProjection){ Log::Info() << "kMiscPerspProjection" << ", "; }
								if(fmisc & hsGMatState::kMiscOrthoProjection){ Log::Info() << "kMiscOrthoProjection" << ", "; }
								//if(fmisc & kMiscProjection){ Log::Info() << "kMiscProjection" << ", "; }
								if(fmisc & hsGMatState::kMiscRestartPassHere){ Log::Info() << "kMiscRestartPassHere" << ", "; }
								if(fmisc & hsGMatState::kMiscBumpLayer){ Log::Info() << "kMiscBumpLayer" << ", "; }
								if(fmisc & hsGMatState::kMiscBumpDu){ Log::Info() << "kMiscBumpDu" << ", "; }
								if(fmisc & hsGMatState::kMiscBumpDv){ Log::Info() << "kMiscBumpDv" << ", "; }
								if(fmisc & hsGMatState::kMiscBumpDw){ Log::Info() << "kMiscBumpDw" << ", "; }
								//if(fmisc & kMiscBumpChans){ Log::Info() << "kMiscBumpChans" << ", "; }
								if(fmisc & hsGMatState::kMiscNoShadowAlpha){ Log::Info() << "kMiscNoShadowAlpha" << ", "; }
								if(fmisc & hsGMatState::kMiscUseRefractionXform){ Log::Info() << "kMiscUseRefractionXform" << ", "; }
								if(fmisc & hsGMatState::kMiscCam2Screen){ Log::Info() << "kMiscCam2Screen" << ", "; }
								//if(fmisc & hsGMatState::kAllMiscFlags){ Log::Info() << "kAllMiscFlags" << ", "; }
								Log::Info() << std::endl;
								}
								
								if(fshade!=0){
								Log::Info() << "\t\tShade: ";
								if(fshade & hsGMatState::kShadeSoftShadow){ Log::Info() << "kShadeSoftShadow" << ", "; }
								if(fshade & hsGMatState::kShadeNoProjectors){ Log::Info() << "kShadeNoProjectors" << ", "; }
								if(fshade & hsGMatState::kShadeEnvironMap){ Log::Info() << "kShadeEnvironMap" << ", "; }
								if(fshade & hsGMatState::kShadeVertexShade){ Log::Info() << "kShadeVertexShade" << ", "; }
								if(fshade & hsGMatState::kShadeNoShade){ Log::Info() << "kShadeNoShade" << ", "; }
								if(fshade & hsGMatState::kShadeBlack){ Log::Info() << "kShadeBlack" << ", "; }
								if(fshade & hsGMatState::kShadeSpecular){ Log::Info() << "kShadeSpecular" << ", "; }
								if(fshade & hsGMatState::kShadeNoFog){ Log::Info() << "kShadeNoFog" << ", "; }
								if(fshade & hsGMatState::kShadeWhite){ Log::Info() << "kShadeWhite" << ", "; }
								if(fshade & hsGMatState::kShadeSpecularAlpha){ Log::Info() << "kShadeSpecularAlpha" << ", "; }
								if(fshade & hsGMatState::kShadeSpecularColor){ Log::Info() << "kShadeSpecularColor" << ", "; }
								if(fshade & hsGMatState::kShadeSpecularHighlight){ Log::Info() << "kShadeSpecularHighlight" << ", "; }
								if(fshade & hsGMatState::kShadeVertColShade){ Log::Info() << "kShadeVertColShade" << ", "; }
								if(fshade & hsGMatState::kShadeInherit){ Log::Info() << "kShadeInherit" << ", "; }
								if(fshade & hsGMatState::kShadeIgnoreVtxIllum){ Log::Info() << "kShadeIgnoreVtxIllum" << ", "; }
								if(fshade & hsGMatState::kShadeEmissive){ Log::Info() << "kShadeEmissive" << ", "; }
								if(fshade & hsGMatState::kShadeReallyNoFog){ Log::Info() << "kShadeReallyNoFog" << ", "; }
								Log::Info() << std::endl;
								}
								if(fz != 0){
								Log::Info() << "\t\tDepth: ";
								if(fz & hsGMatState::kZIncLayer){ Log::Info() << "kZIncLayer" << ", "; }
								if(fz & hsGMatState::kZClearZ){ Log::Info() << "kZClearZ" << ", "; }
								if(fz & hsGMatState::kZNoZRead){ Log::Info() << "kZNoZRead" << ", "; }
								if(fz & hsGMatState::kZNoZWrite){ Log::Info() << "kZNoZWrite" << ", "; }
								//if(fz & kZMask){ Log::Info() << "kZMask" << ", "; }
								if(fz & hsGMatState::kZLODBias){ Log::Info() << "kZLODBias" << ", "; }
								Log::Info() << std::endl;
								}
								if(lay->getVertexShader().Exists() && lay->getPixelShader().Exists()){
									Log::Info() <<"\t\tShaders: " << lay->getVertexShader()->getName() << ", " << lay->getPixelShader()->getName() << std::endl;
								}
								if(lay->getUnderLay().Exists()){
									Log::Info() <<"\t\tUnderlay: " << lay->getUnderLay()->getName() << std::endl;
								}
								
							}
							/*
							LayerInterface* lay = plLayerInterface::Convert(mat->getLayers()[0]->getObj(), false);
							if(lay->getTexture() != NULL){
								textureName = lay->getTexture()->getName().to_std_string();
							}
							while (lay != NULL && lay->getUnderLay().Exists()){
								lay = plLayerInterface::Convert(lay->getUnderLay()->getObj(), false);
								if(lay->getTexture() != NULL){
									textureName = lay->getTexture()->getName().to_std_string();
								}
							}
							*/
							//const unsigned int uvwSrc = plLayerInterface::kUVWIdxMask & lay->getUVWSrc();
							//const hsMatrix44 uvwXform = lay->getTransform();
							
						
							Log::Info() << std::endl;
						}
					}
					
					
					
					const MeshInfos mesh = Resources::manager().registerMesh(fileName, meshIndices, meshPositions, meshNormals, meshColors, meshUVs);
					_objects.back()->addSubObject(mesh, hsGMaterial::Convert(matKey->getObj(), false), shadingMode);
				}
			}
			Log::Info() << std::endl;
		}
		
	}

	const auto textureKeys = rm.getKeys(ploc, pdUnifiedTypeMap::ClassIndex("plMipmap")); // TODO: also support envmap
	Log::Info() << textureKeys.size() << " textures." << std::endl;

	for(const auto & texture : textureKeys){
		// For each texture, send it to the gpu and keep a reference and the name.
		plMipmap* tex = plMipmap::Convert(texture->getObj());
		
		const std::string textureName = texture->getName().to_std_string() ;
		
		Resources::manager().registerTexture(textureName, tex);
		_textures.push_back(textureName);
	}
	
	const auto envmapKeys = rm.getKeys(ploc, pdUnifiedTypeMap::ClassIndex("plCubicEnvironmap"));
	Log::Info() << envmapKeys.size() << " cubemaps." << std::endl;
	
	for(const auto & envmap : envmapKeys){
		plCubicEnvironmap* env = plCubicEnvironmap::Convert(envmap->getObj());
		
		const std::string envmapName = envmap->getName().to_std_string() ;
		Resources::manager().registerCubemap(envmapName, env);
		_textures.push_back(envmapName);
	}
	
}

const glm::vec3 Age::getDefaultLinkingPoint(){
	if(_linkingPoints.count("LinkInPointDefault")>0){
		return _linkingPoints["LinkInPointDefault"];
	}
	if(_linkingPoints.empty()){
		return glm::vec3(0.0f,5.0f,0.0f);
	}
	return _linkingPoints[_linkingNamesCache.front()];
}


