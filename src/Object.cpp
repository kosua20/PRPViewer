#include "Object.hpp"
#include "helpers/Logger.hpp"
#include <stdio.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>


Object::Object(std::shared_ptr<ProgramInfos> prog) {
	_program = prog;
}

Object::~Object() {}


void Object::addSubObject(const MeshInfos & infos){
	_meshes.push_back(infos);
}

void Object::update(const glm::mat4& model) {

	//_model = model;

}


void Object::draw(const glm::mat4& view, const glm::mat4& projection) const {

	// Combine the three matrices.
	glm::mat4 MV = view;// * _model;
	glm::mat4 MVP = projection * MV;
	
	// Compute the normal matrix
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
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
	
	for(const auto & mesh : _meshes){
		glBindVertexArray(mesh.vId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.eId);
		glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, (void*)0);
	}
	glBindVertexArray(0);
	glUseProgram(0);

}



void Object::clean() const {
	// FIXME: won't support shared meshes.
	for(const auto & mesh : _meshes){
		glDeleteVertexArrays(1, &mesh.vId);
	}
//
//	for (auto & texture : _textures) {
//		glDeleteTextures(1, &(texture.id));
//	}
//	glDeleteProgram(_program->id());
}


