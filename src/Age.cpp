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
#include <fstream>

#define M_PI_2 1.57079632679489661923

unsigned int  kLiteMask = (plSpan::kLiteMaterial | plSpan::kLiteVtxPreshaded | plSpan::kLiteVtxNonPreshaded | plSpan::kLiteProjection | plSpan::kLiteShadowErase | plSpan::kLiteShadow);

unsigned char reverse(unsigned char b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}



Age::Age(){
	_name = "None";
}

Age::Age(const std::string & path){
	
	const PlasmaVer plasmaVersion = PlasmaVer::pvMoul;
	_rm.reset(new plResManager());
	
	plAgeInfo* age = _rm->ReadAge(path, true);
	
	_name = age->getAgeName().to_std_string();
	
	Log::Info() << "Age " << _name << ": ";
	
	const size_t pageCount = age->getNumPages();
	Log::Info() << pageCount << " pages, " << std::flush;
	for(int i = 0 ; i < pageCount; ++i){
		const plLocation ploc = age->getPageLoc(i, plasmaVersion);
		loadMeshes(*_rm, ploc);
	}
	
	const size_t commmonCount = age->getNumCommonPages(plasmaVersion);
	Log::Info() << commmonCount << " common pages, " << std::flush;
	for(int i = 0 ; i < commmonCount; ++i){
		const plLocation ploc = age->getCommonPageLoc(i, plasmaVersion);
		loadMeshes(*_rm, ploc);
	}
	
	checkGLError();
	Log::Info() << _objects.size() << " objects."  << std::endl;
}

Age::~Age(){
	for(const auto & obj : _objects){
		obj->clean();
	}
	// Should clean the Age and everything.
}



