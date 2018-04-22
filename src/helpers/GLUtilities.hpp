#ifndef GLUtilities_h
#define GLUtilities_h
#include "../resources/MeshUtilities.hpp"
#include "../Framebuffer.hpp"
#include <PRP/Surface/plMipmap.h>
#include <PRP/Surface/plCubicEnvironmap.h>
#include <gl3w/gl3w.h>
#include <string>
#include <vector>
#include <memory>

/// This macro is used to check for OpenGL errors with access to the file and line number where the error is detected.

#define checkGLError() _checkGLError(__FILE__, __LINE__, "")
#define checkGLErrorInfos(infos) _checkGLError(__FILE__ , __LINE__, infos)

/// Converts a GLenum error number into a human-readable string.
std::string getGLErrorString(GLenum error);

/// Check if any OpenGL error has been detected and log it.
int _checkGLError(const char *file, int line, const std::string & infos);

struct TextureInfos {
	GLuint id;
	int width;
	int height;
	int mipmap;
	bool cubemap;
	bool hdr;
	TextureInfos() : id(0), width(0), height(0), mipmap(0), cubemap(false), hdr(false) {}

};



static const unsigned char getQuadrant(const glm::vec3 & point, const glm::mat4 & viewproj)  {
	const auto homCoords = viewproj * glm::vec4(point, 1.0f);
	const glm::vec3 cCoords = glm::vec3(homCoords)/homCoords.w;
	return ((cCoords.x > 1.0f) << 3) | ((cCoords.x < -1.0f) << 2)
		 | ((cCoords.z > 1.0f) << 5) | ((cCoords.x < -1.0f) << 4)
		 | ((cCoords.y > 1.0f) << 1) | (cCoords.y < -1.0f);
}

struct BoundingBox {
	glm::vec3 mins;
	glm::vec3 maxs;
	glm::vec3 center;
	glm::vec3 c000, c001, c010, c011, c100, c101, c110, c111;
	
	BoundingBox(){
		mins = glm::vec3(0.0f);
		maxs = glm::vec3(0.0f);
		updateValues();
	}
	
	BoundingBox(const glm::vec3 minis, const glm::vec3 maxis){
		mins = minis;
		maxs = maxis;
		updateValues();
	}
	
	glm::vec3 getScale() const {
		return (maxs - mins);
	}
	
	void updateValues(){
		center = 0.5f*(maxs+mins);
		c000 = glm::vec3(mins.x, mins.y, mins.z);
		c001 = glm::vec3(mins.x, mins.y, maxs.z);
		c010 = glm::vec3(mins.x, maxs.y, mins.z);
		c011 = glm::vec3(mins.x, maxs.y, maxs.z);
		c100 = glm::vec3(maxs.x, mins.y, mins.z);
		c101 = glm::vec3(maxs.x, mins.y, maxs.z);
		c110 = glm::vec3(maxs.x, maxs.y, mins.z);
		c111 = glm::vec3(maxs.x, maxs.y, maxs.z);
	}
	
	BoundingBox transform(const glm::mat4 & transf){
		const auto tc000 = glm::vec3(transf * glm::vec4(c000, 1.0f));
		const auto tc001 = glm::vec3(transf * glm::vec4(c001, 1.0f));
		const auto tc010 = glm::vec3(transf * glm::vec4(c010, 1.0f));
		const auto tc011 = glm::vec3(transf * glm::vec4(c011, 1.0f));
		const auto tc100 = glm::vec3(transf * glm::vec4(c100, 1.0f));
		const auto tc101 = glm::vec3(transf * glm::vec4(c101, 1.0f));
		const auto tc110 = glm::vec3(transf * glm::vec4(c110, 1.0f));
		const auto tc111 = glm::vec3(transf * glm::vec4(c111, 1.0f));
		const auto pmins = glm::min(glm::min(glm::min(tc000, tc001), glm::min(tc010, tc011)),
									glm::min(glm::min(tc100, tc101), glm::min(tc110, tc111)));
		const auto pmaxs = glm::max(glm::max(glm::max(tc000, tc001), glm::max(tc010, tc011)),
									glm::max(glm::max(tc100, tc101), glm::max(tc110, tc111)));
		return BoundingBox(pmins, pmaxs);
		
	}
	
