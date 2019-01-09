#ifndef Renderer_h
#define Renderer_h
#include "Config.hpp"
#include "Age.hpp"
#include "input/Camera.hpp"
#include "ScreenQuad.hpp"
#include "Object.hpp"
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
	float _resolutionScaling;
	
private:
	
	std::shared_ptr<Age> _age;
	ScreenQuad _quad;
	ScreenQuad _fxaaquad;
	Camera _camera;
	std::shared_ptr<Framebuffer> _sceneFramebuffer;
	
	
	bool _wireframe = true;
	bool _doCulling = true;
	float _cullingDistance = 1500.0f;
	int _drawCount = 0;
	bool _forceLighting;
	bool _forceNoLighting;
	float _cameraFarPlane;
	float _cameraFOV;
	float _clearColor[3];
	bool _vertexOnly;
	bool _showDot;
	
	enum DisplayMode {
		Scene = 0, OneObject = 1, OneTexture = 2
	};
	
	DisplayMode _displayMode;
	int _objectId = 0;
	int _textureId = 0;
	int _subObjectId = -1;
	int _subLayerId = -1;
	
	void defaultGLSetup();
	void loadAge(const std::string & path);
};

#endif

