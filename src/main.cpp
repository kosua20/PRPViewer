#include <stdio.h>
#include <iostream>
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
#include <PRP/Surface/plLayer.h>
#include <Stream/plEncryptedStream.h>
#include <Debug/plDebug.h>
#include <string_theory/stdio>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <fstream>

std::ostream &operator<<(std::ostream& strm, const ST::string& ststr){
	strm << ststr.to_std_string();
	return strm;
}

const std::string outputPath = "../../../data/out/";

void logLocation(plResManager & rm, const plLocation & ploc){
	/*const auto types = rm.getTypes(ploc);
	
	std::cout << "(" << types.size() << ")" << std::endl;
	for(const auto & type : types){
		std::cout << "\t" << pdUnifiedTypeMap::ClassName(type);
		
		
		const auto keys = rm.getKeys(ploc, type);
		
		std::cout << " (" << keys.size() << ")" << std::endl;
		for(const auto & key : keys){
			std::cout << "\t\t" << key.toString() << std::endl;
		}
	}*/
//	const auto keys = rm.getKeys(ploc, pdUnifiedTypeMap::ClassIndex("plSceneNode"));
//
//	std::cout << " (" << keys.size() << ")" << std::endl;
//	for(const auto & key : keys){
//
//		// (plSceneNode*)rm.getObject(key);
//
//		//std::cout << "\t\t" << (scene->getSceneObjects()).toString() << std::endl;
//	}
	plSceneNode* scene = rm.getSceneNode(ploc);
	if(scene){
		std::cout << scene->getKey().toString() << " (" << scene->getSceneObjects().size() << ")" << std::endl;
		for(const auto & objKey : scene->getSceneObjects()){
			std::cout << objKey.toString() << std::endl;
			/* if (!obj->getDrawInterface().isLoaded()) {
			 plDebug::Warning("Cannot get draw interface for {}", obj->getKey()->getName());
			 return;
			 }
			 */
			
			
			
			plSceneObject* obj = plSceneObject::Convert(rm.getObject(objKey));
			if(!obj->getDrawInterface().Exists()){
				continue;
			}
			plDrawInterface* draw = plDrawInterface::Convert(obj->getDrawInterface()->getObj());
			plCoordinateInterface* coord = NULL;
			if (obj->getCoordInterface().Exists()){
				coord = plCoordinateInterface::Convert(obj->getCoordInterface()->getObj());
			}
			std::cout << "Element " << draw->getNumDrawables() << std::endl;
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
				std::cout << "Subelements " << di.fIndices.size() << std::endl;
				for(size_t id = 0; id < di.fIndices.size(); ++id){
					const std::string fileName = scene->getKey()->getName().to_std_string() + "__" + obj->getKey()->getName().to_std_string() + "__" + std::to_string(i) + "_" + std::to_string(id) ;
					std::ofstream outfile(outputPath + "/" + fileName + ".obj");
					if(!outfile.is_open()){
						std::cerr << "ISSUE" << std::endl;
						continue;
					}
					
					
					// Get the mesh internal representation.
					plIcicle* ice = (plIcicle*)span->getSpan(di.fIndices[id]);
					std::vector<plGBufferVertex> verts = span->getVerts(ice);
					std::vector<unsigned short> indices = span->getIndices(ice);
					
					for (size_t j = 0; j < verts.size(); j++) {
						hsVector3 pos;
						if (coord != NULL){
							pos = coord->getLocalToWorld().multPoint(verts[j].fPos);// * 10.0f;
						} else {
							pos = ice->getLocalToWorld().multPoint(verts[j].fPos);// * 10.0f;
						}
						outfile << "v " << pos.X << " " << pos.Z << " " << (-pos.Y) << std::endl;
						
						outfile << "vn " <<  verts[j].fNormal.X << " " <<  verts[j].fNormal.Z << " " << (- verts[j].fNormal.Y) << std::endl;
						
					}
					for (size_t j = 0; j < indices.size(); j += 3) {
						outfile << "f " << indices[j]+1 << "//" << indices[j]+1;
						outfile <<  " " << indices[j+1]+1 << "//" << indices[j+1]+1;
						outfile <<  " " << indices[j+2]+1 << "//" << indices[j+2]+1;
						outfile << std::endl;
					}
					outfile.close();
					// Find proper uv transformation (we can translate/scale material layers)
				//	unsigned int uvwSrc = 0;
				//	hsMatrix44 uvwXform;
//					plKey matKey = span->getMaterials()[ice->getMaterialIdx()];
//					if (matKey.Exists()) {
//						hsGMaterial* mat = hsGMaterial::Convert(matKey->getObj(), false);
//						if (mat != NULL && mat->getLayers().size() > 0) {
//							plLayerInterface* lay = plLayerInterface::Convert(mat->getLayers()[0]->getObj(), false);
//							// find lowest underlay ?
//							while (lay != NULL && lay->getUnderLay().Exists())
//								lay = plLayerInterface::Convert(lay->getUnderLay()->getObj(), false);
//							uvwSrc = lay->getUVWSrc();
//							uvwXform = lay->getTransform();
//						}
//					}
					
					// indices
//					if (span->getBuffer(ice->getGroupIdx())->getNumUVs() > uvwSrc) {
//						for (size_t j = 0; j < indices.size(); j += 3) {
//							/*
//							 v t n v t n v t n
//												   indices[j+0] + s_BaseIndex,
//												   indices[j+0] + s_TexBaseIndex,
//												   indices[j+0] + s_BaseIndex,
//												   indices[j+1] + s_BaseIndex,
//												   indices[j+1] + s_TexBaseIndex,
//												   indices[j+1] + s_BaseIndex,
//												   indices[j+2] + s_BaseIndex,
//												   indices[j+2] + s_TexBaseIndex,
//												   indices[j+2] + s_BaseIndex
//							 */
//						}
//						s_BaseIndex += ice->getVLength();
//						s_TexBaseIndex += ice->getVLength();
//					} else {
//						for (size_t j = 0; j < indices.size(); j += 3) {
//
//							//v n v n v n
//												 /*  indices[j+0] + s_BaseIndex,
//												   indices[j+0] + s_BaseIndex,
//												   indices[j+1] + s_BaseIndex,
//												   indices[j+1] + s_BaseIndex,
//												   indices[j+2] + s_BaseIndex,
//												   indices[j+2] + s_BaseIndex));*/
//						}
						//s_BaseIndex += ice->getVLength();
					//}
				}
				
			}
		}
	}
	const auto textureKeys = rm.getKeys(ploc, pdUnifiedTypeMap::ClassIndex("plMipmap")); // also support envmap
	std::cout << textureKeys.size() << " textures." << std::endl;
}


