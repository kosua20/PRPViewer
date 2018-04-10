#ifndef Object_h
#define Object_h
#include "resources/ResourcesManager.hpp"

#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Object {

	
public:

	enum Type {
		Default = 0,
		Billboard = 1,
		BillboardY = 2
	};
	
	Object(const Type & type, std::shared_ptr<ProgramInfos> prog, const glm::mat4 & model);

	~Object();
	
	void addSubObject(const MeshInfos & infos);
	
	/// Update function
	void update(const glm::mat4& model);
	
	/// Draw function
	void draw(const glm::mat4& view, const glm::mat4& projection) const;
	
	/// Clean function
	void clean() const;
	

private:
	
	std::shared_ptr<ProgramInfos> _program;
	
	std::vector<MeshInfos> _meshes;
	
	Type _type;
	glm::mat4 _model;
};

#endif
