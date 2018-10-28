#include "Object.hpp"
#include "helpers/Logger.hpp"
#include <stdio.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <PRP/Surface/hsGMaterial.h>
#include <PRP/Geometry/plSpan.h>
#include <PRP/Surface/plLayer.h>

Object::Object(const Type & type, std::shared_ptr<ProgramInfos> prog, const glm::mat4 &model, const std::string & name) {
	_program = prog;
	_type = type;
	_model = glm::mat4(model);
	_name = name;
	enabled = true;
	_transparent = false;
	_billboard = false;
	_centroid = glm::vec3(0.0f);
	
	if(_type == Billboard || _type == BillboardY){
		_billboard = true;
	}
	
	_probablySky = (name.find("sky") != std::string::npos || name.find("Sky") != std::string::npos);
}

Object::~Object() {}


void Object::addSubObject(const MeshInfos & infos, hsGMaterial * material, const unsigned int shadingMode){
	if(_subObjects.empty()){
		_localBounds = infos.bbox;
	} else {
		_localBounds += infos.bbox;
	}
	_localBounds.updateValues();
	_globalBounds = _localBounds.transform(_model);
	// That's not very accurate, but simpler.
	_centroid = (float(_subObjects.size()) * _centroid + infos.centroid) / (_subObjects.size()+1.0f);
	
	// Check if subobject is transparent.
	bool isAlphaBlend = false;
	if(!material->getLayers().empty()){
		const auto & layKey = material->getLayers()[0];
		// Obtain the layer to apply.
		const plLayerInterface * lay = plLayerInterface::Convert(layKey->getObj(), false);
		isAlphaBlend = lay->getState().fBlendFlags & hsGMatState::kBlendAlpha;
		_transparent = _transparent || isAlphaBlend;
		// Also check the underlay.
		const bool hasUnderlay = lay->getUnderLay().Exists();
		if(hasUnderlay){
			const plLayerInterface * lay1 = plLayerInterface::Convert(lay->getUnderLay()->getObj(), false);
			isAlphaBlend = lay1->getState().fBlendFlags & hsGMatState::kBlendAlpha;
			_transparent = _transparent || isAlphaBlend;
		}
	}
	auto newSubObject = std::make_shared<SubObject>(infos, material, shadingMode, isAlphaBlend);
	
	_subObjects.push_back(newSubObject);
	
	// Should we instead sort the transparent and non transparent subobjects too ?
	//if(isAlphaBlend || !_transparent){
	//	_subObjects.push_back(newSubObject);
	//} else {
	//		size_t soid = 0;
	//		while(soid < _subObjects.size() && !_subObjects[soid]->transparent){
	//			++soid;
	//		}
	//// Soid is the first transparent element, or the size.
	//_subObjects.insert(_subObjects.begin()+soid, newSubObject);
	//}
}


const bool Object::isVisible(const glm::vec3 & point, const glm::mat4 & viewproj) const {
	return _globalBounds.contains(point) || _globalBounds.intersectsFrustum(viewproj);
}

const bool Object::isVisible(const glm::vec3 & point, const glm::vec3 & dir) const {
	return _globalBounds.contains(point) || _globalBounds.isVisible(point, dir);
}

const bool Object::contains( const std::shared_ptr<Object> & other) const {
	return _globalBounds.contains(other->_globalBounds);
}

