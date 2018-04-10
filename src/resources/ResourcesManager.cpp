#include "ResourcesManager.hpp"
#include "MeshUtilities.hpp"
#include "../helpers/Logger.hpp"
#include <fstream>
#include <sstream>
#include <tinydir/tinydir.h>
#include <miniz/miniz.h>

// By enabling RESOURCES_PACKAGED, the resources will be loaded from a zip archive
// instead of the resources directory. Basic text files can still be read from disk
// (for configuration, settings,...) by using Resources::loadStringFromExternalFile.

//#define RESOURCES_PACKAGED


/// Singleton.
Resources& Resources::manager(){
	static Resources* res = new Resources("../../../resources");
	return *res;
}

#ifdef RESOURCES_PACKAGED
Resources::Resources(const std::string & root) : _rootPath(root + ".zip"){
	Log::Info() << Log::Resources << "Loading resources from archive (" << _rootPath << ")." << std::endl;
	parseArchive(_rootPath);
}
#else
Resources::Resources(const std::string & root) : _rootPath(root){
	Log::Info() << Log::Resources << "Loading resources from disk (" << _rootPath << ")." << std::endl;
	parseDirectory(_rootPath);
}
#endif


void Resources::parseArchive(const std::string & archivePath){
	
	mz_zip_archive zip_archive = {0};
	int status = mz_zip_reader_init_file(&zip_archive, archivePath.c_str(), 0);
	if (!status){
		Log::Error() << Log::Resources << "Unable to load zip file \"" << archivePath << "\" (" << mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)) << ")." << std::endl;
	}
	
	// Get and print information about each file in the archive.
	for (unsigned int i = 0; i < (unsigned int)mz_zip_reader_get_num_files(&zip_archive); ++i){
		mz_zip_archive_file_stat file_stat;
		
		if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)){
			Log::Error() << Log::Resources << "Error reading file infos." << std::endl;
			mz_zip_reader_end(&zip_archive);
		}
		
		if(mz_zip_reader_is_file_a_directory(&zip_archive, i)){
			continue;
		}
		
		const std::string filePath = std::string(file_stat.m_filename);
		const std::string fileNameWithExt = filePath.substr(filePath.find_last_of("/\\") + 1);
		// Filter empty files and system files.
		if(fileNameWithExt.size() > 0 && fileNameWithExt.at(0) != '.' ){
			if(_files.count(fileNameWithExt) == 0){
				_files[fileNameWithExt] = filePath;
			} else {
				// If the file already exists somewhere else in the hierarchy, warn about this.
				Log::Error() << Log::Resources << "Error: asset named \"" << fileNameWithExt << "\" alread exists." << std::endl;
			}
		}
	}
	mz_zip_reader_end(&zip_archive);
}

void Resources::parseDirectory(const std::string & directoryPath){
	// Open directory.
	tinydir_dir dir;
	if(tinydir_open(&dir, directoryPath.c_str()) == -1){
		tinydir_close(&dir);
		Log::Error() << Log::Resources << "Unable to open resources directory at path \"" << directoryPath << "\"" << std::endl;
	}
	
	// For each file in dir.
	while (dir.has_next) {
		tinydir_file file;
		if(tinydir_readfile(&dir, &file) == -1){
			// Handle any read error.
			Log::Error() << Log::Resources << "Error getting file in directory \"" << std::string(dir.path) << "\"" << std::endl;
			
		} else if(file.is_dir){
			// Extract subdirectory name, check that it isn't a special dir, and recursively aprse it.
			const std::string dirName(file.name);
			if(dirName.size() > 0 && dirName != "." && dirName != ".."){
				// @CHECK: "/" separator on Windows.
				parseDirectory(directoryPath + "/" + dirName);
			}
			
		} else {
			// Else, we have a regular file.
			const std::string fileNameWithExt(file.name);
			// Filter empty files and system files.
			if(fileNameWithExt.size() > 0 && fileNameWithExt.at(0) != '.' ){
				if(_files.count(fileNameWithExt) == 0){
					// Store the file and its path.
					// @CHECK: "/" separator on Windows.
					_files[fileNameWithExt] = std::string(dir.path) + "/" + fileNameWithExt;
					
				} else {
					// If the file already exists somewhere else in the hierarchy, warn about this.
					Log::Error() << Log::Resources << "Error: asset named \"" << fileNameWithExt << "\" alread exists." << std::endl;
				}
			}
		}
		// Get to next file.
		if (tinydir_next(&dir) == -1){
			// Reach end of dir early.
			break;
		}
		
	}
	tinydir_close(&dir);
}


