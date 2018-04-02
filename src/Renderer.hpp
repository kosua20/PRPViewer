#ifndef Renderer_h
#define Renderer_h
#include "Config.hpp"
#include "Age.hpp"
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>



class Renderer {
	
public:
	
	~Renderer();
	
	Renderer(Config & config);
	
	/// Draw function
	void draw();
	
	void update();
	
	void physics(double fullTime, double frameTime);
	
	/// Clean function
	void clean() const;
	
	/// Handle screen resizing
	void resize(int width, int height);
	
	
protected:
	
	void updateResolution(int width, int height);
	
	Config & _config;
	glm::vec2 _renderResolution;
	
private:
	
	void defaultGLSetup();
};

#endif