void Object::drawDebug(const glm::mat4& view, const glm::mat4& projection, const int subObjId) const {
	
	//Log::Info() << _bounds.getCenter() << std::endl;
	glm::mat4 MV = view * _model;
	if(_type == Billboard || _type == BillboardY){
		glm::mat4 viewCopy = glm::transpose(glm::mat4(glm::mat3(view)));
		if(_type == BillboardY){
			viewCopy[1][0] = 0.0;
			viewCopy[1][1] = 1.0;
			viewCopy[1][2] = 0.0;
		}
		const float scaleBoard = std::max(std::max(std::abs(_model[0][0]),std::abs(_model[1][1])), std::abs(_model[2][2]));
		MV = view * glm::translate(glm::mat4(1.0f), glm::vec3(_model[3][0],_model[3][1],_model[3][2])) * viewCopy  * glm::scale(glm::mat4(1.0f), glm::vec3(scaleBoard));
	}
	const glm::mat4 MVP = projection * MV;
	const auto debugProgram = Resources::manager().getProgram("wireframe");
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glUseProgram(debugProgram->id());
	
	glUniformMatrix4fv(debugProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
	glUniform3f(debugProgram->uniform("color"), 0.0f,0.0f,0.0f);
	
	glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );
	
	int sid = -1;
	for(const auto & subObject : _subObjects){
		++sid;
		if(subObjId > -1 && subObjId != sid){
			continue;
		}
		glBindVertexArray(subObject->mesh.vId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subObject->mesh.eId);
		glDrawElements(GL_TRIANGLES, subObject->mesh.count, GL_UNSIGNED_INT, (void*)0);
	}
	
	if(_type != Billboard && _type != BillboardY){
		const auto center = _globalBounds.center;
		const auto scale = _globalBounds.getScale()*0.5f;
		glm::mat4 iden = glm::scale(glm::translate(glm::mat4(1.0f), center), scale);
		iden = projection * view * iden;
		glUniformMatrix4fv(debugProgram->uniform("mvp"), 1, GL_FALSE, &iden[0][0]);
		glUniform3f(debugProgram->uniform("color"), 1.0f,0.0f,0.0f);
		const auto boxMesh = Resources::manager().getMesh("box");
		glBindVertexArray(boxMesh.vId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxMesh.eId);
		glDrawElements(GL_TRIANGLES, boxMesh.count, GL_UNSIGNED_INT, (void*)0);
		
	}
	glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );
	glBindVertexArray(0);
	glUseProgram(0);
	glEnable(GL_CULL_FACE);
}