/// Image path utilities.

const std::vector<std::string> Resources::getCubemapPaths(const std::string & name){
	const std::vector<std::string> names { name + "_px", name + "_nx", name + "_py", name + "_ny", name + "_pz", name + "_nz" };
	std::vector<std::string> paths;
	paths.reserve(6);
	for(auto & faceName : names){
		const std::string filePath = getImagePath(faceName);
		// If a face is missing, cancel the whole loading.
		if(filePath.empty()){
			return std::vector<std::string>();
		}
		// Else append the path.
		paths.push_back(filePath);
	}
	return paths;
}

const std::string Resources::getImagePath(const std::string & name){
	std::string path = "";
	// Check if the file exists with an image extension.
	if(_files.count(name + ".png") > 0){
		path = _files[name + ".png"];
	} else if(_files.count(name + ".jpg") > 0){
		path = _files[name + ".jpg"];
	} else if(_files.count(name + ".jpeg") > 0){
		path = _files[name + ".jpeg"];
	} else if(_files.count(name + ".bmp") > 0){
		path = _files[name + ".bmp"];
	} else if(_files.count(name + ".tga") > 0){
		path = _files[name + ".tga"];
	} else if(_files.count(name + ".exr") > 0){
		path = _files[name + ".exr"];
	}
	return path;
}


/// Base methods.

#ifdef RESOURCES_PACKAGED

char * Resources::getRawData(const std::string & path, size_t & size) {
	char * rawContent;
	mz_zip_archive zip_archive = {0};
	int status = mz_zip_reader_init_file(&zip_archive, _rootPath.c_str(), 0);
	if (!status){
		Log::Error() << Log::Resources << "Unable to load zip file at path \"" << _rootPath << "\" (" << mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)) << ")." << std::endl;
		return NULL;
	}
	rawContent = (char*)mz_zip_reader_extract_file_to_heap(&zip_archive, path.c_str(), &size, 0);
	mz_zip_reader_end(&zip_archive);
	return rawContent;
}

#else
	
char * Resources::getRawData(const std::string & path, size_t & size) {
	return Resources::loadRawDataFromExternalFile(path, size);
}
	
#endif
	

const std::string Resources::getString(const std::string & filename){
	std::string path = "";
	if(_files.count(filename) > 0){
		path = _files[filename];
	} else if(_files.count(filename + ".txt") > 0){
		path = _files[filename + ".txt"];
	} else {
		Log::Error() << Log::Resources << "Unable to find text file named \"" << filename << "\"." << std::endl;
		return "";
	}
	
	size_t rawSize = 0;
	char* rawContent = Resources::manager().getRawData(path, rawSize);
	const std::string content(rawContent, rawSize);
	free(rawContent);
	return content;
}

/// Mesh method.

const MeshInfos Resources::getMesh(const std::string & name){
	if(_meshes.count(name) > 0){
		return _meshes[name];
	}

	MeshInfos infos;
	std::string completeName;
	
	// Load geometry. For now we only support OBJs.
	Mesh mesh;
	const std::string meshText = getString(name + ".obj");
	if(!meshText.empty()){
		std::stringstream meshStream(meshText);
	
		MeshUtilities::loadObj(meshStream, mesh, MeshUtilities::Indexed);
		// If uv or positions are missing, tangent/binormals won't be computed.
		MeshUtilities::computeTangentsAndBinormals(mesh);
		
	} else {
		Log::Error() << Log::Resources << "Unable to load mesh named " << name << "." << std::endl;
		return infos;
	}
	
	// Setup GL buffers and attributes.
	infos = GLUtilities::setupBuffers(mesh);
	_meshes[name] = infos;
	return infos;
}

const MeshInfos Resources::registerMesh(const std::string & name, const std::vector<unsigned int> & indices, const std::vector<glm::vec3> & positions, const std::vector<glm::vec3> & normals, const std::vector<glm::vec2> & texcoords){
	MeshInfos infos;
	Mesh mesh;
	//Init the mesh.
	mesh.indices = indices;
	mesh.positions = positions;
	mesh.normals = normals;
	mesh.texcoords = texcoords;
	// If uv or positions are missing, tangent/binormals won't be computed.
	//MeshUtilities::computeTangentsAndBinormals(mesh);
	MeshUtilities::centerAndUnitMesh(mesh);
	infos = GLUtilities::setupBuffers(mesh);
	
	_meshes[name] = infos;
	return infos;
}

