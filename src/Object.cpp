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

	
	for(const auto & subObject : _subObjects){
		plKey tex = plLayerInterface::Convert(subObject.material->getLayers()[0]->getObj(), false)->getTexture();
		const TextureInfos infos = Resources::manager().getTexture(tex != NULL ? tex->getName().to_std_string() : "");//(^Resources::manager().getTexture(subObject.material->getLayers()[0]->getObj());
		
		//Log::Info() << subObject.mesh.uvCount << std::endl;
		if(infos.cubemap){
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, infos.id);
			glUniform1i(_program->uniform("useCubemap"), 1);
		} else {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, infos.id);
			glUniform1i(_program->uniform("useCubemap"), 0);
		}
		glBindVertexArray(subObject.mesh.vId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subObject.mesh.eId);
		glDrawElements(GL_TRIANGLES, subObject.mesh.count, GL_UNSIGNED_INT, (void*)0);
	}
	glBindVertexArray(0);
	glUseProgram(0);
	glDisable(GL_BLEND);
	if(_type == Billboard || _type == BillboardY){
		glEnable(GL_CULL_FACE);
	}
	
}



void Object::clean() const {
	
}


