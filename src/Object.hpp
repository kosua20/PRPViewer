#ifndef Object_h
#define Object_h
#include "resources/ResourcesManager.hpp"
#include <PRP/Region/hsBounds.h>
#include <PRP/Surface/plLayerInterface.h>
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


class hsGMaterial;

class Object {

	
public:

	enum Type {
		Default = 0,
		Billboard = 1,
		BillboardY = 2
	};
	
	struct SubObject {
		MeshInfos mesh;
		hsGMaterial * material;
		unsigned int mode;
	};
	
	Object(const Type & type, std::shared_ptr<ProgramInfos> prog, const glm::mat4 & model, const std::string & name);

	~Object();
	
	void addSubObject(const MeshInfos & infos, hsGMaterial * material, const unsigned int shadingMode);
	
	/// Update function
	void update(const glm::mat4& model);
	
	/// Draw function
	void drawDebug(const glm::mat4& view, const glm::mat4& projection, const int subObject = -1) const;
	
	void draw(const glm::mat4& view, const glm::mat4& projection, const int subObject = -1, const int layer = -1) const;
	
	/// Clean function
	void clean() const;
	
	const std::string & getName() const { return _name; }

	const glm::vec3 & getCenter() const { return _globalBounds.center; }
	
	const bool isVisible(const glm::vec3 & point, const glm::vec3 & dir) const;
	
	const bool isVisible(const glm::vec3 & point, const glm::mat4 & viewproj) const;
	
	const std::vector<SubObject> & subObjects(){ return _subObjects; }
	
private:
	
	void renderLayer(const SubObject & subObject, plLayerInterface * lay, const int tid) const;
	void renderLayerMult(const SubObject & subObject, plLayerInterface * lay0, plLayerInterface * lay1, const int tid) const;
	
	void resetState() const;
	void depthState(plLayerInterface* lay, const bool forceDecal, const int tid) const;
	void shadeState(const std::shared_ptr<ProgramInfos> & program, plLayerInterface* lay, unsigned int mode) const;
	void blendState(const std::shared_ptr<ProgramInfos> & program, plLayerInterface * lay) const;
	void textureState(const std::shared_ptr<ProgramInfos> & program, plLayerInterface* lay) const;
	void textureStateCustom(const std::shared_ptr<ProgramInfos> & program, plLayerInterface* lay) const;
	std::shared_ptr<ProgramInfos> _program;
	
	std::vector<SubObject> _subObjects;
	
	Type _type;
	glm::mat4 _model;
	std::string _name;
	BoundingBox _localBounds;
	BoundingBox _globalBounds;
};

#endif
