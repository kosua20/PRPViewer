#ifndef InterfaceUtilities_h
#define InterfaceUtilities_h

#include <imgui/imgui.h>
#include <string>
#include <vector>

namespace ImGui {
	
	std::vector<std::string> listFiles(const std::string & path, const bool listHidden, const bool includeSubdirectories, const std::vector<std::string> & allowedExtensions);
	
	void OpenFilePicker(const std::string & name);

	bool BeginFilePicker(const std::string & name, const std::string & helpMessage,
		const std::string & directoryPath, std::string & selectedFile,
		const bool saveMode, const bool allowDirectories = false,
		const std::vector<std::string>& extensionsAllowed = {});

}
#endif
