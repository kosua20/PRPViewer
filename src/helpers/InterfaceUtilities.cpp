#include "InterfaceUtilities.hpp"
#include <iostream>
#include <boost/filesystem.hpp>

namespace ImGui {

	bool directoryExists(const std::string& path) {
		boost::filesystem::path p(path);
		return boost::filesystem::exists(p) && boost::filesystem::is_directory(path);
	}

	bool fileExists(const std::string& path){
		boost::filesystem::path p(path);
		return boost::filesystem::exists(p) && boost::filesystem::is_regular_file(path);
	}

	void makeDirectory(const std::string& path){
		boost::filesystem::path p(path);
		if (boost::filesystem::exists(p) == false) {
			boost::filesystem::create_directories(p);
		}	
	}

	std::vector<std::string> listFiles(const std::string & path, const bool listHidden, const bool includeSubdirectories, const std::vector<std::string> & allowedExtensions){
		if (!directoryExists(path)) {
			return {};
		}

		std::vector<std::string> files;
		bool shouldCheckExtension = !allowedExtensions.empty();

		try {
			boost::filesystem::directory_iterator end_iter;
			for (boost::filesystem::directory_iterator dir_itr(path); dir_itr != end_iter; ++dir_itr) {

				const std::string itemName = dir_itr->path().filename().string();
				if (includeSubdirectories && boost::filesystem::is_directory(dir_itr->status())) {
					if (listHidden || (itemName.size() > 0 && itemName.at(0) != '.')) {
						files.push_back(itemName);
					}
				} else if (boost::filesystem::is_regular_file(dir_itr->status())) {
					bool shouldKeep = !shouldCheckExtension;
					if (shouldCheckExtension) {
						for (const auto & allowedExtension : allowedExtensions) {
							if (dir_itr->path().extension() == ("." + allowedExtension) || dir_itr->path().extension() == allowedExtension) {
								shouldKeep = true;
								break;
							}
						}
					}

					if (shouldKeep && (listHidden || (itemName.size() > 0 && itemName.at(0) != '.'))) {
						files.push_back(itemName);
					}
				}
			}
		} catch (const boost::filesystem::filesystem_error& ex) {
			std::cout << "Can't access or find directory." << std::endl;
		}

		std::sort(files.begin(), files.end());

		return files;
	}

	std::vector<std::string> listSubdirectories(const std::string & path, const bool listHidden){
		if (!directoryExists(path)) {
			return {};
		}

		std::vector<std::string> dirs;


		try {
			boost::filesystem::directory_iterator end_iter;
			for (boost::filesystem::directory_iterator dir_itr(path); dir_itr != end_iter; ++dir_itr) {

				const std::string itemName = dir_itr->path().filename().string();
				if (boost::filesystem::is_directory(dir_itr->status())) {
					if (listHidden || (itemName.size() > 0 && itemName.at(0) != '.')) {
						dirs.push_back(itemName);
					}
				}
			}
		} catch (const boost::filesystem::filesystem_error& ex) {
			std::cout << "Can't access or find directory." << std::endl;
		}

		std::sort(dirs.begin(), dirs.end());

		return dirs;
	}
	// Adopt an approach similar to the one used internally by ImGui:
	// We have a shared static state used at every frame.
	// The two-calls structure similar to the one used for ImGui::Popup
	// allows us to use a file picker via a button, a menu, or any
	// interactive one-shot item. Call ImGui::OpenFilePicker once w/ the
	// file picker unique ID. (for instance in a if(ImGui::Button(...))
	// scope; then call if(ImGui::BeginFilePicker(...)) which returns
	// true if a file/directory has been selected.

	struct FilePickerInternal {
		boost::filesystem::path currentDir;
		std::vector<std::string> items = {};
		std::vector<char*> rawItems = {};
		std::vector<std::string> dirs = {};
		std::vector<char*> rawDirs = {};
		std::vector<std::string> volumes = {};
		int currentFileId = -1;
		int currentSubdirId = -1;
		bool showHidden = false;
		char newName[128] = "New file name";
		char newDir[128] = "New directory name";
		char newPath[256] = "./";
		std::string exts = "";
		std::string currentResult = "";
		bool initPopup = false;
	};
	static FilePickerInternal filePickerInternals;

	void OpenFilePicker(const std::string & name) {
		filePickerInternals.initPopup = true;
	}

