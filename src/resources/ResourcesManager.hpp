#ifndef ResourcesManager_h
#define ResourcesManager_h

#include "../helpers/GLUtilities.hpp"
#include "../helpers/ProgramInfos.hpp"
#include <gl3w/gl3w.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

class plMipmap;
class plCubicEnvironmap;

class Resources {
	
	friend class ImageUtilities;
	
public:
	
	enum ShaderType {
		Vertex, Fragment
	};
	
	/// Singleton management.
	static Resources& manager();
	
private:
	
	Resources(const std::string & root);
	
	void parseArchive(const std::string & archivePath);
	
	void parseDirectory(const std::string & directoryPath);
	
	const std::string getImagePath(const std::string & name);
	
	const std::vector<std::string> getCubemapPaths(const std::string & name);
	
	char * getRawData(const std::string & path, size_t & size);
	
public:

	const std::string getString(const std::string & filename);
	
	const MeshInfos getMesh(const std::string & name);
	
	const MeshInfos registerMesh(const std::string & name, const std::vector<unsigned int> & indices, const std::vector<glm::vec3> & positions, const std::vector<glm::vec3> & normals, const std::vector<glm::vec2> & texcoords);
	
	const TextureInfos getTexture(const std::string & name, bool srgb = true);
	
	const TextureInfos registerTexture(const std::string & name, const plMipmap* textureData );
	
	const TextureInfos registerCubemap(const std::string & name, plCubicEnvironmap* textureData );
	
	const TextureInfos getCubemap(const std::string & name, bool srgb = true);
	
	const std::string getShader(const std::string & name, const ShaderType & type);
	
	const std::shared_ptr<ProgramInfos> getProgram(const std::string & name);
	
	const std::shared_ptr<ProgramInfos> getProgram(const std::string & name, const std::string & vertexName, const std::string & fragmentName);
	
	void reload();
	
	void reset();
	
	static char * loadRawDataFromExternalFile(const std::string & path, size_t & size);
	
	static std::string loadStringFromExternalFile(const std::string & filename);
	
	static std::string trim(const std::string & str, const std::string & del);
	
	
private:
	
	~Resources();
	
	Resources& operator= (const Resources&);
	
	Resources (const Resources&);
	
	/// Attributes
	
	const std::string _rootPath;
	
	std::map<std::string, std::string> _files;
	
	std::map<std::string, TextureInfos> _textures;
	
	std::map<std::string, MeshInfos> _meshes;
	
	std::map<std::string, std::shared_ptr<ProgramInfos>> _programs;
	
};

#endif
