#include "Object.hpp"
#include "helpers/Logger.hpp"
#include <stdio.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <PRP/Surface/hsGMaterial.h>
#include <PRP/Surface/plLayer.h>

Object::Object(const Type & type, std::shared_ptr<ProgramInfos> prog, const glm::mat4 &model, const std::string & name) {
	_program = prog;
	_type = type;
	_model = glm::mat4(model);
	_name = name;
}

Object::~Object() {}


void Object::addSubObject(const MeshInfos & infos, hsGMaterial * material){
	if(_subObjects.empty()){
		_localBounds = infos.bbox;
	} else {
		_localBounds += infos.bbox;
	}
	_subObjects.push_back({infos, material});
	
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

	// Combine the three matrices.
	
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
	glm::mat4 MVP = projection * MV;
	
	// Compute the normal matrix
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
	
	
	glEnable(GL_BLEND);
	// Select the program (and shaders).
	glUseProgram(_program->id());

	// Upload the MVP matrix.
	glUniformMatrix4fv(_program->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
	
	// Upload the normal matrix.
	glUniformMatrix3fv(_program->uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);

	// Bind the textures.
	/*for (unsigned int i = 0; i < _textures.size(); ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(_textures[i].cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, _textures[i].id);
	}*/

	const TextureInfos emptyInfos = Resources::manager().getTexture("");
	//const glm::mat4 ident(1.0f);
	for(const auto & subObject : _subObjects){
		int tid = 0;
		for(const auto & layKey : subObject.material->getLayers()){
			if(tid>=8){
				Log::Warning() << "8 layers reached, stopping." << std::endl;
				break;
			}
			
			plLayerInterface * lay = plLayerInterface::Convert(layKey->getObj(), false);
			TextureInfos infos;
			const std::string arrayPos = "[" + std::to_string(tid) + "]";
			if(lay->getTexture().Exists()){
				infos = Resources::manager().getTexture(lay->getTexture()->getName().to_std_string());
				glUniformMatrix4fv(_program->uniform("uvMatrix"+arrayPos), 1, GL_FALSE, lay->getTransform().glMatrix());
				if(infos.cubemap){
					glActiveTexture(GL_TEXTURE0+8+tid);
					glBindTexture(GL_TEXTURE_CUBE_MAP, infos.id);
					glUniform1i(_program->uniform("useTexture"+arrayPos), 2);
				} else {
					glActiveTexture(GL_TEXTURE0+tid);
					glBindTexture(GL_TEXTURE_2D, infos.id);
					glUniform1i(_program->uniform("useTexture"+arrayPos), 1);
				}
				glUniform1i(_program->uniform("uvSource"+arrayPos), lay->getUVWSrc());
			} else {
				infos = emptyInfos;
				glActiveTexture(GL_TEXTURE0+tid);
				glBindTexture(GL_TEXTURE_2D, infos.id);
				glUniform1i(_program->uniform("useTexture"+arrayPos), 0);
			}
			glUniform1i(_program->uniform("clampTexture"+arrayPos), lay->getState().fClampFlags & hsGMatState::kClampTexture ? 1 : 0);
			
			const unsigned int fshade = lay->getState().fShadeFlags;
			
			const auto ambient = lay->getAmbient();
			const auto prediff = (fshade & hsGMatState::kShadeWhite) ? hsColorRGBA(1.0f,1.0f,1.0f,1.0f) : lay->getPreshade();
			const auto diffuse = lay->getRuntime(); const float opac = lay->getOpacity();
			const auto specular = (fshade & hsGMatState::kShadeSpecular) ? lay->getSpecular() : hsColorRGBA(0.0f,0.0f,0.0f,0.0f); const float specpow = lay->getSpecularPower();
			
			glUniform3f(_program->uniform("ambient"+arrayPos), ambient.r, ambient.g, ambient.b);
			glUniform3f(_program->uniform("prediff"+arrayPos), prediff.r, prediff.g, prediff.b);
			glUniform4f(_program->uniform("diffuse"+arrayPos), diffuse.r, diffuse.g, diffuse.b, opac);
			glUniform4f(_program->uniform("specular"+arrayPos), specular.r, specular.g, specular.b, specpow);
			
			if(lay->getState().fZFlags & hsGMatState::kZNoZWrite){
				glDepthMask(GL_FALSE);
			}
			if(lay->getState().fZFlags & hsGMatState::kZNoZRead){
				glDisable(GL_DEPTH_TEST);
			}
			if(lay->getState().fMiscFlags & hsGMatState::kMiscTwoSided){
				glDisable(GL_CULL_FACE);
			}
			
			++tid;
			if(lay->getState().fZFlags & hsGMatState::kZIncLayer){
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(0.0f, -tid*20.0f);
			}
			
			
		}
		glUniform1i(_program->uniform("layerCount"), tid);
		
		
		
		glBindVertexArray(subObject.mesh.vId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subObject.mesh.eId);
		glDrawElements(GL_TRIANGLES, subObject.mesh.count, GL_UNSIGNED_INT, (void*)0);
		
		// Restore state after each subelement that can have a different material.
		if(!(_type==Billboard) && !(_type==BillboardY)){
			glEnable(GL_CULL_FACE);
		}
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glPolygonOffset(0.0f, 0.0f);
		glDisable(GL_POLYGON_OFFSET_FILL);
		
	}
	glBindVertexArray(0);
	glUseProgram(0);
	glDisable(GL_BLEND);
	
	
}



void Object::clean() const {
	
}


