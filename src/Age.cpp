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
#include <Stream/plEncryptedStream.h>
#include <Debug/plDebug.h>
#include <Util/plPNG.h>
#include <Util/plJPEG.h>
#include <string_theory/stdio>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <fstream>
/*
static void pl_png_write(png_structp png, png_bytep data, png_size_t size) {
	hsStream* S = reinterpret_cast<hsStream*>(png_get_io_ptr(png));
	S->write(size, reinterpret_cast<const uint8_t*>(data));
}

void SavePNG(hsStream* S, const void* buf, size_t size,
						uint32_t width, uint32_t height, int pixelSize) {
	png_structp fPngWriter;
	png_infop   fPngInfo;
	
	
	
	fPngWriter = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!fPngWriter)
		throw hsPNGException(__FILE__, __LINE__, "Error initializing PNG writer");
	
	fPngInfo = png_create_info_struct(fPngWriter);
	if (!fPngInfo) {
		png_destroy_write_struct(&fPngWriter, NULL);
		fPngWriter = NULL;
		throw hsPNGException(__FILE__, __LINE__, "Error initializing PNG info structure");
	}
	png_set_write_fn(fPngWriter, (png_voidp)S, &pl_png_write, NULL);
	png_set_IHDR(fPngWriter, fPngInfo, width, height, 8,
				 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	// Plasma uses BGR for DirectX (TODO: Does HSPlasma need this?)
	//png_set_bgr(pi.fPngWriter);
	
	png_write_info(fPngWriter, fPngInfo);
	
	png_bytep src = reinterpret_cast<png_bytep>(const_cast<void*>(buf));
	std::unique_ptr<png_bytep[]> rows(new png_bytep[height]);
	const unsigned int stride = width * pixelSize / 8;
	for (size_t i = 0; i < height; ++i) {
		rows[i] = src + (i * stride);
		if (rows[i] + stride > src + size)
			throw hsPNGException(__FILE__, __LINE__, "PNG input buffer is too small");
	}
	
	png_write_image(fPngWriter, rows.get());
	png_write_end(fPngWriter, fPngInfo);
	png_destroy_write_struct(&fPngWriter, &fPngInfo);
}

*/


Age::Age(const std::string & path){
	
	const PlasmaVer plasmaVersion = PlasmaVer::pvMoul;
	//rm.reset(new plResManager());
	plResManager _rm;
	
	plAgeInfo* age = _rm.ReadAge(path, true);
	
	_name = age->getAgeName().to_std_string();
	
	Log::Info() << "Age " << _name << ":" << std::endl;
	
	const size_t pageCount = age->getNumPages();
	Log::Info() << pageCount << " pages." << std::endl;
	for(int i = 0 ; i < pageCount; ++i){
		Log::Info() << "Page: " << age->getPageFilename(i, plasmaVersion) << " ";
		const plLocation ploc = age->getPageLoc(i, plasmaVersion);
		loadMeshes(_rm, ploc);
		Log::Info() << std::endl;
	}
	Log::Info() << std::endl;
	
	const size_t commmonCount = age->getNumCommonPages(plasmaVersion);
	Log::Info() << commmonCount << " common pages." << std::endl;
	for(int i = 0 ; i < commmonCount; ++i){
		Log::Info() << "Page: " << age->getCommonPageFilename(i, plasmaVersion) << " ";
		const plLocation ploc = age->getCommonPageLoc(i, plasmaVersion);
		//loadMeshes(*_rm, ploc);
		Log::Info() << std::endl;
	}
	Log::Info() << std::endl;
	
	checkGLError();
	Log::Info() << "Objects: " << _objects.size() << std::endl;
	_rm.DelAge(age->getAgeName());
}

Age::~Age(){
	//_rm->
	for(const auto & obj : _objects){
		obj.clean();
	}
	//_rm.reset();
}