void Object::draw(const glm::mat4& view, const glm::mat4& projection, const int subObjId, const int layerId) const {

	// Compute final view and model matrices.
	glm::mat4 MV = view * _model;
	if(_type == Billboard || _type == BillboardY){
		glm::mat4 viewCopy = glm::transpose(glm::mat4(glm::mat3(view)));
		if(_type == BillboardY){
			viewCopy[1][0] = 0.0;
			viewCopy[1][1] = 1.0;
			viewCopy[1][2] = 0.0;
		}
		const float scaleBoard = std::max(std::max(std::abs(_model[0][0]),std::abs(_model[1][1])), std::abs(_model[2][2]));
		MV = view * glm::translate(glm::mat4(1.0f), glm::vec3(_model[3][0],_model[3][1],_model[3][2])) * viewCopy  * glm::scale(glm::mat4(1.0f), glm::vec3(scaleBoard));
	}
	const glm::mat4 MVP = projection * MV;
	const glm::mat4 invV = glm::inverse(view);
	const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
	
	// Send common infos.
	glUseProgram(_program->id());
	glUniformMatrix4fv(_program->uniform("mv"), 1, GL_FALSE, &MV[0][0]);
	glUniformMatrix4fv(_program->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(_program->uniform("invV"), 1, GL_FALSE, &invV[0][0]);
	glUniformMatrix3fv(_program->uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
	
	int sid = -1;
	
	for(const auto & subObject : _subObjects){
		
		glPolygonOffset(0.0f, 0.0f);
		glDisable(GL_POLYGON_OFFSET_FILL);
		
		++sid;
		if(subObjId > -1 && sid != subObjId){
			continue;
		}
		// Render each layer, one after the other.
		if(!subObject->material){
			continue;
		}
	
		const auto * layObj = plLayerInterface::Convert(subObject->material->getLayers()[0]->getObj(), false);
		const bool hasTexture = layObj->getTexture().Exists();
		const bool hasUnderlay = layObj->getUnderLay().Exists();
		if(subObject->material->getLayers().size() == 1 && !hasTexture && !hasUnderlay){
			continue;
		}
		   
		// Transparent object: layer  has non unit opacity + blend.
		for(size_t tid = 0; tid < subObject->material->getLayers().size(); tid++){
			
			
			if(layerId > -1 && tid > layerId){
				continue;
			}
			const auto & layKey = subObject->material->getLayers()[tid];
			// Obtain the layer to aply.
			plLayerInterface * lay = plLayerInterface::Convert(layKey->getObj(), false);
			
			// If this layer is a bump layer, skip for now. TODO: add support for bump maps.
			if((lay->getState().fMiscFlags & (hsGMatState::kMiscBumpDu | hsGMatState::kMiscBumpDv | hsGMatState::kMiscBumpDw | hsGMatState::kMiscBumpLayer | hsGMatState::kMiscBumpChans)) != 0){
				continue;
			}
			// Fix for non texture null state layers.
			bool shouldStop = false;
			while(!lay->getTexture().Exists() && lay->getState().fMiscFlags == 0 && lay->getState().fBlendFlags == 0 && lay->getState().fZFlags == 0 && lay->getState().fShadeFlags == 0 && !shouldStop){
				
				if(lay->getUnderLay().Exists()){
					// So we have a texture, but no infos on how to render it, and then an underlay?
					// Smells like the vertex color hack.
					// Where the underlay is used as an alpha map.
					plLayerInterface * underlay = plLayerInterface::Convert(lay->getUnderLay()->getObj(), false);
					lay = underlay;
					// Just to be safe, let's replicate the same check here.
					if((lay->getState().fMiscFlags & (hsGMatState::kMiscBumpDu | hsGMatState::kMiscBumpDv | hsGMatState::kMiscBumpDw | hsGMatState::kMiscBumpLayer | hsGMatState::kMiscBumpChans)) != 0){
						shouldStop = true;
					}
				} else {
					shouldStop = true;
				}
				
			}
			if(shouldStop){
				continue;
			}
			
			// TODO: cache this at load time?
			if(tid < subObject->material->getLayers().size()-1){
				
				const bool restartBindNext = (lay->getState().fMiscFlags & hsGMatState::kMiscBindNext) ;
				//&& (lay->getState().fMiscFlags & hsGMatState::kMiscRestartPassHere)
				if(restartBindNext){
					plLayerInterface * layNext = plLayerInterface::Convert(subObject->material->getLayers()[tid+1]->getObj(), false);
					const bool nextIsAlphaBlend = (layNext->getState().fBlendFlags & hsGMatState::kBlendAlphaMult) && (layNext->getState().fBlendFlags & hsGMatState::kBlendNoTexColor);
					if(nextIsAlphaBlend){
						// Render both at the same time, using our special shader.
						// Skip next layer.
						const auto & program = Resources::manager().getProgram("object_special");
						glUseProgram(program->id());
						glUniformMatrix4fv(program->uniform("mv"), 1, GL_FALSE, &MV[0][0]);
						glUniformMatrix4fv(program->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
						glUniformMatrix4fv(program->uniform("invV"), 1, GL_FALSE, &invV[0][0]);
						glUniformMatrix3fv(program->uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
						renderLayerMult(subObject, lay, layNext, tid);
						++tid;
						continue;
					} else {
						//Log::Warning() << "Can this case arise?" << std::endl;
					}
				}
			}
			
			renderLayer(subObject, lay, tid);
			
		}
	}
	
	glUseProgram(0);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glPolygonOffset(0.0f, 0.0f);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_CULL_FACE);
	checkGLError();
	
}

void Object::resetState() const {
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	
	if(_type == Billboard || _type == BillboardY){
		glDisable(GL_CULL_FACE);
	}
}

void Object::depthState(plLayerInterface* lay, const bool forceDecal, const int tid) const {
	const unsigned int zflag = lay->getState().fZFlags;
	if((zflag & hsGMatState::kZNoZWrite) || forceDecal){
		glDepthMask(GL_FALSE);
	}
	if(zflag & hsGMatState::kZNoZRead){
		glDepthFunc(GL_ALWAYS);
	}
	if(zflag & hsGMatState::kZClearZ){
		glDepthFunc(GL_ALWAYS);
	}
	if((zflag & hsGMatState::kZIncLayer) || forceDecal){
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-10.0f,-(tid+1)*10.0f);
	}
	if(lay->getState().fMiscFlags & hsGMatState::kMiscTwoSided){
		glDisable(GL_CULL_FACE);
	}
	checkGLError();
}

void Object::shadeState(const std::shared_ptr<ProgramInfos> & program, plLayerInterface*lay, const unsigned int mode) const {
	const unsigned int fshade = lay->getState().fShadeFlags;
	
	switch(mode){
		case plSpan::kLiteMaterial:
		{
			// Ambient.
			if(fshade & hsGMatState::kShadeWhite){
				glUniform4f(program->uniform("globalAmbient"), 1.0f, 1.0f, 1.0f, 1.0f);
				glUniform4f(program->uniform("ambient"), 1.0f, 1.0f, 1.0f, 1.0f);
			} else {
				const hsColorRGBA amb = lay->getPreshade();
				glUniform4f(program->uniform("globalAmbient"), amb.r, amb.g, amb.b, 1.0f);
				glUniform4f(program->uniform("ambient"), amb.r, amb.g, amb.b, 1.0f);
			}
			const hsColorRGBA dif = lay->getRuntime();
			const hsColorRGBA emi = lay->getAmbient();
			glUniform4f(program->uniform("diffuse"), dif.r, dif.g, dif.b, lay->getOpacity());
			glUniform4f(program->uniform("emissive"), emi.r, emi.g, emi.b, 1.0f);
			
			// Specular.
			if (fshade & hsGMatState::kShadeSpecular) {
				const hsColorRGBA spec = lay->getSpecular();
				glUniform4f(program->uniform("specular"), spec.r, spec.g, spec.b, 1.0f);
			} else {
				glUniform4f(program->uniform("specular"), 0.0f, 0.0f, 0.0f, 0.0f);
			}
			
			glUniform1f(program->uniform("diffuseSrc"), 1.0f);
			glUniform1f(program->uniform("specularSrc"), 1.0f);
			glUniform1f(program->uniform("emissiveSrc"), 1.0f);
			
			if (fshade & hsGMatState::kShadeNoShade) {
				glUniform1f(program->uniform("ambientSrc"), 1.0f);
			} else {
				glUniform1f(program->uniform("ambientSrc"), 0.0f);
			}
			break;
		}
		case plSpan::kLiteVtxNonPreshaded:
		{
			const hsColorRGBA amb = lay->getPreshade();
			const hsColorRGBA emi = lay->getAmbient();
			
			glUniform4f(program->uniform("globalAmbient"), amb.r, amb.g, amb.b, amb.a);
			glUniform4f(program->uniform("ambient"), 0.0f, 0.0f, 0.0f, 0.0f);
			glUniform4f(program->uniform("diffuse"), 0.0f, 0.0f, 0.0f, 0.0f);
			glUniform4f(program->uniform("emissive"), emi.r, emi.g, emi.b, 1.0f);
			
			if (fshade & hsGMatState::kShadeSpecular) {
				const hsColorRGBA spec = lay->getSpecular();
				glUniform4f(program->uniform("specular"), spec.r, spec.g, spec.b, 1.0f);
			} else {
				glUniform4f(program->uniform("specular"), 0.0f, 0.0f, 0.0f, 0.0f);
			}
			
			glUniform1f(program->uniform("diffuseSrc"), 0.0f);
			glUniform1f(program->uniform("ambientSrc"), 0.0f);
			glUniform1f(program->uniform("specularSrc"), 1.0f);
			glUniform1f(program->uniform("emissiveSrc"), 1.0f);
			break;
		}
		case plSpan::kLiteVtxPreshaded:
		{
			glUniform4f(program->uniform("globalAmbient"), 0.0f, 0.0f, 0.0f, 0.0f);
			glUniform4f(program->uniform("ambient"), 0.0f, 0.0f, 0.0f, 0.0f);
			glUniform4f(program->uniform("diffuse"), 0.0f, 0.0f, 0.0f, 0.0f);
			glUniform4f(program->uniform("emissive"), 0.0f, 0.0f, 0.0f, 0.0f);
			glUniform4f(program->uniform("specular"), 0.0f, 0.0f, 0.0f, 0.0f);
			glUniform1f(program->uniform("diffuseSrc"), 0.0f);
			glUniform1f(program->uniform("ambientSrc"), 1.0f);
			glUniform1f(program->uniform("specularSrc"), 1.0f);
			glUniform1f(program->uniform("emissiveSrc"), (fshade & hsGMatState::kShadeEmissive) ? 0.0f : 1.0f);
			break;
		}
		default:
			break;
	}
	checkGLError();
}

void Object::blendState(const std::shared_ptr<ProgramInfos> & program, plLayerInterface * lay) const {
	glEnable(GL_BLEND);
	const unsigned int bflags = lay->getState().fBlendFlags;
	glUniform1i(program->uniform("invertVertexAlpha"), bflags & hsGMatState::kBlendInvertVtxAlpha ? 1 : 0);
	
	glUniform1i(program->uniform("blendInvertColor"), bflags & hsGMatState::kBlendInvertColor ? 1 : 0);
	glUniform1i(program->uniform("blendInvertAlpha"), bflags & hsGMatState::kBlendInvertAlpha ? 1 : 0);
	
	glUniform1i(program->uniform("blendNoTexColor"), bflags & hsGMatState::kBlendNoTexColor ? 1 : 0);
	glUniform1i(program->uniform("blendNoVtxAlpha"), bflags & hsGMatState::kBlendNoVtxAlpha ? 1 : 0);
	glUniform1i(program->uniform("blendNoTexAlpha"), bflags & hsGMatState::kBlendNoTexAlpha ? 1 : 0);
	
	if (bflags & hsGMatState::kBlendNoColor) {
		// dst = dst
		glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ZERO, GL_ONE);
	} else {
		switch (bflags & hsGMatState::kBlendMask) {
			case hsGMatState::kBlendDetail:
			case hsGMatState::kBlendAlpha:
			{
				GLenum srcAlpha = GL_ZERO;
				GLenum dstAlpha = GL_ONE;
				if(bflags & hsGMatState::kBlendAlphaAdd){
					srcAlpha = GL_ONE;
				}
				if(bflags & hsGMatState::kBlendAlphaMult){
					//	srcAlpha = GL_ZERO;
					//	dstAlpha = (bflags & hsGMatState::kBlendInvertFinalAlpha) ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;
				}
				// dst = a * src + (1-a) * dst
				// support a = 1 - a'
				if (bflags & hsGMatState::kBlendInvertFinalAlpha) {
					glBlendFuncSeparate(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, srcAlpha, dstAlpha);
				} else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, srcAlpha, dstAlpha);
				}
				break;
			}
			case hsGMatState::kBlendMult:
				// dst = src * dst
				// support src = 1 - src'
				if (bflags & hsGMatState::kBlendInvertFinalColor) {
					glBlendFuncSeparate(GL_ZERO, GL_ONE_MINUS_SRC_COLOR, GL_ZERO, GL_ONE);
				} else {
					glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
				}
				break;
				
			case hsGMatState::kBlendAdd:
				// dst = dst + src
				glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
				break;
			case hsGMatState::kBlendMADD:
				// dst = src * dest + dest
				glBlendFuncSeparate(GL_DST_COLOR, GL_ONE, GL_ZERO, GL_ONE);
				break;
			case hsGMatState::kBlendAddColorTimesAlpha:
				// dst = src * a + dst
				if (bflags & hsGMatState::kBlendInvertFinalAlpha) {
					glBlendFuncSeparate(GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
				} else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
				}
				break;
				
			case hsGMatState::kBlendDot3:
				// dst = src . dst
				Log::Warning() << "Unsupported blend mode." << std::endl;
				break;
			case hsGMatState::kBlendAddSigned:
				// dst = src + dst - 0.5
				Log::Warning() << "Unsupported blend mode." << std::endl;
				break;
			case hsGMatState::kBlendAddSigned2X:
				// dst = 2*(src + dst) - 1.0
				Log::Warning() << "Unsupported blend mode." << std::endl;
				break;
			case 0:
				// dst = src
				glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ZERO, GL_ONE);
				break;
				
			default:
				Log::Error() << "Blend mode doesn't exist." << std::endl;
				break;
		}
	}
	
	// Alpha threshold state (DONE).
	if (bflags & (hsGMatState::kBlendTest | hsGMatState::kBlendAlpha | hsGMatState::kBlendAddColorTimesAlpha)
		&& !(bflags & hsGMatState::kBlendAlphaAlways)) {
		if (bflags & hsGMatState::kBlendAlphaTestHigh) {
			glUniform1f(program->uniform("alphaThreshold"), 40.0f/255.0f);
		} else {
			glUniform1f(program->uniform("alphaThreshold"), 2.0f/255.0f);
		}
	} else {
		glUniform1f(program->uniform("alphaThreshold"), 0.f);
	}
	checkGLError();
}

