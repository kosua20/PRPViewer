#include "Renderer.hpp"
#include "Object.hpp"
#include "input/Input.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "helpers/Logger.hpp"
#include <glm/gtx/norm.hpp>
#include <stdio.h>
#include <vector>


Renderer::~Renderer(){}

Renderer::Renderer(Config & config) : _config(config) {
	
	// Initial render resolution.
	_renderResolution = (_config.internalVerticalResolution/_config.screenResolution[1]) * _config.screenResolution;
	
	defaultGLSetup();
	
	_quad.init("passthrough");
	// Setup camera parameters.
	_camera.projection(config.screenResolution[0]/config.screenResolution[1], 1.3f, 0.1f, 8000.0f);
	Resources::manager().getProgram("object_basic")->registerTexture("texture0", 0);
	Resources::manager().getProgram("object_basic")->registerTexture("cubemap1", 1);

	
	loadAge("../../../data/uru/spyroom.age");
}

void Renderer::draw(){
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glClearColor(0.45f,0.45f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// FIXME: move to attributes.
	static int textureId = 0;
	static bool showTextures = false;
	static int objectId = 0;
	static bool showObject = false;
	static bool wireframe = false;
	static bool doCulling = true;
	static float cullingDistance = 1500.0f;
	static int drawCount = 0;
	if (ImGui::Begin("Infos")) {
		const std::string nameStr = "Age: " + (_age ? _age->getName() : "None");
		ImGui::Text("%s", nameStr.c_str());
		const std::string nameStr1 = "Draws: " + std::to_string(drawCount) + "/" + std::to_string(_age->objects().size());
		ImGui::Text("%s", nameStr1.c_str());
		if(showTextures){
			const std::string txtStr = "Current: " + _age->textures()[textureId];
			ImGui::Text("%s", txtStr.c_str());
		}
		if(showObject){
			const std::string txtStr = "Current: " + _age->objects()[objectId]->getName();
			ImGui::Text("%s", txtStr.c_str());
		}
	}
	ImGui::End();
	
	if (ImGui::Begin("Settings")) {
		static int current_item_id = 0;
		
		if(ImGui::Button("Load .age file...")){
			ImGui::OpenFilePicker("Load Age");
		}
		std::string selectedFile;
		if(ImGui::BeginFilePicker("Load Age", "Load a .age file.", "../../../data/", selectedFile, false, false, {"age"})){
			loadAge(selectedFile);
			current_item_id = 0;
			textureId = 0;
			showTextures = false;
		}
		
		
		auto linkingNameProvider = [](void* data, int idx, const char** out_text) {
			const std::vector<std::string>* arr = (std::vector<std::string>*)data;
			*out_text = (*arr)[idx].c_str();
			return true;
		};
		if (ImGui::Combo("Linking point", &current_item_id, linkingNameProvider, (void*)&(_age->linkingNames()), _age->linkingNames().size())) {
			if (current_item_id >= 0) {
				_camera.setCenter(_age->linkingPoints().at(_age->linkingNames()[current_item_id]));
			}
			
		}
		
		
		ImGui::Checkbox("Wireframe", &wireframe);
		ImGui::Checkbox("Culling", &doCulling);
		ImGui::SliderFloat("Culling dist.", &cullingDistance, 10.0f, 3000.0f);
		ImGui::Checkbox("Show textures", &showTextures);
		ImGui::SliderInt("Texture ID", &textureId, 0, _age->textures().size()-1);
		
		ImGui::Checkbox("Show object", &showObject);
		ImGui::SliderInt("Object ID", &objectId, 0, _age->objects().size()-1);
		
	}
	ImGui::End();
	
	if(showTextures){
		_quad.draw(Resources::manager().getTexture(_age->textures()[textureId]).id);
		return;
	}
	
	glEnable(GL_DEPTH_TEST);
	checkGLError();
	if(showObject){
		const auto objectToShow = _age->objects()[objectId];
		
		if(wireframe){
			objectToShow->drawDebug(_camera.view() , _camera.projection());
		} else {
			objectToShow->draw(_camera.view() , _camera.projection());
		}
		
	} else {
		//const glm::mat4 viewproj = _camera.projection() * _camera.view();
		drawCount = 0;
		for(const auto & object : _age->objects()){
			if(doCulling &&
			   (glm::length2(object->getCenter() - _camera.getPosition()) > cullingDistance*cullingDistance
				|| !object->isVisible(_camera.getPosition(), _camera.getDirection())
				)){
				continue;
			}
			++drawCount;
			if(wireframe){
				object->drawDebug(_camera.view() , _camera.projection());
			} else {
				object->draw(_camera.view() , _camera.projection());
			}
		}
	}
	glDisable(GL_DEPTH_TEST);
	
	checkGLError();
}

void Renderer::loadAge(const std::string & path){
	Log::Info() << "Should load " << path << std::endl;
	Resources::manager().reset();
	_age.reset(new Age(path));
	// A Uru human is around 4/5 units in height apparently.
	_camera.setCenter(_age->getDefaultLinkingPoint());
}
void Renderer::update(){
	if(Input::manager().resized()){
		resize((int)Input::manager().size()[0], (int)Input::manager().size()[1]);
	}
	_camera.update();
}

void Renderer::physics(double fullTime, double frameTime){
	_camera.physics(frameTime);
}

/// Clean function
void Renderer::clean() const {
	Resources::manager().reset();
}

/// Handle screen resizing
void Renderer::resize(int width, int height){
	updateResolution(width, height);
	// Update the projection aspect ratio.
	_camera.ratio(_renderResolution[0] / _renderResolution[1]);
}



void Renderer::updateResolution(int width, int height){
	_config.screenResolution[0] = float(width > 0 ? width : 1);
	_config.screenResolution[1] = float(height > 0 ? height : 1);
	// Same aspect ratio as the display resolution
	_renderResolution = (_config.internalVerticalResolution/_config.screenResolution[1]) * _config.screenResolution;
}

void Renderer::defaultGLSetup(){
	// Default GL setup.
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}