	bool BeginFilePicker(const std::string & name, const std::string & helpMessage,
		const std::string & directoryPath, std::string & selectedFile,
		const bool saveMode, const bool allowDirectories,
		const std::vector<std::string>& extensionsAllowed)
	{

		// Per-frame state.
		bool shouldUpdate = false;

		if (!directoryExists(directoryPath)) {
			std::cout << "Directory doesn't exist." << std::endl;
			return false;
		}

		// Button to open the file picker window.
		ImGui::PushID(name.c_str());

		// First frame of the popup being displayed.
		// Reset informations and internal state.
		if (filePickerInternals.initPopup) {
			filePickerInternals.initPopup = false;
			shouldUpdate = true;
			filePickerInternals.currentDir = boost::filesystem::canonical(directoryPath);
#ifdef _WIN32
			strcpy_s(filePickerInternals.newName, 128, "New file name");
#else
			strcpy(filePickerInternals.newName, "New file name");
#endif
			// Update allowed extensions string for display.
			if (!extensionsAllowed.empty()) {
				filePickerInternals.exts = "(allowed: ";
				for (const auto & ext : extensionsAllowed) {
					filePickerInternals.exts += ("." + ext + " ");
				}
				filePickerInternals.exts += ")";
			} else {
				filePickerInternals.exts = "";
			}
			filePickerInternals.currentResult = "";
			// Check if any windows volume exists.
			filePickerInternals.volumes.clear();
			char vol[5] = "a://";
			for (char l = 'a'; l <= 'z'; ++l) {
				vol[0] = l;
				const std::string volPath(vol);
				if (directoryExists(volPath)) {
					filePickerInternals.volumes.push_back(volPath);
				}
			}
			// Set popup flag to "open".
			ImGui::OpenPopup(name.c_str());
		} else if (ImGui::BeginPopupModal(name.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{

			const float scaling = ImGui::GetIO().FontGlobalScale;
			// Message and allowed extensions if any.
			ImGui::Text("%s", helpMessage.c_str());
			if (!filePickerInternals.exts.empty()) {
				ImGui::SameLine();
				ImGui::Text("%s", filePickerInternals.exts.c_str());
			}

			// Current path.
			ImGui::Separator();
			ImGui::Text("Current:");
			ImGui::SameLine();
			const std::string curr = filePickerInternals.currentDir.string();

			ImGui::TextWrapped("%s", curr.c_str());

			// Parent button.
			if (ImGui::Button("Parent")) {
				// If not root, go upward in the directory tree and update lists.
				if (filePickerInternals.currentDir != filePickerInternals.currentDir.root_path()) {
					filePickerInternals.currentDir = filePickerInternals.currentDir.parent_path();
					shouldUpdate = true;
				}
			}
			ImGui::SameLine();
			// Go to button.
			if (ImGui::Button("Go to...")) {
#ifdef _WIN32
				strncpy_s(filePickerInternals.newPath, 256, filePickerInternals.currentDir.string().c_str(), 255);
#else
				strncpy(filePickerInternals.newPath, filePickerInternals.currentDir.string().c_str(), 255);
#endif
				filePickerInternals.newPath[255] = '\0';
				ImGui::OpenPopup("Go to##Popup");
			}
			// Go to modal popup.
			if (ImGui::BeginPopupModal("Go to##Popup")) {
				// Require absolute path in text fueld.
				ImGui::Text("Absolute path:");
				ImGui::PushItemWidth(scaling * 400);
				ImGui::InputText("##Goto path field", filePickerInternals.newPath, 256);
				ImGui::PopItemWidth();
				// If Ok, check the directory exists, clean it and update current path and state.
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					std::string newPathString = std::string(filePickerInternals.newPath);
					if (newPathString[newPathString.size() - 1] == ':') {
						newPathString += "/";
					}
					if (directoryExists(newPathString)) {
						filePickerInternals.currentDir = boost::filesystem::canonical(boost::filesystem::path(newPathString));
						if (filePickerInternals.currentDir != filePickerInternals.currentDir.root_path()) {
							filePickerInternals.currentDir.remove_trailing_separator();
						}
						shouldUpdate = true;
						ImGui::CloseCurrentPopup();

					}
					// If the directory doesn't exist, don't close.
				}
				ImGui::SameLine();
				// Cancel by closing.
				if (ImGui::Button("Cancel")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();

			// New directory button.
			if (ImGui::Button("New directory...")) {
#ifdef _WIN32
				strcpy_s(filePickerInternals.newDir, 128, "New directory name");
#else
				strcpy(filePickerInternals.newDir, "New directory name");
#endif
				ImGui::OpenPopup("Create directory##Popup");
			}
			// New directory popup.
			if (ImGui::BeginPopupModal("Create directory##Popup")) {
				// Ask for name in text field.
				ImGui::Text("Name:");
				ImGui::SameLine();
				ImGui::PushItemWidth(scaling * 200);
				ImGui::InputText("##New dir field", filePickerInternals.newDir, 128);
				ImGui::PopItemWidth();
				// If ok, create directory using utility, update lists.
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					const boost::filesystem::path tempPath = filePickerInternals.currentDir.append(filePickerInternals.newDir);
					const std::string newDirPath = tempPath.string();
					makeDirectory(newDirPath);
					shouldUpdate = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				// If cancel, close popup.
				if (ImGui::Button("Cancel")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();

			}
			ImGui::Separator();

			// Lists.
			// Subdirectories list.
			ImGui::PushItemWidth(scaling * 200);
			const bool clickedDir = ImGui::ListBox("##SelectFileBoxSubdirectories", &filePickerInternals.currentSubdirId, filePickerInternals.rawDirs.empty() ? NULL : &filePickerInternals.rawDirs[0], filePickerInternals.rawDirs.size(), 15);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			// Files list.
			ImGui::PushItemWidth(scaling * 300);
			const bool clickedFile = ImGui::ListBox("##SelectFileBoxFiles", &filePickerInternals.currentFileId, filePickerInternals.rawItems.empty() ? NULL : &filePickerInternals.rawItems[0], filePickerInternals.rawItems.size(), 15);
			ImGui::PopItemWidth();

			// If a directory has been selected in the list, we should update the filePickerInternals.currentDir and the lists.
			if (clickedDir && filePickerInternals.currentSubdirId >= 0) {
				shouldUpdate = true;
				// Detect volumes.
				const std::string newPath = std::string(filePickerInternals.rawDirs[filePickerInternals.currentSubdirId]);
				// Detect if the directory is pointing to a windows volume.
				if (newPath.size() == 4 && newPath[1] == ':' && newPath[2] == '/' && newPath[3] == '/') {
					// In this case overwrite the current path.
					filePickerInternals.currentDir = boost::filesystem::canonical(boost::filesystem::path(newPath));
				} else if (!newPath.empty()) {
					filePickerInternals.currentDir.append(newPath);
				}

			}

			// File creation.
			if (saveMode) {
				// If the user selected a file, copy its name in the text field to allow easy modifications.
				if (clickedFile && filePickerInternals.currentFileId >= 0) {
#ifdef _WIN32
					strncpy_s(filePickerInternals.newName, 128, filePickerInternals.rawItems[filePickerInternals.currentFileId], 127);
#else
					strncpy(filePickerInternals.newName, filePickerInternals.rawItems[filePickerInternals.currentFileId], 127);
#endif
					filePickerInternals.newName[127] = '\0';
				}
				// Ask for file name in text field.
				ImGui::Text("File name");
				ImGui::SameLine();
				ImGui::InputText("##Filename", filePickerInternals.newName, 128);
			}

			// Bottom row.
			// OK button.
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				// If we have a candidate file for output.
				if (saveMode || filePickerInternals.currentFileId >= 0) {
					// Build it path and store it.
					const std::string finalFileName = saveMode ? std::string(filePickerInternals.newName) : std::string(filePickerInternals.rawItems[filePickerInternals.currentFileId]);
					filePickerInternals.currentResult = filePickerInternals.currentDir.string() + "/" + finalFileName;
					// If the file already exists, warn the user.
					if (saveMode && (fileExists(filePickerInternals.currentResult) || directoryExists(filePickerInternals.currentResult))) {
						ImGui::OpenPopup("Warning!##Popup");
					} else {
						// Save, done.
						ImGui::CloseCurrentPopup();
						ImGui::EndPopup();
						ImGui::PopID();
						selectedFile = filePickerInternals.currentResult;
						return true;
					}
				}
			}
			// Warning popup.
			if (ImGui::BeginPopupModal("Warning!##Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("You will overwrite this file/directory.");
				// Overwrite button: save, done, close everything.
				if (ImGui::Button("Overwrite##Overwrite")) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					ImGui::PopID();
					selectedFile = filePickerInternals.currentResult;
					return true;
				}
				ImGui::SameLine();
				// Else close popup.
				if (ImGui::Button("Cancel##Overwrite")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			// Cancel button,  just close the popup.
			if (ImGui::Button("Cancel")) {
				selectedFile = "";
				ImGui::CloseCurrentPopup();
			}
			// Checkbox for hidden files.
			ImGui::SameLine();
			if (ImGui::Checkbox("Show hidden files", &filePickerInternals.showHidden)) {
				// Trigger an update of the lists.
				shouldUpdate = true;
			}

			ImGui::EndPopup();
		}

		ImGui::PopID();

		// Update subdirectories and files lists.
		if (shouldUpdate) {
			filePickerInternals.currentFileId = -1;
			filePickerInternals.currentSubdirId = -1;
			// Update files list.
			const std::string newDirPath = filePickerInternals.currentDir.string();
			filePickerInternals.items = listFiles(newDirPath, filePickerInternals.showHidden, allowDirectories, extensionsAllowed);
			filePickerInternals.rawItems.clear();
			for (size_t i = 0; i < filePickerInternals.items.size(); ++i) {
				filePickerInternals.rawItems.push_back(const_cast<char*>(filePickerInternals.items[i].c_str()));
			}
			// Update files list.
			filePickerInternals.dirs = listSubdirectories(newDirPath, filePickerInternals.showHidden);
			// If we are at the root of a disk hierarchy, append the windows drives.
			if (filePickerInternals.currentDir == filePickerInternals.currentDir.root_path()) {
				// filePickerInternals.dirs.push_back(""); // Additional spacing for legibility.
				for (auto & vol : filePickerInternals.volumes) {
					filePickerInternals.dirs.push_back(vol);
				}
			}

			filePickerInternals.rawDirs.clear();
			for (size_t i = 0; i < filePickerInternals.dirs.size(); ++i) {
				filePickerInternals.rawDirs.push_back(const_cast<char*>(filePickerInternals.dirs[i].c_str()));
			}
		}

		return false;
	}

}
