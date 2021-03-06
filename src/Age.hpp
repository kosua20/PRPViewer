#ifndef Age_h
#define Age_h

#include "Object.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

class plResManager;
class plLocation;
class plFogEnvironment;

class Age {
public:
	
	Age();
	
	Age(const std::string & path);
	
	~Age();
	
	const std::string& getName(){
		return _name;
	}
	
	const std::vector<std::shared_ptr<Object>> & objects(){
		return _objects;
	}
	
	std::vector<std::shared_ptr<Object>> objectsClone(){
		return _objects;
	}
	
	const std::vector<std::string> & textures(){
		return _textures;
	}
	
	const glm::vec3 getDefaultLinkingPoint();
	
	const std::map<std::string, glm::vec3> & linkingPoints(){
		return _linkingPoints;
	}
	const std::vector<std::string> & linkingNames(){
		return _linkingNamesCache;
	}
	
	const size_t maxLayer(){
		return _maxLayer;
	}
	
	const glm::vec3 & clearColor(){
		return _clearColor;
	}
	
	const plFogEnvironment * getFog(){
		return _fogEnv;
	}
	
private:
	
	std::shared_ptr<ProgramInfos> generateShaders(hsGMaterial * mat);
	
	void loadMeshes( plResManager & rm, const plLocation& ploc);
	
	std::string _name;
	std::shared_ptr<plResManager> _rm;
	std::vector<std::shared_ptr<Object>> _objects;
	std::vector<std::string> _textures;
	std::map<std::string, glm::vec3> _linkingPoints;
	std::vector<std::string> _linkingNamesCache;
	
	glm::vec3 _clearColor;
	
	size_t _maxLayer = 0;
	
	plFogEnvironment * _fogEnv;
};

#endif
