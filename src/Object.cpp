#include "Object.hpp"
#include "helpers/Logger.hpp"
#include <stdio.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <PRP/Surface/hsGMaterial.h>
#include <PRP/Surface/plLayer.h>
#include <PRP/Geometry/plSpan.h>

Object::Object(const Type & type, std::shared_ptr<ProgramInfos> prog, const glm::mat4 &model, const std::string & name) {
	_program = prog;
	_type = type;
	_model = glm::mat4(model);
	_name = name;
}

Object::~Object() {}


void Object::addSubObject(const MeshInfos & infos, hsGMaterial * material, const unsigned int shadingMode){
	if(_subObjects.empty()){
		_localBounds = infos.bbox;
	} else {
		_localBounds += infos.bbox;
	}
	_subObjects.push_back({infos, material, shadingMode});
	
	_localBounds.updateValues();
	_globalBounds = _localBounds.transform(_model);
}

void Object::update(const glm::mat4& model) {
	_model = model;
	_globalBounds = _localBounds.transform(_model);
}

const bool Object::isVisible(const glm::vec3 & point, const glm::mat4 & viewproj) const {
	return _globalBounds.contains(point) || _globalBounds.intersectsFrustum(viewproj);
}

const bool Object::isVisible(const glm::vec3 & point, const glm::vec3 & dir) const {
	return _globalBounds.contains(point) || _globalBounds.isVisible(point, dir);
}

