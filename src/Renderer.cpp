#include "Renderer.hpp"
#include "Object.hpp"
#include "input/Input.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "helpers/Logger.hpp"
#include <PRP/Surface/hsGMaterial.h>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
	for(int i = 0; i < 8; ++i){
		//Resources::manager().getProgram("object_basic")->registerTexture("textures["+std::to_string(i)+"]", i);
		//Resources::manager().getProgram("object_basic")->registerTexture("cubemaps["+std::to_string(i)+"]", 8+i);
	}
	Resources::manager().getProgram("object_basic")->registerTexture("textures", 0);
	Resources::manager().getProgram("object_basic")->registerTexture("cubemaps", 1);
	Resources::manager().getProgram("object_special")->registerTexture("textures", 0);
	Resources::manager().getProgram("object_special")->registerTexture("cubemaps", 1);
	Resources::manager().getProgram("object_special")->registerTexture("textures1", 2);
	
	
	std::vector<std::string> files = ImGui::listFiles("../../../data/mystv/", false, false, {"age"});
	
	loadAge("../../../data/uru/spyroom.age");
}



void Renderer::draw(){
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glClearColor(0.45f,0.45f, 0.5f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// FIXME: move to attributes.
	static int textureId = 0;
	static bool showTextures = false;
	static int objectId = 0;
	static int subObjectId = -1;
	static int layerId = -1;
	
	static bool showObject = false;
	static bool showList = false;
	static bool wireframe = true;
	static bool doCulling = true;
	static bool multipleSelection = false;
	static float cullingDistance = 1500.0f;
	static int drawCount = 0;
	
	static bool forceLighting = false;
	if (ImGui::Begin("Infos")) {
		
		ImGui::Text("%2.1f FPS (%2.1f ms)", ImGui::GetIO().Framerate, ImGui::GetIO().DeltaTime*1000.0f);
		ImGui::Text("Age: %s", (_age ? _age->getName().c_str() : "None"));
		ImGui::Text("Draws: %i/%lu", drawCount, _age->objects().size());
		if(showTextures){
			ImGui::Text("Current: %s", _age->textures()[textureId].c_str());
		}
		if(showObject){
			ImGui::Text("Current: %s", _age->objects()[objectId]->getName().c_str());
			ImGui::Text("%lu subobjects", _age->objects()[objectId]->subObjects().size());
			if(subObjectId>-1){
				ImGui::Text("%lu layers", _age->objects()[objectId]->subObjects()[subObjectId].material->getLayers().size());
			}
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
			objectId = 0;
			subObjectId = -1;
			layerId = -1;
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
		
		if(ImGui::Checkbox("Force lighting", &forceLighting)){
			const auto prog = Resources::manager().getProgram("object_basic");
			glUseProgram(prog->id());
			glUniform1i(prog->uniform("forceLighting"), forceLighting);
			glUseProgram(0);
			const auto prog1 = Resources::manager().getProgram("object_special");
			glUseProgram(prog1->id());
			glUniform1i(prog1->uniform("forceLighting"), forceLighting);
			glUseProgram(0);
		}
		ImGui::Checkbox("Wireframe", &wireframe);
		ImGui::SameLine();
		ImGui::Checkbox("Culling", &doCulling);
		ImGui::SliderFloat("Culling dist.", &cullingDistance, 10.0f, 3000.0f);
		ImGui::Checkbox("Show textures", &showTextures);
		
		if(showTextures){
			if(ImGui::InputInt("Texture ID", &textureId)){
				objectId = std::min(std::max(textureId,0), (int)_age->textures().size()-1);
			}
		}
		
	}
	ImGui::End();
	Log::Info().display();
	
	if(ImGui::Begin("Listing")){
		
		if(ImGui::Button("All")){
			for(auto & object : _age->objects()){
				object->enabled = true;
			}
		}
		ImGui::SameLine();
		if(ImGui::Button("None")){
			for(auto & object : _age->objects()){
				object->enabled = false;
			}
		}
		ImGui::SameLine();
		if(ImGui::Checkbox("Multiple", &multipleSelection)){
			if(!multipleSelection){
				// Disable all but one element.
				bool first = true;
				for(size_t oid = 0; oid < _age->objects().size(); ++oid){
					if(first && _age->objects()[oid]->enabled){
						first = false;
						continue;
					}
					_age->objects()[oid]->enabled = false;
				}
			}
		}
		
		ImGui::BeginChild("List", ImVec2(0,500), true);
		for(size_t oid = 0; oid < _age->objects().size(); ++oid){
			auto & object = _age->objects()[oid];
			
			if(ImGui::Selectable(object->getName().c_str(), &object->enabled)){
				if(!multipleSelection){
					// Disable all other elements.
					for(size_t doid = 0; doid < oid; ++doid){
						_age->objects()[doid]->enabled = false;
					}
					object->enabled = true;
					for(size_t doid = oid+1; doid < _age->objects().size(); ++doid){
						_age->objects()[doid]->enabled = false;
					}
				}
			}
		}
		ImGui::EndChild();
		
		ImGui::BeginChild("Details");
		if(showObject){
			const auto & obj = _age->objects()[objectId];
			const ImGuiTreeNodeFlags parentFlags = 0 ;
			const ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			
			for(int soid = 0; soid < obj->subObjects().size(); ++soid){
				const auto & subobj = obj->subObjects()[soid];
				if(ImGui::TreeNodeEx((void*)(intptr_t)soid, parentFlags, "Subobject %i", soid)){
					for(const auto & layer : subobj.material->getLayers()){
						if(ImGui::TreeNodeEx(layer->getName().c_str(), parentFlags, "%s", layer->getName().c_str())){
							
							plLayerInterface * lay = plLayerInterface::Convert(layer->getObj());
							const std::string logString = logLayer(lay);
							//ImGui::TreeNodeEx(logString.c_str(), leafFlags);
							ImGui::TextWrapped("%s", logString.c_str());
							
							ImGui::TreePop();
						}
						
					}
					ImGui::TreePop();
				}
			}
			if(ImGui::InputInt("Subobject ID", &subObjectId)){
				subObjectId = std::min(std::max(subObjectId,-1), (int)_age->objects()[objectId]->subObjects().size()-1);
			}
			if(ImGui::InputInt("Layer ID", &layerId)){
				layerId = std::min(std::max(layerId,-1), (int)_age->objects()[objectId]->subObjects()[subObjectId].material->getLayers().size()-1);
			}
			
		}
		ImGui::EndChild();
	}
	ImGui::End();
	
	bool firstObjectEnabled = true;
	showObject = false;
	for(size_t oid = 0; oid < _age->objects().size(); ++oid){
		if(_age->objects()[oid]->enabled){
			if(!firstObjectEnabled){
				showObject = false;
				break;
			}
			firstObjectEnabled = false;
			objectId = oid;
			showObject = true;
		}
	}
	
	
	
	//ImGui::ShowTestWindow();
	if(showTextures){
		_quad.draw(Resources::manager().getTexture(_age->textures()[textureId]).id);
		return;
	}
	
	glEnable(GL_DEPTH_TEST);
	checkGLError();
	if(showObject){
		const auto objectToShow = _age->objects()[objectId];
		
		if(wireframe){
			objectToShow->drawDebug(_camera.view() , _camera.projection(), subObjectId);
		} else {
			objectToShow->draw(_camera.view() , _camera.projection(), subObjectId, layerId);
		}
		
	} else {
		const glm::mat4 viewproj = _camera.projection() * _camera.view();
		drawCount = 0;
		auto objects = _age->objectsClone();
		/*auto& camera = _camera;
		std::sort(objects.begin(), objects.end(), [&camera](const std::shared_ptr<Object> & leftObj, const std::shared_ptr<Object> & rightObj){
			
			// Compute both distances to camera.
			const float leftDist = glm::length2(leftObj->getCenter() - camera.getPosition());
			const float rightDist = glm::length2(rightObj->getCenter() - camera.getPosition());
			// We want to first render objects further away from the camera.
			return leftDist > rightDist;
			
		});*/
		for(const auto & object : objects){
			if(!object->enabled){
				continue;
			}
			if(doCulling &&
			   (glm::length2(object->getCenter() - _camera.getPosition()) > cullingDistance*cullingDistance ||
				// !object->isVisible(_camera.getPosition(), _camera.getDirection()) || (
				 !object->isVisible(_camera.getPosition(), viewproj)
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
	
	const float scale = glm::length(_camera.getDirection());
	const glm::mat4 MVP = _camera.projection() * _camera.view() * glm::scale(glm::translate(glm::mat4(1.0f), _camera.getCenter()), glm::vec3(0.015f*scale));
	const auto debugProgram = Resources::manager().getProgram("camera-center");
	const auto debugObject = Resources::manager().getMesh("sphere");
	
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glUseProgram(debugProgram->id());
	glBindVertexArray(debugObject.vId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugObject.eId);
	
	glUniformMatrix4fv(debugProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
	glUniform2f(debugProgram->uniform("screenSize"), _config.screenResolution[0], _config.screenResolution[1]);
	glDrawElements(GL_TRIANGLES, debugObject.count, GL_UNSIGNED_INT, (void*)0);
	
	
	
	glBindVertexArray(0);
	glUseProgram(0);
	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	
	checkGLError();
}

void Renderer::loadAge(const std::string & path){
	Log::Info() << "Should load " << path << std::endl;
	Resources::manager().reset();
	_age.reset(new Age(path));
	// A Uru human is around 4/5 units in height apparently.
	_camera.setCenter(_age->getDefaultLinkingPoint());
	//_maxLayer = std::max(_maxLayer, _age->maxLayer());
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



