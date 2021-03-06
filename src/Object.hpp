#ifndef Object_h
#define Object_h
#include "resources/ResourcesManager.hpp"
#include <PRP/Region/hsBounds.h>
#include <PRP/Surface/plLayerInterface.h>
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

struct Light {
	
	enum Type {
		DIR, OMNI, SPOT
	} type;
	
	glm::vec4 posdir;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular; //unused?
	float constAtten;
	float linAtten;
	float quadAtten;
	float scale;
};

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
		bool transparent;
		std::vector<Light> lights;
		
		SubObject(MeshInfos amesh, hsGMaterial * amaterial, const std::vector<Light> & alights, unsigned int amode, bool atransparent){
			mesh = amesh;
			material = amaterial;
			mode = amode;
			transparent = atransparent;
			lights = alights;
		}
	};
	
	Object(const Type & type, std::shared_ptr<ProgramInfos> prog, const glm::mat4 & model, const std::string & name);

	~Object();
	
	void addSubObject(const MeshInfos & infos, hsGMaterial * material, const std::vector<Light> & lights, const unsigned int shadingMode);
	
	/// Draw function
	void drawDebug(const glm::mat4& view, const glm::mat4& projection, const int subObject = -1) const;
	
	void draw(const glm::mat4& view, const glm::mat4& projection, const int subObject = -1, const int layer = -1) const;
	
	/// Clean function
	void clean() const;
	
	const std::string & getName() const { return _name; }

	const glm::vec3 & getCenter() const { return _globalBounds.center; }
	
	const bool isVisible(const glm::vec3 & point, const glm::vec3 & dir) const;
	
	const bool isVisible(const glm::vec3 & point, const glm::mat4 & viewproj) const;
	
	const bool contains( const std::shared_ptr<Object> & other) const;
	
	const std::vector<std::shared_ptr<SubObject>> & subObjects(){ return _subObjects; }
	
	bool enabled;
	
	const bool transparent(){ return _transparent; }
	
	const bool billboard(){ return _billboard; }
	
	const bool probablySky(){ return _probablySky; }
	
private:
	
	void renderLayer(const std::shared_ptr<SubObject> & subObject, plLayerInterface * lay, const int tid) const;
	void renderLayerMult(const std::shared_ptr<SubObject> & subObject, plLayerInterface * lay0, plLayerInterface * lay1, const int tid) const;
	
	void setupLights(const std::shared_ptr<ProgramInfos> & program, const std::vector<Light> & lights, const glm::mat4 & view) const;
	void resetState() const;
	void depthState(plLayerInterface* lay, const bool forceDecal, const int tid) const;
	void shadeState(const std::shared_ptr<ProgramInfos> & program, plLayerInterface* lay, unsigned int mode) const;
	void blendState(const std::shared_ptr<ProgramInfos> & program, plLayerInterface * lay) const;
	void textureState(const std::shared_ptr<ProgramInfos> & program, plLayerInterface* lay) const;
	void textureStateCustom(const std::shared_ptr<ProgramInfos> & program, plLayerInterface* lay) const;
	std::shared_ptr<ProgramInfos> _program;
	
	std::vector<std::shared_ptr<SubObject>> _subObjects;
	
	
	Type _type;
	glm::mat4 _model;
	std::string _name;
	BoundingBox _localBounds;
	BoundingBox _globalBounds;
	
	glm::vec3 _centroid;
	bool _transparent;
	bool _billboard;
	bool _probablySky;
};

#endif