void Object::drawDebug(const glm::mat4& view, const glm::mat4& projection) const {
	
	//Log::Info() << _bounds.getCenter() << std::endl;
	glm::mat4 MV = view * _model;
	if(_type == Billboard || _type == BillboardY){
		glm::mat4 viewCopy = glm::transpose(glm::mat4(glm::mat3(view)));
		if(_type == BillboardY){
			viewCopy[1][0] = 0.0;
			viewCopy[1][1] = 1.0;
			viewCopy[1][2] = 0.0;
		}
		MV = view * glm::translate(glm::mat4(1.0f), glm::vec3(_model[3][0],_model[3][1],_model[3][2])) * viewCopy  * glm::scale(glm::mat4(1.0f), glm::vec3(_model[0][0]));
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
	for(const auto & subObject : _subObjects){
		glBindVertexArray(subObject.mesh.vId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subObject.mesh.eId);
		glDrawElements(GL_TRIANGLES, subObject.mesh.count, GL_UNSIGNED_INT, (void*)0);
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

void Object::draw(const glm::mat4& view, const glm::mat4& projection) const {

	glUseProgram(_program->id());
	
	// Compute final view and model matrices.
	glm::mat4 MV = view * _model;
	if(_type == Billboard || _type == BillboardY){
		glDisable(GL_CULL_FACE);
		glm::mat4 viewCopy = glm::transpose(glm::mat4(glm::mat3(view)));
		if(_type == BillboardY){
			viewCopy[1][0] = 0.0;
			viewCopy[1][1] = 1.0;
			viewCopy[1][2] = 0.0;
		}
		MV = view * glm::translate(glm::mat4(1.0f), glm::vec3(_model[3][0],_model[3][1],_model[3][2])) * viewCopy  * glm::scale(glm::mat4(1.0f), glm::vec3(_model[0][0]));
	}
	const glm::mat4 MVP = projection * MV;
	const glm::mat4 invV = glm::inverse(view);
	const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
	
	// Send common infos.
	glUniformMatrix4fv(_program->uniform("mv"), 1, GL_FALSE, &MV[0][0]);
	glUniformMatrix4fv(_program->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(_program->uniform("invV"), 1, GL_FALSE, &invV[0][0]);
	glUniformMatrix3fv(_program->uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
	
	const TextureInfos emptyInfos = Resources::manager().getTexture("");
	
	for(const auto & subObject : _subObjects){
		int tid = 0;
		// Render each layer, one after the other.
		for(const auto & layKey : subObject.material->getLayers()){
			if(tid>=8){
				Log::Warning() << "8 layers reached." << std::endl;
			}
			// Obtain the layer to aply.
			plLayerInterface * lay = plLayerInterface::Convert(layKey->getObj(), false);
			
			// Setup Z state.
			if(lay->getState().fZFlags & hsGMatState::kZNoZWrite){
				glDepthMask(GL_FALSE);
			}
			if(lay->getState().fZFlags & hsGMatState::kZNoZRead){
				glDisable(GL_DEPTH_TEST);
			}
			if(lay->getState().fMiscFlags & hsGMatState::kMiscTwoSided){
				glDisable(GL_CULL_FACE);
			}
			glUniform1i(_program->uniform("layerCount"), tid);
			
			if(lay->getState().fZFlags & hsGMatState::kZIncLayer){
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(0.0f, -(tid+1)*20.0f);
			}
			checkGLError();
			
			// Setup shading state.
			const unsigned int fshade = lay->getState().fShadeFlags;
			
			switch(subObject.mode){
				case plSpan::kLiteMaterial:
				{
					// Ambient.
					if(fshade & hsGMatState::kShadeWhite){
						glUniform4f(_program->uniform("globalAmbient"), 1.0f, 1.0f, 1.0f, 1.0f);
						glUniform4f(_program->uniform("ambient"), 1.0f, 1.0f, 1.0f, 1.0f);
					} else {
						const hsColorRGBA amb = lay->getPreshade();
						glUniform4f(_program->uniform("globalAmbient"), amb.r, amb.g, amb.b, 1.0f);
						glUniform4f(_program->uniform("ambient"), amb.r, amb.g, amb.b, 1.0f);
					}
					const hsColorRGBA dif = lay->getRuntime();
					const hsColorRGBA emi = lay->getAmbient();
					glUniform4f(_program->uniform("diffuse"), dif.r, dif.g, dif.b, lay->getOpacity());
					glUniform4f(_program->uniform("emissive"), emi.r, emi.g, emi.b, 1.0f);
					
					// Specular.
					if (fshade & hsGMatState::kShadeSpecular) {
						const hsColorRGBA spec = lay->getSpecular();
						glUniform4f(_program->uniform("specular"), spec.r, spec.g, spec.b, 1.0f);
					} else {
						glUniform4f(_program->uniform("specular"), 0.0f, 0.0f, 0.0f, 0.0f);
					}
					
					glUniform1f(_program->uniform("diffuseSrc"), 1.0f);
					glUniform1f(_program->uniform("specularSrc"), 1.0f);
					glUniform1f(_program->uniform("emissiveSrc"), 1.0f);
					
					if (fshade & hsGMatState::kShadeNoShade) {
						glUniform1f(_program->uniform("ambientSrc"), 1.0f);
					} else {
						glUniform1f(_program->uniform("ambientSrc"), 0.0f);
					}
					break;
				}
				case plSpan::kLiteVtxNonPreshaded:
				{
					const hsColorRGBA amb = lay->getPreshade();
					const hsColorRGBA emi = lay->getAmbient();
					
					glUniform4f(_program->uniform("globalAmbient"), amb.r, amb.g, amb.b, amb.a);
					glUniform4f(_program->uniform("ambient"), 0.0f, 0.0f, 0.0f, 0.0f);
					glUniform4f(_program->uniform("diffuse"), 0.0f, 0.0f, 0.0f, 0.0f);
					glUniform4f(_program->uniform("emissive"), emi.r, emi.g, emi.b, 1.0f);
					
					if (fshade & hsGMatState::kShadeSpecular) {
						const hsColorRGBA spec = lay->getSpecular();
						glUniform4f(_program->uniform("specular"), spec.r, spec.g, spec.b, 1.0f);
					} else {
						glUniform4f(_program->uniform("specular"), 0.0f, 0.0f, 0.0f, 0.0f);
					}
					
					glUniform1f(_program->uniform("diffuseSrc"), 0.0f);
					glUniform1f(_program->uniform("ambientSrc"), 0.0f);
					glUniform1f(_program->uniform("specularSrc"), 1.0f);
					glUniform1f(_program->uniform("emissiveSrc"), 1.0f);
					break;
				}
				case plSpan::kLiteVtxPreshaded:
				{
					glUniform4f(_program->uniform("globalAmbient"), 0.0f, 0.0f, 0.0f, 0.0f);
					glUniform4f(_program->uniform("ambient"), 0.0f, 0.0f, 0.0f, 0.0f);
					glUniform4f(_program->uniform("diffuse"), 0.0f, 0.0f, 0.0f, 0.0f);
					glUniform4f(_program->uniform("emissive"), 0.0f, 0.0f, 0.0f, 0.0f);
					glUniform4f(_program->uniform("specular"), 0.0f, 0.0f, 0.0f, 0.0f);
					glUniform1f(_program->uniform("diffuseSrc"), 0.0f);
					glUniform1f(_program->uniform("ambientSrc"), 1.0f);
					glUniform1f(_program->uniform("specularSrc"), 1.0f);
					glUniform1f(_program->uniform("emissiveSrc"), (fshade & hsGMatState::kShadeEmissive) ? 0.0f : 1.0f);
					break;
				}
				default:
				break;
			}
			checkGLError();
			
			// Setup blending state.
			glEnable(GL_BLEND);
			const unsigned int fblend = lay->getState().fBlendFlags;
			glUniform1i(_program->uniform("invertAlpha"), fblend & hsGMatState::kBlendInvertAlpha);
			
			glUniform1i(_program->uniform("useReflectionXform"), lay->getState().fMiscFlags &  hsGMatState::kMiscUseReflectionXform);
			glUniform1i(_program->uniform("useRefractionXform"), lay->getState().fMiscFlags &  hsGMatState::kMiscUseRefractionXform);
			
			glUniform1i(_program->uniform("blendInvertColor"), fblend & hsGMatState::kBlendInvertColor);
			glUniform1i(_program->uniform("blendNoTexColor"), fblend & hsGMatState::kBlendNoTexColor);
			glUniform1i(_program->uniform("blendInvertAlpha"), fblend & hsGMatState::kBlendInvertAlpha);
			glUniform1i(_program->uniform("blendNoVtxAlpha"), fblend & hsGMatState::kBlendNoVtxAlpha);
			glUniform1i(_program->uniform("blendNoTexAlpha"), fblend & hsGMatState::kBlendNoTexAlpha);
			glUniform1i(_program->uniform("blendAlphaAdd"), fblend & hsGMatState::kBlendAlphaAdd);
			glUniform1i(_program->uniform("blendAlphaMult"), fblend & hsGMatState::kBlendAlphaMult);
			
			switch(fblend & hsGMatState::kBlendMask){
				case hsGMatState::kBlendAddColorTimesAlpha:
				glUniform1i(_program->uniform("blendMode"), 1);
				break;
				case hsGMatState::kBlendAlpha:
				glUniform1i(_program->uniform("blendMode"), 2);
				break;
				case hsGMatState::kBlendAdd:
				glUniform1i(_program->uniform("blendMode"), 3);
				break;
				case hsGMatState::kBlendMult:
				glUniform1i(_program->uniform("blendMode"), 4);
				break;
				case hsGMatState::kBlendDot3:
				glUniform1i(_program->uniform("blendMode"), 5);
				break;
				case hsGMatState::kBlendAddSigned:
				glUniform1i(_program->uniform("blendMode"), 6);
				break;
				case hsGMatState::kBlendAddSigned2X:
				glUniform1i(_program->uniform("blendMode"), 7);
				break;
				default:
				break;
			}
			
			
			if (fblend & (hsGMatState::kBlendTest | hsGMatState::kBlendAlpha | hsGMatState::kBlendAddColorTimesAlpha)
				&& !(fblend & hsGMatState::kBlendAlphaAlways)) {
				if (fblend & hsGMatState::kBlendAlphaTestHigh) {
					glUniform1f(_program->uniform("alphaThreshold"), 40.f/255.f);
				} else {
					glUniform1f(_program->uniform("alphaThreshold"), 1.f/255.f);
				}
			} else {
				glUniform1f(_program->uniform("alphaThreshold"), 0.f);
			}
			checkGLError();
			
			// Setup texture state.
			TextureInfos infos;
			if(lay->getTexture().Exists()){
				infos = Resources::manager().getTexture(lay->getTexture()->getName().to_std_string());
				glUniformMatrix4fv(_program->uniform("uvMatrix"), 1, GL_FALSE, lay->getTransform().glMatrix());
				if(infos.cubemap){
					glActiveTexture(GL_TEXTURE0+1);
					glBindTexture(GL_TEXTURE_CUBE_MAP, infos.id);
					glUniform1i(_program->uniform("useTexture"), 2);
				} else {
					glActiveTexture(GL_TEXTURE0+0);
					glBindTexture(GL_TEXTURE_2D, infos.id);
					glUniform1i(_program->uniform("useTexture"), 1);
				}
				if(lay->getUVWSrc() == plLayer::kUVWNormal){
					glUniform1i(_program->uniform("uvSource"), -1);
				} else if(lay->getUVWSrc() == plLayer::kUVWPosition){
					glUniform1i(_program->uniform("uvSource"),-2);
				} else if(lay->getUVWSrc() == plLayer::kUVWReflect){
					glUniform1i(_program->uniform("uvSource"), -3);
				} else {
					glUniform1i(_program->uniform("uvSource"), lay->getUVWSrc() & plLayer::kUVWIdxMask);
				}
				//TODO:
			} else {
				infos = emptyInfos;
				glActiveTexture(GL_TEXTURE0+tid);
				glBindTexture(GL_TEXTURE_2D, infos.id);
				glUniform1i(_program->uniform("useTexture"), 0);
			}
			glUniform1i(_program->uniform("clampedTexture"), lay->getState().fClampFlags & hsGMatState::kClampTexture ? 1 : 0);
			checkGLError();
			
			// Render.
			glBindVertexArray(subObject.mesh.vId);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subObject.mesh.eId);
			glDrawElements(GL_TRIANGLES, subObject.mesh.count, GL_UNSIGNED_INT, (void*)0);
			checkGLError();
			
			// Reset states.
			glBindVertexArray(0);
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);
			glPolygonOffset(0.0f, 0.0f);
			glDisable(GL_POLYGON_OFFSET_FILL);
			checkGLError();
			
			++tid;
		}
		
		
		// Restore state after each subelement that can have a different material.
		if(!(_type==Billboard) && !(_type==BillboardY)){
			glEnable(GL_CULL_FACE);
		}
		
	}
	
	glUseProgram(0);
	glDisable(GL_BLEND);
	checkGLError();
	
}



void Object::clean() const {
	
}