/// Texture methods.

const TextureInfos Resources::getTexture(const std::string & name, bool srgb){
	
	// If texture already loaded, return it.
	if(_textures.count(name) > 0){
		return _textures[name];
	}
	// Else, find the corresponding file.
	TextureInfos infos;
	std::string path = getImagePath(name);
	
	if(!path.empty()){
		// Else, load it and store the infos.
		infos = GLUtilities::loadTexture({path}, srgb);
		_textures[name] = infos;
		return infos;
	}
	// Else, maybe there are custom mipmap levels.
	// In this case the true name is name_mipmaplevel.
	
	// How many mipmap levels can we accumulate?
	std::vector<std::string> paths;
	unsigned int lastMipmap = 0;
	std::string mipmapPath = getImagePath(name + "_" + std::to_string(lastMipmap));
	while(!mipmapPath.empty()) {
		// Transfer them to the final paths vector.
		paths.push_back(mipmapPath);
		++lastMipmap;
		mipmapPath = getImagePath(name + "_" + std::to_string(lastMipmap));
	}
	if(!paths.empty()){
		// We found the texture files.
		// Load them and store the infos.
		infos = GLUtilities::loadTexture(paths, srgb);
		_textures[name] = infos;
		return infos;
	}
	
	// If couldn't file the image, return empty texture infos.
	Log::Error() << Log::Resources << "Unable to find texture named \"" << name << "\"." << std::endl;
	return infos;
}