void Age::loadMeshes(plResManager & rm, const plLocation& ploc){
	plSceneNode* scene = rm.getSceneNode(ploc);
	
	if(scene){
		
		//Log::Info() << scene->getKey()->getName() << " (" << scene->getSceneObjects().size() << ")" << std::endl;
		
		for(const auto & objKey : scene->getSceneObjects()){
			// For each object, store its subgeometries (icicle)
			// and the used materials, referencing textures.
			
			plSceneObject* obj = plSceneObject::Convert(rm.getObject(objKey));
			
			//Log::Info() << objKey->getName() << std::endl;
			if(objKey->getName().substr(0, 11) == "LinkInPoint" && obj->getCoordInterface().Exists()){
				Log::Info() << objKey->getName() << std::endl;
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
			
			
			
			
		
			
			bool isBillboard = false;
			for(const auto & modKey : obj->getModifiers()){
				if(modKey->getType() == kViewFaceModifier){
					Log::Info() << "BILLBOARD" << std::endl;
					plViewFaceModifier* modif = plViewFaceModifier::Convert(modKey->getObj());
					Log::Info() << 	modif->getLocalToParent() << std::endl;
					isBillboard = true;
				}
			}
			
			
			glm::mat4 model = glm::mat4(1.0f);
			
			bool bakePosition = !obj->getCoordInterface().Exists();
			if(!bakePosition ){
				
				hsMatrix44 localToWorld = hsMatrix44::Identity();
				plCoordinateInterface* coord = plCoordinateInterface::Convert(obj->getCoordInterface()->getObj());
				localToWorld = coord->getLocalToWorld();
				if(isBillboard){Log::Info() << 	localToWorld << std::endl;}
				model[0][0] = localToWorld(0,0); model[0][1] = localToWorld(1,0); model[0][2] = localToWorld(2,0); model[0][3] = localToWorld(3,0);
				model[1][0] = localToWorld(0,1); model[1][1] = localToWorld(1,1); model[1][2] = localToWorld(2,1); model[1][3] = localToWorld(3,1);
				model[2][0] = localToWorld(0,2); model[2][1] = localToWorld(1,2); model[2][2] = localToWorld(2,2); model[2][3] = localToWorld(3,2);
				model[3][0] = localToWorld(0,3); model[3][1] = localToWorld(1,3); model[3][2] = localToWorld(2,3); model[3][3] = localToWorld(3,3);
			}
			model = glm::rotate(glm::mat4(1.0f), -float(M_PI_2), glm::vec3(1.0f,0.0f,0.0f)) * model;
			
			_objects.emplace_back(Resources::manager().getProgram("object_basic"), model, isBillboard);
			
			
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
				//Log::Info() << "\tIn drawable span " << i << " (" << span->getKey()->getName() << "): " << di.fIndices.size() << " indices." << std::endl;
					
				for(size_t id = 0; id < di.fIndices.size(); ++id){
					const std::string fileName = scene->getKey()->getName().to_std_string() + "_" + obj->getKey()->getName().to_std_string() + "_" + std::to_string(i) + "_" + std::to_string(id) ;
//					std::ofstream outfile(outputPath + "/" + fileName + ".obj");
//					if(!outfile.is_open()){
//						std::cerr << "ISSUE" << std::endl;
//						continue;
//					}
					
					
					// Get the mesh internal representation.
					plIcicle* ice = span->getIcicle(di.fIndices[id]);
					
					
					//Log::Info() << "\t\t" << "Icicle " << id << " (" << di.fIndices[id] << ")";
					//Log::Info() << span->getSpan(di.fIndices[id])
					//Log::Info() << std::endl;
					const hsMatrix44 transfoMatrix = (bakePosition ? ice->getLocalToWorld() : hsMatrix44::Identity());
					
					//const glm::vec3 center(centerRaw.X, centerRaw.Z, -centerRaw.Y);
					//Log::Info() << center << std::endl;
					
					std::vector<plGBufferVertex> verts = span->getVerts(ice);
					std::vector<unsigned short> indices = span->getIndices(ice);
					//Log::Info() << "Num uvs: " << span->getBuffer(ice->getGroupIdx())->getNumUVs() << std::endl;
					
					std::vector<unsigned int> meshIndices(indices.size());
					std::vector<glm::vec3> meshPositions(verts.size());
					std::vector<glm::vec3> meshNormals(verts.size());
					std::vector<glm::vec2> meshUVs(verts.size());
					
					for (size_t j = 0; j < indices.size(); ++j) {
						meshIndices[j] = (unsigned int)(indices[j]);
					}
					
					for (size_t j = 0; j < verts.size(); j++) {
						const hsVector3 pos = transfoMatrix.multPoint(verts[j].fPos);// * 10.0f;
						const auto fnorm = verts[j].fNormal;
						meshPositions[j] = glm::vec3(pos.X, pos.Y, pos.Z);
						meshNormals[j] = glm::vec3(fnorm.X, fnorm.Y, fnorm.Z);
					}
					
					bool hasTexture = false;
					// Find proper uv transformation (we can translate/scale material layers)
					
					plKey matKey = span->getMaterials()[ice->getMaterialIdx()];
					if (matKey.Exists()) {
						hsGMaterial* mat = hsGMaterial::Convert(matKey->getObj(), false);
						if (mat == NULL || mat->getLayers().empty()) {
							continue;
						}
						
						//Log::Info() << "\t\t\tMaterial: " << mat->getKey()->getName();
						//Log::Info() << ", " << mat->getLayers().size() << " layers. ";
						//Log::Info() << "Compo: " << mat->getCompFlags() << ", piggyback: " << mat->getPiggyBacks().size() << std::endl;
						
						for(size_t lid = 0; lid < mat->getLayers().size(); ++lid){
							plLayerInterface* lay = plLayerInterface::Convert(mat->getLayers()[lid]->getObj(), false);
							//Log::Info() << "\t\t\t\t" << lay->getKey()->getName();
							//Log::Info() << ", flags: " << lay->getState().fBlendFlags << " " << lay->getState().fClampFlags << " " <<  lay->getState().fShadeFlags << " " <<  lay->getState().fZFlags  << " " <<  lay->getState().fMiscFlags << ".";
							if(lay->getTexture()){
							//	Log::Info() << " Texture: " << lay->getTexture()->getName() << ", UV: " << lay->getUVWSrc();
								// Export uvs.
								const unsigned int uvwSrc = lay->getUVWSrc();
								const hsMatrix44 uvwXform = lay->getTransform();
								for (size_t j = 0; j < verts.size(); j++) {
									//hsVector3 txUvw = uvwXform.multPoint(verts[j].fUVWs[uvwSrc]);
									// z will probably always be 0
									// -1 for blender?
									//outfile << "vt " << txUvw.X << " " << -txUvw.Y << " " << txUvw.Z << std::endl;
								}
								hasTexture = true;
							}
							//Log::Info() << std::endl;
							
						}
						
						plLayerInterface* lay = plLayerInterface::Convert(mat->getLayers()[0]->getObj(), false);
						if(lay->getTexture()){
							// Export uvs.
							const unsigned int uvwSrc = plLayerInterface::kUVWIdxMask & lay->getUVWSrc();
							const hsMatrix44 uvwXform = lay->getTransform();
							for (size_t j = 0; j < verts.size(); j++) {
								hsVector3 txUvw = uvwXform.multPoint(verts[j].fUVWs[uvwSrc]);
								// z will probably always be 0
								// -1 for blender?
								meshUVs[j] = glm::vec2(txUvw.X, -txUvw.Y);
								//outfile << "vt " << txUvw.X << " " << -txUvw.Y << " " << txUvw.Z << std::endl;
							}
						}
						
							// find lowest underlay UV set.
							// TODO: support undlerlay?
							//
							//
							//
							//
							
							
					}
					
					const MeshInfos mesh = Resources::manager().registerMesh(fileName, meshIndices, meshPositions, meshNormals, meshUVs);
					_objects.back().addSubObject(mesh);
						//					if(hasTexture){
						//						for (size_t j = 0; j < verts.size(); j++) {
						//							hsVector3 txUvw = uvwXform.multPoint(verts[j].fUVWs[uvwSrc]);
						//							// z will probably always be 0
						//							// -1 for blender?
						//							outfile << "vt " << txUvw.X << " " << -txUvw.Y << " " << txUvw.Z << std::endl;
						//						}
						//					}
						
						
					
							//outfile << "f " << indices[j]+1 << "/" << (hasTexture ? std::to_string(indices[j]+1) : "") << "/" << indices[j]+1;
							//outfile <<  " " << indices[j+1]+1 << "/" << (hasTexture ? std::to_string(indices[j+1]+1) : "") << "/" << indices[j+1]+1;
							//outfile <<  " " << indices[j+2]+1 << "/" << (hasTexture ? std::to_string(indices[j+2]+1) : "") << "/" << indices[j+2]+1;
							//outfile << std::endl;
					
						//outfile.close();
						
						
				}
					
			}
		}
		
	}

	const auto textureKeys = rm.getKeys(ploc, pdUnifiedTypeMap::ClassIndex("plMipmap")); // also support envmap
	Log::Info() << textureKeys.size() << " textures." << std::endl;

	for(const auto & texture : textureKeys){
		// For each texture, send it to the gpu and keep a reference and the name.
		plMipmap* tex = plMipmap::Convert(texture->getObj());
		
		const std::string fileName = "texture__" + texture->getName().to_std_string() ;
		Log::Info() << fileName << std::endl;
		
		
		uint8_t *dest;
		size_t sizeComplete;
		if(tex->getCompressionType() != plBitmap::kDirectXCompression){
			//dest = (uint8_t*)(tex->getLevelData(0));
			sizeComplete = tex->getLevelSize(0);
			
		} else {
			size_t blocks = tex->GetUncompressedSize(0) / sizeof(size_t);
			if ((tex->GetUncompressedSize(0)) % sizeof(size_t) != 0)
				++blocks;
			sizeComplete = tex->GetUncompressedSize(0);
			//dest = reinterpret_cast<uint8_t*>(new size_t[blocks]);
			//DecompressImage(0, fImageData, fLevelData[0].fSize);
			//tex->DecompressImage(0, dest, tex->getLevelSize(0));
		}
		//hsFileStream outTex;
		//outTex.open(ST::string(outputPath + "/" + fileName + ".png"), FileMode::fmWrite);
		//SavePNG(&outTex, dest, sizeComplete, tex->getLevelWidth(0),  tex->getLevelHeight(0), tex->getBPP());
		//outTex.close();
		
		
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


