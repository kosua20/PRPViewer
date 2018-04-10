#ifndef Age_h
#define Age_h

#include "Object.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

class plResManager;
class plLocation;

class Age {
public:
	Age(const std::string & path);
	~Age();
	
	const std::string& getName(){
		return _name;
	}
	
	const std::vector<Object> & objects(){
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
	
private:
	
	void loadMeshes( plResManager & rm, const plLocation& ploc);
	
	std::string _name;
	//std::shared_ptr<plResManager> _rm;
	std::vector<Object> _objects;
	std::vector<std::string> _textures;
	std::map<std::string, glm::vec3> _linkingPoints;
	std::vector<std::string> _linkingNamesCache;
	
};

#endif