const TextureInfos Resources::registerTexture(const std::string & name, const plMipmap* textureData ){
	
	TextureInfos infos;
	infos.cubemap = false;
	infos.hdr = false;
	
	// Create 2D texture.
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	
	// Set proper max mipmap level.
	unsigned int mipmapCount = textureData->getNumLevels();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmapCount-1);
	
	// Texture settings.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	// Image infos.
	checkGLError();
	if(textureData->getCompressionType() == plBitmap::kDirectXCompression){
		GLenum format;
		switch(textureData->getDXCompression()){
			case plBitmap::kDXT1:
				format = int(textureData->getBPP()) == 32 ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				break;
			case plBitmap::kDXT3:
				format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;
			case plBitmap::kDXT5:
				format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
			default:
				Log::Error()<< "Unable to find format." << std::endl;
				return infos;
		}
		
		for(unsigned int mipid = 0; mipid < mipmapCount; ++mipid){
			// Magic formula for DXT miplevel size.
			unsigned int mipmapSize = ((textureData->getLevelHeight(mipid)+3)/4)*((textureData->getLevelWidth(mipid)+3)/4)*int(textureData->getDXBlockSize());
			glCompressedTexImage2D(GL_TEXTURE_2D, mipid, format,  textureData->getLevelWidth(mipid), textureData->getLevelHeight(mipid), 0,  mipmapSize, textureData->getLevelData(mipid));
		}
	} else {
		
		Log::Info() << name << std::endl;
		
		unsigned short flags = textureData->getFlags();
		unsigned short cflags = textureData->getCompressionType();
		unsigned short sflags = textureData->getSpace();
		unsigned short dflags = textureData->getDXCompression();
		unsigned short bflags = textureData->getARGBType();
		
		Log::Info() << "Flags: ";
		if(flags & plBitmap::kAlphaChannelFlag){ Log::Info() << "kAlphaChannelFlag" << ", "; }
		if(flags & plBitmap::kAlphaBitFlag){ Log::Info() << "kAlphaBitFlag" << ", "; }
		if(flags & plBitmap::kBumpEnvMap){ Log::Info() << "kBumpEnvMap" << ", "; }
		if(flags & plBitmap::kForce32Bit){ Log::Info() << "kForce32Bit" << ", "; }
		if(flags & plBitmap::kDontThrowAwayImage ){ Log::Info() << "kDontThrowAwayImage" << ", "; }
		if(flags & plBitmap::kForceOneMipLevel ){ Log::Info() << "kForceOneMipLevel" << ", "; }
		if(flags & plBitmap::kNoMaxSize ){ Log::Info() << "kNoMaxSize" << ", "; }
		if(flags & plBitmap::kIntensityMap ){ Log::Info() << "kIntensityMap" << ", "; }
		if(flags & plBitmap::kHalfSize){ Log::Info() << "kHalfSize" << ", "; }
		if(flags & plBitmap::kUserOwnsBitmap){ Log::Info() << "kUserOwnsBitmap" << ", "; }
		if(flags & plBitmap::kForceRewrite){ Log::Info() << "kForceRewrite" << ", "; }
		if(flags & plBitmap::kForceNonCompressed){ Log::Info() << "kForceNonCompressed" << ", "; }
		if(flags & plBitmap::kIsTexture){ Log::Info() << "kIsTexture" << ", "; }
		if(flags & plBitmap::kIsOffscreen){ Log::Info() << "kIsOffscreen" << ", "; }
		if(flags & plBitmap::kIsProjected){ Log::Info() << "kIsProjected" << ", "; }
		if(flags & plBitmap::kIsOrtho){ Log::Info() << "kIsOrtho" << ", "; }
		Log::Info() << std::endl;
		Log::Info() << "Compression: ";
		if(cflags ==  plBitmap::kUncompressed){Log::Info() << "kUncompressed" << ", ";};
		if(cflags ==  plBitmap::kDirectXCompression){Log::Info() << "kDirectXCompression" << ", ";};
		if(cflags ==  plBitmap::kJPEGCompression){Log::Info() << "kJPEGCompression" << ", ";};
		if(cflags ==  plBitmap::kPNGCompression){Log::Info() << "kPNGCompression" << ", ";};
		Log::Info() << std::endl;
//		Log::Info() << "Space: ";
//		if(sflags == plBitmap::kNoSpace){Log::Info() << "kNoSpace" << ", ";};
//		if(sflags == plBitmap::kDirectSpace){Log::Info() << "kDirectSpace" << ", ";};
//		if(sflags == plBitmap::kGraySpace){Log::Info() << "kGraySpace" << ", ";};
//		if(sflags == plBitmap::kIndexSpace){Log::Info() << "kIndexSpace" << ", ";};
//		Log::Info() << std::endl;
//		Log::Info() << "DXT: ";
//		if(dflags ==  plBitmap::kDXTError){Log::Info() << "kDXTError" << ", ";};
//		if(dflags ==  plBitmap::kDXT1){Log::Info() << "kDXT1" << ", ";};
//		if(dflags ==  plBitmap::kDXT2_UNUSED){Log::Info() << "kDXT2_UNUSED" << ", ";};
//		if(dflags ==  plBitmap::kDXT3){Log::Info() << "kDXT3" << ", ";};
//		if(dflags ==  plBitmap::kDXT4_UNUSED){Log::Info() << "kDXT4_UNUSED" << ", ";};
//		if(dflags ==  plBitmap::kDXT5){Log::Info() << "kDXT5" << ", ";};
//		Log::Info() << std::endl;
		Log::Info() << "Comps: ";
		if(bflags ==  plBitmap::kRGB8888){Log::Info() << "kRGB8888" << ", ";};
		if(bflags ==  plBitmap::kRGB4444){Log::Info() << "kRGB4444" << ", ";};
		if(bflags ==  plBitmap::kRGB1555){Log::Info() << "kRGB1555" << ", ";};
		if(bflags ==  plBitmap::kInten8){Log::Info() << "kInten8" << ", ";};
		if(bflags ==  plBitmap::kAInten88){Log::Info() << "kAInten88" << ", ";};
		Log::Info() << std::endl;
		const GLenum format = (bflags == plBitmap::kInten8) ? GL_RED : (bflags == plBitmap::kAInten88 ? GL_RG : GL_RGBA);
		const GLenum type = GL_UNSIGNED_BYTE;
		const GLenum preciseFormat = format;
		
		for(unsigned int mipid = 0; mipid < mipmapCount; ++mipid){
			// Magic formula for DXT miplevel size.
			
			glTexImage2D(GL_TEXTURE_2D, mipid, preciseFormat, textureData->getLevelWidth(mipid), textureData->getLevelHeight(mipid), 0, format, type, textureData->getLevelData(mipid));
		}
		// If only level 0 was given, generate mipmaps pyramid automatically.
		//glGenerateMipmap(GL_TEXTURE_2D);
	}
	checkGLError();
	
	
	infos.id = textureId;
	infos.width = textureData->getWidth();
	infos.height = textureData->getHeight();
	
	_textures[name] = infos;
	glBindTexture(GL_TEXTURE_2D, 0);
	return infos;
}



