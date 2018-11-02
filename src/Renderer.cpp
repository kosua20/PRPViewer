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
#include <cctype>

bool findSubstringInsensitive(const std::string & strHaystack, const std::string & strNeedle)
{
	auto it = std::search(
						  strHaystack.begin(), strHaystack.end(),
						  strNeedle.begin(),   strNeedle.end(),
						  [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
						  );
	return (it != strHaystack.end() );
}

Renderer::~Renderer(){}

Renderer::Renderer(Config & config) : _config(config) {
	
	// Initial render resolution.
	_renderResolution = _config.screenResolution;
	
	defaultGLSetup();
	
	_quad.init("passthrough");
	_fxaaquad.init("fxaa");
	// Setup camera parameters.
	_cameraFarPlane = 8000.0f;
	_cameraFOV = 1.3f;
	_camera.projection(config.screenResolution[0]/config.screenResolution[1], _cameraFOV, 0.1f, _cameraFarPlane);
	
	_sceneFramebuffer = std::make_shared<Framebuffer>(_renderResolution[0], _renderResolution[1], GL_RGB, GL_UNSIGNED_BYTE, GL_RGB8, GL_LINEAR, GL_CLAMP_TO_EDGE, true);
	
	Resources::manager().getProgram("object_basic")->registerTexture("textures", 0);
	Resources::manager().getProgram("object_basic")->registerTexture("cubemaps", 1);
	Resources::manager().getProgram("object_special")->registerTexture("textures", 0);
	Resources::manager().getProgram("object_special")->registerTexture("cubemaps", 1);
	Resources::manager().getProgram("object_special")->registerTexture("textures1", 2);
	
	std::vector<std::string> files = ImGui::listFiles("./", false, false, {"age"});
	
	_age = std::make_shared<Age>();
	_resolutionScaling = 100.0f;
	//loadAge("../../../data/uru/spyroom.age");
	_clearColor[0] = _clearColor[1] = _clearColor[2] = 0.5f;
	_vertexOnly = false;
	_forceNoLighting = false;
	_forceLighting = false;
	_showDot = true;
}



void Renderer::draw(){
	
	// Infos window.
	ImGui::SetNextWindowPos(ImVec2(0,290), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(270,410), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Infos")) {
		ImGui::Text("%2.1f FPS (%2.1f ms)", ImGui::GetIO().Framerate, ImGui::GetIO().DeltaTime*1000.0f);
		ImGui::Text("Age: %s", (_age->getName().c_str()));
		
		if(_displayMode == OneObject && _objectId < _age->objects().size()) {
			
			const auto & selectedObj = _age->objects()[_objectId];
			ImGui::Text("Object: %s, %lu parts", selectedObj->getName().c_str(), selectedObj->subObjects().size());
			if(_subObjectId>-1){
				ImGui::Text("Part: %d, %lu layers", _subObjectId, selectedObj->subObjects()[_subObjectId]->material->getLayers().size());
			}
			
			const ImGuiTreeNodeFlags parentFlags = 0 ;
			
			for(int soid = 0; soid < selectedObj->subObjects().size(); ++soid){
				const auto & subobj = selectedObj->subObjects()[soid];
				const std::string subObjName = "Subobject " + std::to_string(soid);
				if(ImGui::TreeNodeEx(subObjName.c_str(), parentFlags, "%s", subObjName.c_str())){
					for(const auto & layer : subobj->material->getLayers()){
						if(ImGui::TreeNodeEx(layer->getName().c_str(), parentFlags, "%s", layer->getName().c_str())){
							plLayerInterface * lay = plLayerInterface::Convert(layer->getObj());
							const std::string logString = logLayer(lay);
							ImGui::TextWrapped("%s", logString.c_str());
							ImGui::TreePop();
						}
					}
					ImGui::TreePop();
				}
			}
		} else if(_displayMode == OneTexture && _textureId < _age->textures().size()){
			const std::string & textureName = _age->textures()[_textureId];
			ImGui::Text("Texture: %s", textureName.c_str());
			const auto & texInfos = Resources::manager().getTexture(textureName);
			ImGui::Text("(%d x %d), %s %d mips", texInfos.width, texInfos.height, (texInfos.cubemap ? "Cube" : "2D"), texInfos.mipmap);
		} else {
			ImGui::Text("Draws: %i/%lu objects", _drawCount, _age->objects().size());
		}

	}
	ImGui::End();
	
	
	#define DEFAULT_WIDTH 160
	
	ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(270,290), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Settings")) {
		static int current_item_id = 0;
		
		if(ImGui::Button("Load .age file...")){
			ImGui::OpenFilePicker("Load Age");
		}
		std::string selectedFile;
		if(ImGui::BeginFilePicker("Load Age", "Load a .age file.", "../../../data/", selectedFile, false, false, {"age"})){
			loadAge(selectedFile);
			current_item_id = 0;
			
		}
		
		auto linkingNameProvider = [](void* data, int idx, const char** out_text) {
			const std::vector<std::string>* arr = (std::vector<std::string>*)data;
			*out_text = (*arr)[idx].c_str();
			return true;
		};
		ImGui::PushItemWidth(DEFAULT_WIDTH);
		if (ImGui::Combo("Linking point", &current_item_id, linkingNameProvider, (void*)&(_age->linkingNames()), _age->linkingNames().size())) {
			if (current_item_id >= 0) {
				_camera.setCenter(_age->linkingPoints().at(_age->linkingNames()[current_item_id]));
			}
		}
		ImGui::PopItemWidth();
		
		ImGui::Checkbox("Wireframe", &_wireframe);
		ImGui::SameLine();
		if(ImGui::Checkbox("Vertex colors", &_vertexOnly)){
			const auto prog = Resources::manager().getProgram("object_basic");
			glUseProgram(prog->id());
			glUniform1i(prog->uniform("forceVertexColor"), _vertexOnly);
			glUseProgram(0);
			const auto prog1 = Resources::manager().getProgram("object_special");
			glUseProgram(prog1->id());
			glUniform1i(prog1->uniform("forceVertexColor"), _vertexOnly);
			glUseProgram(0);
		}
		
		if(ImGui::Checkbox("Default light", &_forceLighting)){
			const auto prog = Resources::manager().getProgram("object_basic");
			glUseProgram(prog->id());
			glUniform1i(prog->uniform("forceLighting"), _forceLighting);
			glUseProgram(0);
			const auto prog1 = Resources::manager().getProgram("object_special");
			glUseProgram(prog1->id());
			glUniform1i(prog1->uniform("forceLighting"), _forceLighting);
			glUseProgram(0);
		}
		ImGui::SameLine();
		if(ImGui::Checkbox("No lights", &_forceNoLighting)){
			const auto prog = Resources::manager().getProgram("object_basic");
			glUseProgram(prog->id());
			glUniform1i(prog->uniform("forceNoLighting"), _forceNoLighting);
			glUseProgram(0);
			const auto prog1 = Resources::manager().getProgram("object_special");
			glUseProgram(prog1->id());
			glUniform1i(prog1->uniform("forceNoLighting"), _forceNoLighting);
			glUseProgram(0);
		}
		
		
		ImGui::Checkbox("Culling", &_doCulling); ImGui::SameLine();
		ImGui::PushItemWidth(90.0f);
		ImGui::SliderFloat("Dist.", &_cullingDistance, 10.0f, 3000.0f);
		ImGui::PopItemWidth();
		// Camera.
		ImGui::PushItemWidth(DEFAULT_WIDTH);
		ImGui::SliderFloat("Camera speed", &_camera.speed(), 0.0f, 500.0f);
		if(ImGui::SliderFloat("Resolution %", &_resolutionScaling, 10.0, 200.0)){
			_resolutionScaling = std::min(std::max(_resolutionScaling, 10.0f), 200.0f);
			updateResolution(_config.screenResolution[0], _config.screenResolution[1]);
		}
		if(ImGui::SliderFloat("Far plane", &_cameraFarPlane, 0.1f, 15000.0f, "%.3f", 2.0f)){
			_camera.frustum(0.1f, _cameraFarPlane);
		}
		if(ImGui::SliderFloat("Field of view", &_cameraFOV, 0.1f, 3.0f)){
			_camera.fov(_cameraFOV);
		}
		ImGui::PopItemWidth();
		ImGui::ColorEdit3("Background", &_clearColor[0]);
		ImGui::Checkbox("Show cam. center", &_showDot);
	}
	ImGui::End();
	
	// Display the log window.
	
	
	ImGui::SetNextWindowPos(ImVec2(0,680), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(869,180), ImGuiCond_FirstUseEver);
	Log::Info().display();
	
	
	ImGui::SetNextWindowPos(ImVec2(870,0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(330,852), ImGuiCond_FirstUseEver);
	if(ImGui::Begin("Listing")){
		
		ImGui::Combo("Mode", (int*)(&_displayMode), "Scene\0Object\0Texture\0\0");
		
		if(_displayMode == Scene){
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
		}
		
		if(ImGui::Button("List all content")){
			Log::Info() << "Objects: " << std::endl;
			int pii = 0;
			for(const auto & object : _age->objects()){
				Log::Info() << object->getName() << ", ";
				++pii;
				if(pii%5 == 0){
					Log::Info() << std::endl;
				}
			}
			Log::Info() << std::endl;
			
			Log::Info() << "Textures: " << std::endl;
			pii = 0;
			for(const auto & texture : _age->textures()){
				Log::Info() << texture << ", ";
				++pii;
				if(pii%5 == 0){
					Log::Info() << std::endl;
				}
			}
			Log::Info() << std::endl;
		}
		
		// Search in current list (objects or textures).
		static char * searchString = new char[512];
		ImGui::InputText("Query", searchString, 512);
		if(ImGui::Button("Search")){
			const std::string searchQueryString(searchString);
			std::vector<std::pair<int, std::string>> results;
			
			if(_displayMode == Scene || _displayMode == OneObject){
				for(size_t oid = 0; oid < _age->objects().size(); ++oid){
					const auto & obj = _age->objects()[oid];
					if(findSubstringInsensitive(obj->getName(), searchQueryString)){
						results.emplace_back(oid, obj->getName());
					}
				}
			} else if(_displayMode == OneTexture){
				for(size_t oid = 0; oid < _age->textures().size(); ++oid){
					const auto & texName = _age->textures()[oid];
					if(findSubstringInsensitive(texName, searchQueryString)){
						results.emplace_back(oid, texName);
					}
				}
			}
			
			Log::Info() << "Found " << results.size() << " results" << (results.empty() ? "." : ":") << std::endl;
			for(const auto & result : results){
				Log::Info() << result.second << " (" << result.first << ")" << std::endl;
			}
		}
		
		ImGui::BeginChild("List", ImVec2(0,-80), true);
		// If scene or object, display the list of objects.
		if(_displayMode == Scene){
			for(size_t oid = 0; oid < _age->objects().size(); ++oid){
				auto & object = _age->objects()[oid];
				ImGui::Selectable(object->getName().c_str(), &object->enabled);
			}
		} else if (_displayMode == OneObject){
			for(size_t oid = 0; oid < _age->objects().size(); ++oid){
				auto & object = _age->objects()[oid];
				if(ImGui::Selectable(object->getName().c_str(), oid == _objectId)){
					_objectId = oid;
					//Reset the enabled sub object, etc.
					_subObjectId = _subLayerId = -1;
				}
			}
		} else if(_displayMode == OneTexture){
			for(size_t oid = 0; oid < _age->textures().size(); ++oid){
				auto & textureName = _age->textures()[oid];
				if(ImGui::Selectable(textureName.c_str(), oid == _textureId)){
					_textureId = oid;
				}
			}
		}
		ImGui::EndChild();
		
		// Also display basic ID selectors.
		if(_displayMode == OneObject){
			if(ImGui::InputInt("Object ID", &_objectId)){
				_objectId = std::min(std::max(_objectId,0), (int)_age->objects().size()-1);
				_subObjectId = -1;
				_subLayerId = -1;
			}
			
			if(ImGui::InputInt("Part ID", &_subObjectId)){
				if(_objectId < 0){
					_objectId = 0;
				}
				_subObjectId = std::min(std::max(_subObjectId, -1), (int)_age->objects()[_objectId]->subObjects().size()-1);
				_subLayerId = -1;
			}
			
			if(ImGui::InputInt("Layer ID", &_subLayerId)){
				if(_objectId < 0){
					_objectId = 0;
				}
				if(_subObjectId < 0){
					_subObjectId = 0;
				}
				_subLayerId = std::min(std::max(_subLayerId,-1), (int)_age->objects()[_objectId]->subObjects()[_subObjectId]->material->getLayers().size()-1);
			}
		} else if(_displayMode == OneTexture){
			if(ImGui::InputInt("Texture ID", &_textureId)){
				_textureId = std::min(std::max(_textureId,0), (int)_age->textures().size()-1);
			}
		}
		
	}
	ImGui::End();

	glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], 1.0f);
	
	// Display the texture fullscreen.
	// TODO: handle aspect ratio.
	if( _displayMode == OneTexture){
		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0,0, _config.screenResolution[0], _config.screenResolution[1]);
		if(_textureId < _age->textures().size()){
			const auto & texInfos = Resources::manager().getTexture(_age->textures()[_textureId]);
			//const glm::vec2 resolutionTexture(texInfos.width, texInfos.height);
			_quad.draw(texInfos.id);
		}
		return;
	}
	
	
	_sceneFramebuffer->bind();
	
	glViewport(0, 0, GLsizei(_renderResolution[0]), GLsizei(_renderResolution[1]));
	
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	checkGLError();
	if(_displayMode == OneObject){
		if(_objectId < _age->objects().size()){
			const auto objectToShow = _age->objects()[_objectId];
			if(_wireframe){
				objectToShow->drawDebug(_camera.view() , _camera.projection(), _subObjectId);
			} else {
				objectToShow->draw(_camera.view() , _camera.projection(), _subObjectId, _subLayerId);
			}
		}
	} else {
	
		const glm::mat4 viewproj = _camera.projection() * _camera.view();
		_drawCount = 0;
		auto objects = _age->objectsClone();
		auto& camera = _camera;
		
		std::sort(objects.begin(), objects.end(), [&camera](const std::shared_ptr<Object> & leftObj, const std::shared_ptr<Object> & rightObj){
			// We cheat for one object only, the sky, drawn first in all cases.
			
			if(leftObj->probablySky() && !rightObj->probablySky()){
				return true;
			}
			if(rightObj->probablySky() && !leftObj->probablySky()){
				return false;
			}
			// Billboard after non billboard, whatever the transparency state.
			if(leftObj->billboard() && !rightObj->billboard()){
				return false;
			}
			if(!leftObj->billboard() && rightObj->billboard()){
				return true;
			}
			// Transparent is necessarily later than non transparent.
			if(leftObj->transparent() && !rightObj->transparent()){
				return false;
			}
			if(!leftObj->transparent() && rightObj->transparent()){
				return true;
			}
			// All "crossed cases" have been handled.
			// Compute both distances to camera.
			const float leftDist = glm::length2(leftObj->getCenter() - camera.getPosition());
			const float rightDist = glm::length2(rightObj->getCenter() - camera.getPosition());
			
			// Either they are both billboard, both transparent, or both opaque.
			if(!leftObj->transparent()){
				// Both are non transparent, render the closest first.
				// We don't really care.
				return leftDist < rightDist;
			}
			// Else if one is contained in the other, render it first.
			if(leftObj->contains(rightObj)){
				return false;
			} else if(rightObj->contains(leftObj)){
				return true;
			}
			// Else render the furthest first.
			return leftDist > rightDist;

		});
		
		// Rendering order:
		// skybox first.
		// opaque objects from closest to furthest.
		// transparent objects (subojects?) from furthest to closest (maybe at least use the bounding box of the transparent subojects.
		// billboards are after the transparent objects.
		
		for(const auto & object : objects){
			if(!object->enabled){
				continue;
			}
			// Cull based on distance and bounding box vs camera frustum.
			if(_doCulling && !object->probablySky() &&
			   (glm::length2(object->getCenter() - _camera.getPosition()) > _cullingDistance*_cullingDistance ||
				 !object->isVisible(_camera.getPosition(), viewproj)
				)){
				continue;
			}
			if(_wireframe){
				object->drawDebug(_camera.view() , _camera.projection());
			} else {
				object->draw(_camera.view() , _camera.projection());
			}
			++_drawCount;
		}
	
	}

	
	// Render the camera cursor.
	glDisable(GL_BLEND);
	if(_showDot){
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		const float scale = glm::length(_camera.getDirection());
		const glm::mat4 MVP = _camera.projection() * _camera.view() * glm::scale(glm::translate(glm::mat4(1.0f), _camera.getCenter()), glm::vec3(0.015f*scale));
		const auto debugProgram = Resources::manager().getProgram("camera-center");
		const auto debugObject = Resources::manager().getMesh("sphere");
		
		glUseProgram(debugProgram->id());
		glBindVertexArray(debugObject.vId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugObject.eId);
		glUniformMatrix4fv(debugProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		glUniform2f(debugProgram->uniform("screenSize"), _config.screenResolution[0], _config.screenResolution[1]);
		glDrawElements(GL_TRIANGLES, debugObject.count, GL_UNSIGNED_INT, (void*)0);
	}
	// Reset state.
	glBindVertexArray(0);
	glUseProgram(0);
	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	
	_sceneFramebuffer->unbind();
	
	glViewport(0,0, _config.screenResolution[0], _config.screenResolution[1]);
	(_wireframe ? _quad : _fxaaquad).draw(_sceneFramebuffer->textureId(), 1.0f/_config.screenResolution);
	
	checkGLError();
}

void Renderer::loadAge(const std::string & path){
	Log::Info() << "Loading " << path << "..." << std::endl;
	Resources::manager().reset();
	_displayMode = Scene;
	_objectId = 0;
	_textureId = 0;
	_subObjectId = -1;
	_subLayerId = -1;
	_age.reset(new Age(path));
	// A Uru human is around 4/5 units in height apparently.
	_camera.setCenter(_age->getDefaultLinkingPoint());
}
void Renderer::update(){
	if(Input::manager().resized()){
		resize((int)Input::manager().size()[0], (int)Input::manager().size()[1]);
	}
	
	if(_displayMode != OneTexture){
		_camera.update();
	}
}

void Renderer::physics(double fullTime, double frameTime){
	if(_displayMode != OneTexture){
		_camera.physics(frameTime);
	}
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
	_renderResolution = (_resolutionScaling/100.0f ) * _config.screenResolution;
	_sceneFramebuffer->resize(_renderResolution[0], _renderResolution[1]);
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