void Object::textureState(const std::shared_ptr<ProgramInfos> & program, plLayerInterface* lay) const {
	TextureInfos infos;
	if(lay->getTexture().Exists()){
		infos = Resources::manager().getTexture(lay->getTexture()->getName().to_std_string());
		glUniformMatrix4fv(program->uniform("uvMatrix"), 1, GL_FALSE, lay->getTransform().glMatrix());
		if(infos.cubemap){
			glActiveTexture(GL_TEXTURE0+1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, infos.id);
			glUniform1i(program->uniform("useTexture"), 2);
		} else {
			glActiveTexture(GL_TEXTURE0+0);
			glBindTexture(GL_TEXTURE_2D, infos.id);
			glUniform1i(program->uniform("useTexture"), 1);
		}
		if(lay->getUVWSrc() == plLayer::kUVWNormal){
			glUniform1i(program->uniform("uvSource"), -1);
		} else if(lay->getUVWSrc() == plLayer::kUVWPosition){
			glUniform1i(program->uniform("uvSource"),-2);
		} else if(lay->getUVWSrc() == plLayer::kUVWReflect){
			glUniform1i(program->uniform("uvSource"), -3);
		} else {
			glUniform1i(program->uniform("uvSource"), lay->getUVWSrc() & plLayer::kUVWIdxMask);
		}
		
		if(lay->getState().fZFlags & hsGMatState::kZLODBias){
			glTexParameterf(infos.cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, lay->getLODBias());
		} else {
			glTexParameterf(infos.cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f);
		}
		
	} else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUniform1i(program->uniform("useTexture"), 0);
	}
	
	
	glUniform2i(program->uniform("clampedTexture"), lay->getState().fClampFlags & hsGMatState::kClampTextureU ? 1 : 0, lay->getState().fClampFlags & hsGMatState::kClampTextureV ? 1 : 0);
	glUniform1i(program->uniform("useReflectionXform"), lay->getState().fMiscFlags &  hsGMatState::kMiscUseReflectionXform ? 1 : 0);
	glUniform1i(program->uniform("useRefractionXform"), lay->getState().fMiscFlags &  hsGMatState::kMiscUseRefractionXform ? 1 : 0);
	checkGLError();
}