	bool contains(const glm::vec3 & point) const {
		return point.x <= maxs.x && point.x >= mins.x
		&& point.y <= maxs.y && point.y >= mins.y
		&& point.z <= maxs.z && point.z >= mins.z;
	}
	
	bool intersectsFrustum(const glm::mat4 & viewproj) const {
		// TODO: improve support for covering/side-to-side intersections.
		const unsigned char f000 = getQuadrant(c000, viewproj);
		const unsigned char f001 = getQuadrant(c001, viewproj);
		const unsigned char f010 = getQuadrant(c010, viewproj);
		const unsigned char f011 = getQuadrant(c011, viewproj);
		const unsigned char f100 = getQuadrant(c100, viewproj);
		const unsigned char f101 = getQuadrant(c101, viewproj);
		const unsigned char f110 = getQuadrant(c110, viewproj);
		const unsigned char f111 = getQuadrant(c111, viewproj);
		
		const unsigned char fall = f000 & f001 & f010 & f011 & f100 & f101 & f110 & f111;
		if(fall != 0){
			return false;
		}
		return true;
	}
	
	bool isVisible(const glm::vec3 & point, const glm::vec3 & dir) const {
		return (glm::dot(c000 - point, dir) > 0.0f)
			|| (glm::dot(c001 - point, dir) > 0.0f)
			|| (glm::dot(c010 - point, dir) > 0.0f)
			|| (glm::dot(c011 - point, dir) > 0.0f)
			|| (glm::dot(c100 - point, dir) > 0.0f)
			|| (glm::dot(c101 - point, dir) > 0.0f)
			|| (glm::dot(c110 - point, dir) > 0.0f)
		|| (glm::dot(c111 - point, dir) > 0.0f);
	}
	
	bool isVisibleFast(const glm::vec3 & point, const glm::vec3 & dir) const {
		return (glm::dot(mins - point, dir) > 0.0f || glm::dot(maxs - point, dir) > 0.0f);
	}
	
	BoundingBox& operator +=(const BoundingBox & other){
		mins = glm::min(mins, other.mins);
		maxs = glm::max(maxs, other.maxs);
		return *this;
	}
	
	BoundingBox& operator +=(const glm::vec3 & other){
		mins = glm::min(mins, other);
		maxs = glm::max(maxs, other);
		return *this;
	}
};

struct MeshInfos {
	GLuint vId;
	GLuint eId;
	GLsizei count;
	size_t uvCount;
	BoundingBox bbox;
	
	MeshInfos() : vId(0), eId(0), count(0), uvCount(0), bbox(glm::vec3(0.0f), glm::vec3(0.0f)) {}

};


class GLUtilities {
	
private:
	/// Load a shader of the given type from a string
	static GLuint loadShader(const std::string & prog, GLuint type);
	
	static void savePixels(const GLenum type, const GLenum format, const unsigned int width, const unsigned int height, const unsigned int components, const std::string & path, const bool flip, const bool ignoreAlpha);
	
public:
	
	// Program setup.
	/// Create a GLProgram using the shader code contained in the given strings.
	static GLuint createProgram(const std::string & vertexContent, const std::string & fragmentContent);
	
	// Texture loading.
	static TextureInfos  loadTexture(const plMipmap * textureData);
	
	static TextureInfos loadCubemap(plCubicEnvironmap* textureData);
	/// 2D texture.
	static TextureInfos loadTexture(const std::vector<std::string>& path, bool sRGB);
	
	/// Cubemap texture.
	static TextureInfos loadTextureCubemap(const std::vector<std::vector<std::string>> & paths, bool sRGB);
	
	// Mesh loading.
	static MeshInfos setupBuffers(const Mesh & mesh);
	
	// Framebuffer saving to disk.
	static void saveFramebuffer(const std::shared_ptr<Framebuffer> & framebuffer, const unsigned int width, const unsigned int height, const std::string & path, const bool flip = true, const bool ignoreAlpha = false);
	
	static void saveDefaultFramebuffer(const unsigned int width, const unsigned int height, const std::string & path);
	
};


#endif
