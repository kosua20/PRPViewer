#include "Input.hpp"
#include "../helpers/Logger.hpp"

/// Singleton.
Input& Input::manager(){
	static Input* input = new Input();
	return *input;
}

Input::Input(){
	// Check if any joystick is available.
	/*for(int id = GLFW_JOYSTICK_1; id <= GLFW_JOYSTICK_LAST; ++id){
		// We only register the first joystick encountered if it exists.
		if(glfwJoystickPresent(id) == GL_TRUE){
			_joysticks[id].activate(id);
			break;
		}
	}*/
	
}

Input::~Input(){ }

void Input::keyPressedEvent(int key, int action){
	if(key == GLFW_KEY_UNKNOWN){
		return;
	}
	
	if(action == GLFW_PRESS){
		_keys[key].pressed = true;
		_keys[key].first = true;
		_keys[key].last = false;
	} else if (action == GLFW_RELEASE){
		_keys[key].pressed = false;
		_keys[key].first = false;
		_keys[key].last = true;
	}
	Log::Info() << Log::Verbose << Log::Input << "Key " << key << ", " << (action == GLFW_PRESS ? "pressed" : (action == GLFW_RELEASE ? "released" : "held")) << "." << std::endl;
}

void Input::joystickEvent(int joy, int event){
	
	if (event == GLFW_CONNECTED) {
		
		Log::Info() << Log::Verbose << Log::Input << "Joystick: connected joystick " << joy << "." << std::endl;

		// Register the new one, if no joystick was activated consider this one as the active one.
		bool res = _joysticks[joy].activate(joy);
		if(res && _activeJoystick == -1){
			_activeJoystick = joy;
		}
		
	} else if (event == GLFW_DISCONNECTED) {

		Log::Info() << Log::Verbose << Log::Input << "Joystick: disconnected joystick " << joy << "." << std::endl;

		// If the disconnected joystick is the one currently used, register this.
		_joysticks[joy].deactivate();
		if(joy == _activeJoystick){
			_activeJoystick = -1;
		}
		// Here we could also try to fall back on any other (still) connected joystick.
	}
	
}

void Input::mousePressedEvent(int button, int action){
	if(action == GLFW_PRESS){
		_mouseButtons[button].pressed = true;
		_mouseButtons[button].first = true;
		_mouseButtons[button].last = false;
		_mouseButtons[button].x0 = _mouse.x;
		_mouseButtons[button].y0 = _mouse.y;
		// Delta = 0 at the beginning.
		_mouseButtons[button].x1 = _mouse.x;
		_mouseButtons[button].y1 = _mouse.y;
		
	} else if (action == GLFW_RELEASE) {
		_mouseButtons[button].pressed = false;
		_mouseButtons[button].first = false;
		_mouseButtons[button].last = true;
		_mouseButtons[button].x1 = _mouse.x;
		_mouseButtons[button].y1 = _mouse.y;
	}
	Log::Info() << Log::Verbose << Log::Input << "Mouse pressed: " << button << ", " << action << " at " << _mouse.x << "," << _mouse.y << "." << std::endl;
}

void Input::mouseMovedEvent(double x, double y){

	_mouse.x = x/_width;
	_mouse.y = y/_height;
	Log::Info() << Log::Verbose << Log::Input << "Mouse moved: " << x << "," << y << "." << std::endl;

}

void Input::mouseScrolledEvent(double xoffset, double yoffset){
	_mouse.scroll = glm::vec2(xoffset, yoffset);
	Log::Info() << Log::Verbose << Log::Input << "Mouse scrolled: " << xoffset << "," << yoffset << "." << std::endl;

}

void Input::resizeEvent(int width, int height){
	_width = width > 0 ? width : 1;
	_height = height > 0 ? height : 1;
	_resized = true;
	Log::Info() << Log::Verbose << Log::Input << "Resize event: " << width << "," << height << "." << std::endl;

}

void Input::update(){
	// Reset temporary state (first, last).
	for(unsigned int i = 0; i < GLFW_KEY_LAST+1; ++i){
		_keys[i].first = false;
		_keys[i].last = false;
	}
	for(unsigned int i = 0; i < GLFW_MOUSE_BUTTON_LAST+1; ++i){
		_mouseButtons[i].first = false;
		_mouseButtons[i].last = false;
	}
	_mouse.scroll = glm::vec2(0.0f,0.0f);
	_resized = false;
	// Update only the active joystick if it exists.
	if(_activeJoystick >= 0){
		_joysticks[_activeJoystick].update();
	}
	glfwPollEvents();
	
}


bool Input::pressed(const Key & keyboardKey) const {
	return _keys[keyboardKey].pressed;
}

bool Input::triggered(const Key & keyboardKey, bool absorb) {
	bool res = _keys[keyboardKey].first;
	if(absorb){
		_keys[keyboardKey].first = false;
	}
	return res;
}

bool Input::pressed(const Mouse & mouseButton) const {
	return _mouseButtons[mouseButton].pressed;
}

bool Input::triggered(const Mouse & mouseButton, bool absorb) {
	bool res = _mouseButtons[mouseButton].first;
	if(absorb){
		_mouseButtons[mouseButton].first = false;
	}
	return res;
}

glm::vec2 Input::mouse() const {
	return glm::vec2(_mouse.x, _mouse.y);
}

glm::vec2 Input::moved(const Mouse & mouseButton) const {
	const MouseButton & b = _mouseButtons[mouseButton];
	if(b.pressed){
		return glm::vec2(_mouse.x - b.x0, _mouse.y - b.y0);
	}
	return glm::vec2(0.0f,0.0f);
}

glm::vec2 Input::scroll() const {
	return _mouse.scroll;
}

