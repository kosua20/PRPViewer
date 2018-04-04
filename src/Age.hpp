#ifndef Age_h
#define Age_h

#include <string>

class plResManager;
class plLocation;

class Age {
public:
	 Age(const std::string & path);
	
	const std::string& getName(){
		return _name;
	}
	
private:
	
	void loadMeshes( plResManager & rm, const plLocation& ploc);
	
	std::string _name;
	std::shared_ptr<plResManager> _rm;
};

#endif