int main(int argc, char** argv){
	std::cout << "Shorah !" << std::endl;
	if(argc < 2){
		return 1;
	}
	
	const std::string path(argv[1]);
	const PlasmaVer plasmaVersion = PlasmaVer::pvMoul;
	
	plResManager rm;
	plAgeInfo* age;
	try {
		age = rm.ReadAge(path, true);
		
	} catch (hsException& e) {
		ST::printf(stderr, "{}:{}: {}\n", e.File(), e.Line(), e.what());
		return 1;
	} catch (std::exception& e) {
		ST::printf(stderr, "Prc Extract Exception: {}\n", e.what());
		return 1;
	} catch (...) {
		fputs("Undefined error!\n", stderr);
		return 1;
	}
	
	
	
	ST::string outDir = age->getAgeName();
	
	std::cout << outDir.to_std_string() << ":" << std::endl;
	
	const size_t pageCount = age->getNumPages();
	std::cout << pageCount << " pages." << std::endl;
	for(int i = 0 ; i < pageCount; ++i){
		std::cout << "\t" << age->getPageFilename(i, plasmaVersion) << std::endl;
	}
	
	const size_t commmonCount = age->getNumCommonPages(plasmaVersion);
	std::cout << commmonCount << " common pages." << std::endl;
	for(int i = 0 ; i < commmonCount; ++i){
		std::cout << "\t" << age->getCommonPageFilename(i, plasmaVersion) << std::endl;
	}
	std::cout << std::endl;
	
	for(int i = 0 ; i < age->getNumPages(); ++i){
		std::cout << "Page: " << age->getPageFilename(i, plasmaVersion) << " ";
		const plLocation ploc = age->getPageLoc(i, plasmaVersion);
		logLocation(rm, ploc);
	}
	
	for(int i = 0 ; i < age->getNumCommonPages(plasmaVersion); ++i){
		std::cout << "Page: " << age->getCommonPageFilename(i, plasmaVersion) << " ";
		const plLocation ploc = age->getCommonPageLoc(i, plasmaVersion);
		logLocation(rm, ploc);
	}
	
	
	
	
	
	
	
	return 0;
}