void Object::textureStateCustom(const std::shared_ptr<ProgramInfos> & program, plLayerInterface* lay) const {
	TextureInfos infos;
	if(lay->getTexture().Exists()){
		infos = Resources::manager().getTexture(lay->getTexture()->getName().to_std_string());
		glUniformMatrix4fv(program->uniform("uvMatrix1"), 1, GL_FALSE, lay->getTransform().glMatrix());
		if(infos.cubemap){
			Log::Error() << "Cubemap Alpha pseudo vertex not supported." << std::endl;
		} else {
			glActiveTexture(GL_TEXTURE0+2);
			glBindTexture(GL_TEXTURE_2D, infos.id);
			glUniform1i(program->uniform("useTexture1"), 1);
		}
		if(lay->getUVWSrc() == plLayer::kUVWNormal){
			glUniform1i(program->uniform("uvSource1"), -1);
		} else if(lay->getUVWSrc() == plLayer::kUVWPosition){
			glUniform1i(program->uniform("uvSource1"),-2);
		} else if(lay->getUVWSrc() == plLayer::kUVWReflect){
			glUniform1i(program->uniform("uvSource1"), -3);
		} else {
			glUniform1i(program->uniform("uvSource1"), lay->getUVWSrc() & plLayer::kUVWIdxMask);
		}
		if(lay->getState().fZFlags & hsGMatState::kZLODBias){
			glTexParameterf(infos.cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, lay->getLODBias());
		} else {
			glTexParameterf(infos.cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f);
		}
	} else {
		//infos = Resources::manager().getTexture("");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUniform1i(program->uniform("useTexture1"), 0);
	}
	glUniform2i(program->uniform("clampedTexture1"), lay->getState().fClampFlags & hsGMatState::kClampTextureU ? 1 : 0, lay->getState().fClampFlags & hsGMatState::kClampTextureV ? 1 : 0);
	glUniform1i(program->uniform("useReflectionXform1"), lay->getState().fMiscFlags &  hsGMatState::kMiscUseReflectionXform ? 1 : 0);
	glUniform1i(program->uniform("useRefractionXform1"), lay->getState().fMiscFlags &  hsGMatState::kMiscUseRefractionXform ? 1 : 0);
	checkGLError();
}