void Age::loadMeshes(plResManager & rm, const plLocation& ploc){
	plSceneNode* scene = rm.getSceneNode(ploc);
	
	if(scene){
		Log::Mute();
		
		for(const auto & objKey : scene->getSceneObjects()){
			plSceneObject* obj = plSceneObject::Convert(rm.getObject(objKey));
			
			// Find the linking points.
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
			
			// Detect billboards.
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
			
			// Extract mesh transformation.
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
			
			Log::Info() << objKey->getName() << std::endl;
			
			_objects.emplace_back(new Object(type, Resources::manager().getProgram("object_basic"), model, objKey->getName().to_std_string()));
			
			// Extract subobjects, they are batched.
			for (size_t i = 0; i < draw->getNumDrawables(); ++i) {
				if (draw->getDrawableKey(i) == -1){
					continue;
				}
			
				// A span group multiple objects data/assets.
				plDrawableSpans* span = plDrawableSpans::Convert(draw->getDrawable(i)->getObj());
				plDISpanIndex di = span->getDIIndex(draw->getDrawableKey(i));
				
				// Ignore matrix-only span element.
				if ((di.fFlags & plDISpanIndex::kMatrixOnly) != 0){
					continue;
				}
				
				// Fog color.
				Log::Unmute();
				if(span->getSpan(0)->getFogEnvironment().Exists()){
					plFogEnvironment* fog = plFogEnvironment::Convert(span->getSpan(0)->getFogEnvironment()->getObj());
					Log::Info() << fog->getColor() << std::endl;
				}
				Log::Mute();
				
				// Each of these will be a subobject.
				for(size_t id = 0; id < di.fIndices.size(); ++id){
					const std::string fileName = scene->getKey()->getName().to_std_string() + "_" + obj->getKey()->getName().to_std_string() + "_" + std::to_string(i) + "_" + std::to_string(id) ;

					// Get the mesh internal representation.
					plIcicle* ice = span->getIcicle(di.fIndices[id]);
					const hsMatrix44 transfoMatrix = (bakePosition ? ice->getLocalToWorld() : hsMatrix44::Identity());
					hsMatrix44 transfoMatrixNormal = transfoMatrix;
					transfoMatrixNormal.setTranslate(hsVector3(0.0f,0.0f,0.0f));
					
					// Extract geometry data.
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
					
					// Convert infos for each vertex.
					for (size_t j = 0; j < verts.size(); j++) {
						const hsVector3 pos = transfoMatrix.multPoint(verts[j].fPos);
						const auto fnorm = transfoMatrixNormal.multVector(verts[j].fNormal);
						meshPositions[j] = glm::vec3(pos.X, pos.Y, pos.Z);
						meshNormals[j] = glm::vec3(fnorm.X, fnorm.Y, fnorm.Z);
						const unsigned int color = verts[j].fColor;
						unsigned char red = char((color >> 16) & 0xFF);
						unsigned char gre = char((color >> 8) & 0xFF);
						unsigned char blu = char((color) & 0xFF);
						unsigned char alp = char((color >> 24) & 0xFF);
						meshColors[j] = glm::u8vec4(red, gre, blu, alp);
						
						for(size_t uvid = 0; uvid < uvCount; ++uvid){
							const hsVector3 uvCoords = verts[j].fUVWs[uvid];
							meshUVs[uvid][j] = glm::vec3(uvCoords.X, uvCoords.Y, uvCoords.Z);
						}
					}
					
					Log::Info() << std::endl;
					
					// Material information
					plKey matKey = span->getMaterials()[ice->getMaterialIdx()];
					std::string textureName = "";
					
					// Properties.
					const unsigned int iceflags = span->getProps();
					const unsigned int shadingMode = iceflags & kLiteMask;
					
					logSpanProps(iceflags);
					
					// Log material details.
					if (matKey.Exists()) {
						hsGMaterial* mat = hsGMaterial::Convert(matKey->getObj(), false);
						if (mat != NULL && mat->getLayers().size() > 0) {
							Log::Info() << "Material " << mat->getKey()->getName() << std::endl;
							const unsigned int cflags = mat->getCompFlags();
							logCompFlags(cflags);
							// Extract layer infos.
							Log::Info() << "\tLayers: " <<  mat->getLayers().size() << std::endl;
							for(size_t lid = 0; lid < mat->getLayers().size(); ++lid){
								plLayerInterface* lay = plLayerInterface::Convert(mat->getLayers()[lid]->getObj(), false);
								Log::Info() << logLayer(lay) << std::endl;
							}
							Log::Info() << std::endl;
						}
					}
					
					// Add subobject to current object.
					const MeshInfos mesh = Resources::manager().registerMesh(fileName, meshIndices, meshPositions, meshNormals, meshColors, meshUVs);
					auto * matObj = hsGMaterial::Convert(matKey->getObj(), false);
					if(matObj){
						_objects.back()->addSubObject(mesh, matObj, shadingMode);
					}
				}
			}
			Log::Info() << std::endl;
		}
		Log::Unmute();
	}
	
	Log::Mute();
	
	// Extract textures.
	const auto textureKeys = rm.getKeys(ploc, pdUnifiedTypeMap::ClassIndex("plMipmap"));
	Log::Info() << textureKeys.size() << " textures." << std::endl;

	for(const auto & texture : textureKeys){
		// For each texture, send it to the gpu and keep a reference and the name.
		
		plMipmap* tex = plMipmap::Convert(texture->getObj());
		
		const std::string textureName = texture->getName().to_std_string() ;
		Log::Info() << textureName << std::endl;
		Resources::manager().registerTexture(textureName, tex);
		_textures.push_back(textureName);
	}
	// Extract cubemaps.
	const auto envmapKeys = rm.getKeys(ploc, pdUnifiedTypeMap::ClassIndex("plCubicEnvironmap"));
	Log::Info() << envmapKeys.size() << " cubemaps." << std::endl;
	
	for(const auto & envmap : envmapKeys){
		// For each texture, send it to the gpu and keep a reference and the name.
		plCubicEnvironmap* env = plCubicEnvironmap::Convert(envmap->getObj());
		
		const std::string envmapName = envmap->getName().to_std_string() ;
		Resources::manager().registerCubemap(envmapName, env);
		_textures.push_back(envmapName);
	}
	
	Log::Unmute();
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


