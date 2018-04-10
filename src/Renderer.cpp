#include "Renderer.hpp"
#include "Object.hpp"
#include "input/Input.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "helpers/Logger.hpp"
#include <stdio.h>
#include <vector>


Renderer::~Renderer(){}

Renderer::Renderer(Config & config) : _config(config) {
	
	// Initial render resolution.
	_renderResolution = (_config.internalVerticalResolution/_config.screenResolution[1]) * _config.screenResolution;
	
	defaultGLSetup();
	// Setup camera parameters.
	_camera.projection(config.screenResolution[0]/config.screenResolution[1], 1.3f, 0.1f, 8000.0f);
	loadAge("../../../data/spyroom.age");
}

void Renderer::draw(){
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glClearColor(0.45f,0.45f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	if (ImGui::Begin("Options")) {
		static int current_item_id = 0;
		
		if(ImGui::Button("Load .age file...")){
			ImGui::OpenFilePicker("Load Age");
		}
		std::string selectedFile;
		if(ImGui::BeginFilePicker("Load Age", "Load a .age file.", "../../../data/", selectedFile, false, false, {"age"})){
			loadAge(selectedFile);
			current_item_id = 0;
		}
		const std::string nameStr = "Current: " + (_age ? _age->getName() : "None");
		ImGui::Text("%s", nameStr.c_str());
		
		
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
		
	}
	ImGui::End();
	
	glEnable(GL_DEPTH_TEST);
	for(const auto & object : _age->objects()){
		object.draw(_camera.view() , _camera.projection());
	}
	glDisable(GL_DEPTH_TEST);
	
}

void Renderer::loadAge(const std::string & path){
	Log::Info() << "Should load " << path << std::endl;
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
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}