const TextureInfos Resources::getCubemap(const std::string & name, bool srgb){
	// If texture already loaded, return it.
	if(_textures.count(name) > 0){
		return _textures[name];
	}
	// Else, find the corresponding files.
	TextureInfos infos;
	std::vector<std::string> paths = getCubemapPaths(name);
	if(!paths.empty()){
		// We found the texture files.
		// Load them and store the infos.
		infos = GLUtilities::loadTextureCubemap({paths}, srgb);
		_textures[name] = infos;
		return infos;
	}
	// Else, maybe there are custom mipmap levels.
	// In this case the true name is name_mipmaplevel.
	
	// How many mipmap levels can we accumulate?
	std::vector<std::vector<std::string>> allPaths;
	unsigned int lastMipmap = 0;
	std::vector<std::string> mipmapPaths = getCubemapPaths(name + "_" + std::to_string(lastMipmap));
	while(!mipmapPaths.empty()) {
		// Transfer them to the final paths vector.
		allPaths.push_back(mipmapPaths);
		++lastMipmap;
		mipmapPaths = getCubemapPaths(name + "_" + std::to_string(lastMipmap));
	}
	if(!allPaths.empty()){
		// We found the texture files.
		// Load them and store the infos.
		infos = GLUtilities::loadTextureCubemap(allPaths, srgb);
		_textures[name] = infos;
		return infos;
	}
	Log::Error() << Log::Resources << "Unable to find cubemap named \"" << name << "\"." << std::endl;
	// Nothing found, return empty texture.
	return infos;
}

/// Program/shaders methods.

const std::string Resources::getShader(const std::string & name, const ShaderType & type){
	
	std::string path = "";
	const std::string extension = type == Vertex ? "vert" : "frag";
	// Directly query correct shader text file with extension.
	const std::string res = Resources::getString(name + "." + extension);
	// If the file is empty/doesn't exist, error.
	if(res.empty()){
		Log::Error() << Log::Resources << "Unable to find " << (type == Vertex ? "vertex" : "fragment") << " shader named \"" << name << "\"." << std::endl;
	}
	return res;
}

const std::shared_ptr<ProgramInfos> Resources::getProgram(const std::string & name){
	return getProgram(name, name, name);
}

const std::shared_ptr<ProgramInfos> Resources::getProgram(const std::string & name, const std::string & vertexName, const std::string & fragmentName) {
	if (_programs.count(name) > 0) {
		return _programs[name];
	}
	
	_programs.emplace(std::piecewise_construct,
					  std::forward_as_tuple(name),
					  std::forward_as_tuple(new ProgramInfos(vertexName, fragmentName)));
	
	return _programs[name];
}

void Resources::reload() {
	for (auto & prog : _programs) {
		prog.second->reload();
	}
	Log::Info() << Log::Resources << "Shader programs reloaded." << std::endl;
}


/// Static utilities methods.

char * Resources::loadRawDataFromExternalFile(const std::string & path, size_t & size) {
	char * rawContent;
	std::ifstream inputFile(path, std::ios::binary|std::ios::ate);
	if (inputFile.bad() || inputFile.fail()){
		Log::Error() << Log::Resources << "Unable to load file at path \"" << path << "\"." << std::endl;
		return NULL;
	}
	std::ifstream::pos_type fileSize = inputFile.tellg();
	rawContent = new char[fileSize];
	inputFile.seekg(0, std::ios::beg);
	inputFile.read(&rawContent[0], fileSize);
	inputFile.close();
	size = fileSize;
	return rawContent;
}

std::string Resources::loadStringFromExternalFile(const std::string & filename) {
	std::ifstream in;
	// Open a stream to the file.
	in.open(filename.c_str());
	if (!in) {
		Log::Error() << Log::Resources << "" << filename + " is not a valid file." << std::endl;
		return "";
	}
	std::stringstream buffer;
	// Read the stream in a buffer.
	buffer << in.rdbuf();
	// Create a string based on the content of the buffer.
	std::string line = buffer.str();
	in.close();
	return line;
}

std::string Resources::trim(const std::string & str, const std::string & del){
	const size_t firstNotDel = str.find_first_not_of(del);
	if(firstNotDel == std::string::npos){
		return "";
	}
	const size_t lastNotDel = str.find_last_not_of(del);
	return str.substr(firstNotDel, lastNotDel - firstNotDel + 1);
}


Resources::~Resources(){ }