void Object::renderLayer(const std::shared_ptr<SubObject> & subObject, plLayerInterface * lay, const int tid) const {
	
	const bool forceDecal = subObject->material->getCompFlags() & hsGMaterial::kCompDecal;
	
	resetState();
	depthState(lay, forceDecal, tid);
	shadeState(_program, lay, subObject->mode);
	blendState(_program, lay);
	textureState(_program, lay);
	
	// Render.
	glBindVertexArray(subObject->mesh.vId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subObject->mesh.eId);
	glDrawElements(GL_TRIANGLES, subObject->mesh.count, GL_UNSIGNED_INT, (void*)0);
	
	// Reset states.
	glBindVertexArray(0);
	glDisable(GL_BLEND);
	checkGLError();
}

void Object::renderLayerMult(const std::shared_ptr<SubObject> & subObject, plLayerInterface * lay0, plLayerInterface * lay1, const int tid) const {
	
	const bool forceDecal = subObject->material->getCompFlags() & hsGMaterial::kCompDecal;
	// Set everything as usual first.
	const auto & program = Resources::manager().getProgram("object_special");
	resetState();
	depthState(lay0, forceDecal, tid);
	shadeState(program,lay0, subObject->mode);
	blendState(program,lay0);
	glUniform1i(program->uniform("invertVertexAlpha1"), lay1->getState().fBlendFlags & hsGMatState::kBlendInvertVtxAlpha ? 1 : 0);
	textureState(program,lay0);
	textureStateCustom(program, lay1);
	
	// Render.
	glBindVertexArray(subObject->mesh.vId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subObject->mesh.eId);
	glDrawElements(GL_TRIANGLES, subObject->mesh.count, GL_UNSIGNED_INT, (void*)0);
	
	// Reset states.
	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glUseProgram(_program->id());
	checkGLError();
}



void Object::clean() const {
	// Deleted in the Resources manager.
}


