#include "Logger.hpp"
#include <PRP/Geometry/plDrawableSpans.h>
#include <imgui/imgui.h>
#include <ctime>
#include <iomanip>
#include <iostream>


void logSpanProps(unsigned int iceflags){
	if(iceflags!=0){
		Log::Info() << "Span flags: ";
		if(iceflags & plSpan::kLiteMaterial){ Log::Info() << "kLiteMaterial" << ", "; }
		if(iceflags & plSpan::kPropNoDraw){ Log::Info() << "kPropNoDraw" << ", "; }
		if(iceflags & plSpan::kPropNoShadowCast){ Log::Info() << "kPropNoShadowCast" << ", "; }
		if(iceflags & plSpan::kPropFacesSortable){ Log::Info() << "kPropFacesSortable" << ", "; }
		if(iceflags & plSpan::kPropVolatile){ Log::Info() << "kPropVolatile" << ", "; }
		if(iceflags & plSpan::kWaterHeight){ Log::Info() << "kWaterHeight" << ", "; }
		if(iceflags & plSpan::kPropRunTimeLight){ Log::Info() << "kPropRunTimeLight" << ", "; }
		if(iceflags & plSpan::kPropReverseSort){ Log::Info() << "kPropReverseSort" << ", "; }
		if(iceflags & plSpan::kPropHasPermaLights){ Log::Info() << "kPropHasPermaLights" << ", "; }
		if(iceflags & plSpan::kPropHasPermaProjs){ Log::Info() << "kPropHasPermaProjs" << ", "; }
		if(iceflags & plSpan::kLiteVtxPreshaded){ Log::Info() << "kLiteVtxPreshaded" << ", "; }
		if(iceflags & plSpan::kLiteVtxNonPreshaded){ Log::Info() << "kLiteVtxNonPreshaded" << ", "; }
		if(iceflags & plSpan::kLiteProjection){ Log::Info() << "kLiteProjection" << ", "; }
		if(iceflags & plSpan::kLiteShadowErase){ Log::Info() << "kLiteShadowErase" << ", "; }
		if(iceflags & plSpan::kLiteShadow){ Log::Info() << "kLiteShadow" << ", "; }
		if(iceflags & plSpan::kPropMatHasSpecular){ Log::Info() << "kPropMatHasSpecular" << ", "; }
		if(iceflags & plSpan::kPropProjAsVtx){ Log::Info() << "kPropProjAsVtx" << ", "; }
		if(iceflags & plSpan::kPropSkipProjection){ Log::Info() << "kPropSkipProjection" << ", "; }
		if(iceflags & plSpan::kPropNoShadow){ Log::Info() << "kPropNoShadow" << ", "; }
		if(iceflags & plSpan::kPropForceShadow){ Log::Info() << "kPropForceShadow" << ", "; }
		if(iceflags & plSpan::kPropDisableNormal){ Log::Info() << "kPropDisableNormal" << ", "; }
		if(iceflags & plSpan::kPropCharacter){ Log::Info() << "kPropCharacter" << ", "; }
		if(iceflags & plSpan::kPartialSort){ Log::Info() << "kPartialSort" << ", "; }
		if(iceflags & plSpan::kVisLOS){ Log::Info() << "kVisLOS" << ", "; }
		Log::Info() << std::endl;
	}
}

void logCompFlags(unsigned int cflags){
	if(cflags!=0){
		Log::Info() << "\tCompflags: ";
		if(cflags & hsGMaterial::kCompShaded){ Log::Info() << "kCompShaded" << ", ";}
		if(cflags & hsGMaterial::kCompEnvironMap){ Log::Info() << "kCompEnvironMap" << ", ";}
		if(cflags & hsGMaterial::kCompProjectOnto){ Log::Info() << "kCompProjectOnto" << ", ";}
		if(cflags & hsGMaterial::kCompSoftShadow){ Log::Info() << "kCompSoftShadow" << ", ";}
		if(cflags & hsGMaterial::kCompSpecular){ Log::Info() << "kCompSpecular" << ", ";}
		if(cflags & hsGMaterial::kCompTwoSided){ Log::Info() << "kCompTwoSided" << ", ";}
		if(cflags & hsGMaterial::kCompDrawAsSplats){ Log::Info() << "kCompDrawAsSplats" << ", ";}
		if(cflags & hsGMaterial::kCompAdjusted){ Log::Info() << "kCompAdjusted" << ", ";}
		if(cflags & hsGMaterial::kCompNoSoftShadow){ Log::Info() << "kCompNoSoftShadow" << ", ";}
		if(cflags & hsGMaterial::kCompDynamic){ Log::Info() << "kCompDynamic" << ", ";}
		if(cflags & hsGMaterial::kCompDecal){ Log::Info() << "kCompDecal" << ", ";}
		if(cflags & hsGMaterial::kCompIsEmissive){ Log::Info() << "kCompIsEmissive" << ", ";}
		if(cflags & hsGMaterial::kCompIsLightMapped){ Log::Info() << "kCompIsLightMapped" << ", ";}
		if(cflags & hsGMaterial::kCompNeedsBlendChannel){ Log::Info() << "kCompNeedsBlendChannel" << ", ";}
		Log::Info() << std::endl;
	}
}

std::string logLayer(plLayerInterface * lay){
	std::string layerString;
	if(lay->getTexture() != NULL){
		layerString.append("Texture: " + lay->getTexture()->getName().to_std_string() + ", LOD bias " + std::to_string(lay->getLODBias()) + ").\n");
	}
	
//	layerString.append("Opacity: " + lay->getOpacity() + ", ");
//	"Power: " + lay->getSpecularPower() + ", ";
//	std::endl;
//	"Amb: " + lay->getAmbient() <<", ";
//	"Pres: " + lay->getPreshade() <<", ";
//	"Spec: " + lay->getSpecular() <<", ";
//	"Run: " + lay->getRuntime() <<", ";
//	layerString.append(std::endl;
	
	const unsigned int fblend = lay->getState().fBlendFlags;
	const unsigned int fclamp = lay->getState().fClampFlags;
	const unsigned int fmisc = lay->getState().fMiscFlags;
	const unsigned int fshade = lay->getState().fShadeFlags;
	const unsigned int fz = lay->getState().fZFlags;
	
	if(fblend != 0){
		layerString.append("Blend: ");
		if(fblend & hsGMatState::kBlendTest){ layerString.append("kBlendTest, "); }
		if(fblend & hsGMatState::kBlendAlpha){ layerString.append("kBlendAlpha, "); }
		if(fblend & hsGMatState::kBlendMult){ layerString.append("kBlendMult, "); }
		if(fblend & hsGMatState::kBlendAdd){ layerString.append("kBlendAdd, "); }
		if(fblend & hsGMatState::kBlendAddColorTimesAlpha){ layerString.append("kBlendAddColorTimesAlpha, "); }
		if(fblend & hsGMatState::kBlendAntiAlias){ layerString.append("kBlendAntiAlias, "); }
		if(fblend & hsGMatState::kBlendDetail){ layerString.append("kBlendDetail, "); }
		if(fblend & hsGMatState::kBlendNoColor){ layerString.append("kBlendNoColor, "); }
		if(fblend & hsGMatState::kBlendMADD){ layerString.append("kBlendMADD, "); }
		if(fblend & hsGMatState::kBlendDot3){ layerString.append("kBlendDot3, "); }
		if(fblend & hsGMatState::kBlendAddSigned){ layerString.append("kBlendAddSigned, "); }
		if(fblend & hsGMatState::kBlendAddSigned2X){ layerString.append("kBlendAddSigned2X, "); }
		//if(fblend & kBlendMask){ layerString.append("kBlendMask, "); }
		if(fblend & hsGMatState::kBlendInvertAlpha){ layerString.append("kBlendInvertAlpha, "); }
		if(fblend & hsGMatState::kBlendInvertColor){ layerString.append("kBlendInvertColor, "); }
		if(fblend & hsGMatState::kBlendAlphaMult){ layerString.append("kBlendAlphaMult, "); }
		if(fblend & hsGMatState::kBlendAlphaAdd){ layerString.append("kBlendAlphaAdd, "); }
		if(fblend & hsGMatState::kBlendNoVtxAlpha){ layerString.append("kBlendNoVtxAlpha, "); }
		if(fblend & hsGMatState::kBlendNoTexColor){ layerString.append("kBlendNoTexColor, "); }
		if(fblend & hsGMatState::kBlendNoTexAlpha){ layerString.append("kBlendNoTexAlpha, "); }
		if(fblend & hsGMatState::kBlendInvertVtxAlpha){ layerString.append("kBlendInvertVtxAlpha, "); }
		if(fblend & hsGMatState::kBlendAlphaAlways){ layerString.append("kBlendAlphaAlways, "); }
		if(fblend & hsGMatState::kBlendInvertFinalColor){ layerString.append("kBlendInvertFinalColor, "); }
		if(fblend & hsGMatState::kBlendInvertFinalAlpha){ layerString.append("kBlendInvertFinalAlpha, "); }
		if(fblend & hsGMatState::kBlendEnvBumpNext){ layerString.append("kBlendEnvBumpNext, "); }
		if(fblend & hsGMatState::kBlendSubtract){ layerString.append("kBlendSubtract, "); }
		if(fblend & hsGMatState::kBlendRevSubtract){ layerString.append("kBlendRevSubtract, "); }
		if(fblend & hsGMatState::kBlendAlphaTestHigh){ layerString.append("kBlendAlphaTestHigh, "); }
		layerString.append("\n");
	}
	if(fclamp!=0){
		layerString.append("Clamp: ");
		if(fclamp == hsGMatState::kClampTextureU){ layerString.append("kClampTextureU, "); }
		if(fclamp == hsGMatState::kClampTextureV){ layerString.append("kClampTextureV, "); }
		if(fclamp == hsGMatState::kClampTexture){ layerString.append("kClampTexture, "); }
		layerString.append("\n");
	}
	
	if(fmisc!=0){
		layerString.append("Misc: ");
		if(fmisc & hsGMatState::kMiscWireFrame){ layerString.append("kMiscWireFrame, "); }
		if(fmisc & hsGMatState::kMiscDrawMeshOutlines){ layerString.append("kMiscDrawMeshOutlines, "); }
		if(fmisc & hsGMatState::kMiscTwoSided){ layerString.append("kMiscTwoSided, "); }
		if(fmisc & hsGMatState::kMiscDrawAsSplats){ layerString.append("kMiscDrawAsSplats, "); }
		if(fmisc & hsGMatState::kMiscAdjustPlane){ layerString.append("kMiscAdjustPlane, "); }
		if(fmisc & hsGMatState::kMiscAdjustCylinder){ layerString.append("kMiscAdjustCylinder, "); }
		if(fmisc & hsGMatState::kMiscAdjustSphere){ layerString.append("kMiscAdjustSphere, "); }
		//if(fmisc & kMiscAdjust){ layerString.append("kMiscAdjust, "); }
		if(fmisc & hsGMatState::kMiscTroubledLoner){ layerString.append("kMiscTroubledLoner, "); }
		if(fmisc & hsGMatState::kMiscBindSkip){ layerString.append("kMiscBindSkip, "); }
		if(fmisc & hsGMatState::kMiscBindMask){ layerString.append("kMiscBindMask, "); }
		if(fmisc & hsGMatState::kMiscBindNext){ layerString.append("kMiscBindNext, "); }
		if(fmisc & hsGMatState::kMiscLightMap){ layerString.append("kMiscLightMap, "); }
		if(fmisc & hsGMatState::kMiscUseReflectionXform){ layerString.append("kMiscUseReflectionXform, "); }
		if(fmisc & hsGMatState::kMiscPerspProjection){ layerString.append("kMiscPerspProjection, "); }
		if(fmisc & hsGMatState::kMiscOrthoProjection){ layerString.append("kMiscOrthoProjection, "); }
		//if(fmisc & kMiscProjection){ layerString.append("kMiscProjection, "); }
		if(fmisc & hsGMatState::kMiscRestartPassHere){ layerString.append("kMiscRestartPassHere, "); }
		if(fmisc & hsGMatState::kMiscBumpLayer){ layerString.append("kMiscBumpLayer, "); }
		if(fmisc & hsGMatState::kMiscBumpDu){ layerString.append("kMiscBumpDu, "); }
		if(fmisc & hsGMatState::kMiscBumpDv){ layerString.append("kMiscBumpDv, "); }
		if(fmisc & hsGMatState::kMiscBumpDw){ layerString.append("kMiscBumpDw, "); }
		//if(fmisc & kMiscBumpChans){ layerString.append("kMiscBumpChans, "); }
		if(fmisc & hsGMatState::kMiscNoShadowAlpha){ layerString.append("kMiscNoShadowAlpha, "); }
		if(fmisc & hsGMatState::kMiscUseRefractionXform){ layerString.append("kMiscUseRefractionXform, "); }
		if(fmisc & hsGMatState::kMiscCam2Screen){ layerString.append("kMiscCam2Screen, "); }
		//if(fmisc & hsGMatState::kAllMiscFlags){ layerString.append("kAllMiscFlags, "); }
		layerString.append("\n");
	}
	
	if(fshade!=0){
		layerString.append("Shade: ");
		if(fshade & hsGMatState::kShadeSoftShadow){ layerString.append("kShadeSoftShadow, "); }
		if(fshade & hsGMatState::kShadeNoProjectors){ layerString.append("kShadeNoProjectors, "); }
		if(fshade & hsGMatState::kShadeEnvironMap){ layerString.append("kShadeEnvironMap, "); }
		if(fshade & hsGMatState::kShadeVertexShade){ layerString.append("kShadeVertexShade, "); }
		if(fshade & hsGMatState::kShadeNoShade){ layerString.append("kShadeNoShade, "); }
		if(fshade & hsGMatState::kShadeBlack){ layerString.append("kShadeBlack, "); }
		if(fshade & hsGMatState::kShadeSpecular){ layerString.append("kShadeSpecular, "); }
		if(fshade & hsGMatState::kShadeNoFog){ layerString.append("kShadeNoFog, "); }
		if(fshade & hsGMatState::kShadeWhite){ layerString.append("kShadeWhite, "); }
		if(fshade & hsGMatState::kShadeSpecularAlpha){ layerString.append("kShadeSpecularAlpha, "); }
		if(fshade & hsGMatState::kShadeSpecularColor){ layerString.append("kShadeSpecularColor, "); }
		if(fshade & hsGMatState::kShadeSpecularHighlight){ layerString.append("kShadeSpecularHighlight, "); }
		if(fshade & hsGMatState::kShadeVertColShade){ layerString.append("kShadeVertColShade, "); }
		if(fshade & hsGMatState::kShadeInherit){ layerString.append("kShadeInherit, "); }
		if(fshade & hsGMatState::kShadeIgnoreVtxIllum){ layerString.append("kShadeIgnoreVtxIllum, "); }
		if(fshade & hsGMatState::kShadeEmissive){ layerString.append("kShadeEmissive, "); }
		if(fshade & hsGMatState::kShadeReallyNoFog){ layerString.append("kShadeReallyNoFog, "); }
		layerString.append("\n");
	}
	if(fz != 0){
		layerString.append("Depth: ");
		if(fz & hsGMatState::kZIncLayer){ layerString.append("kZIncLayer, "); }
		if(fz & hsGMatState::kZClearZ){ layerString.append("kZClearZ, "); }
		if(fz & hsGMatState::kZNoZRead){ layerString.append("kZNoZRead, "); }
		if(fz & hsGMatState::kZNoZWrite){ layerString.append("kZNoZWrite, "); }
		//if(fz & kZMask){ layerString.append("kZMask, "); }
		if(fz & hsGMatState::kZLODBias){ layerString.append("kZLODBias, "); }
		layerString.append("\n");
	}
	
	if(lay->getUnderLay().Exists()){
		layerString.append("Underlay: " + lay->getUnderLay()->getName().to_std_string() + "\n");
	}
	return layerString;
}

// We statically initialize the default logger.
// We don't really care about its exact construction/destruction moments,
// but we want it to always be created.
Log* Log::_defaultLogger = new Log();

void Log::set(LogLevel l){
	_level = l;
	_appendPrefix = (l != LogLevel::INFO);
}

Log::Log(){
	_level = LogLevel::INFO;
	_logToStdin = true;
	_verbose = false;
	_mute = false;
	_ignoreUntilFlush = false;
	_appendPrefix = false;
}

Log::Log(const std::string & filePath, const bool logToStdin, const bool verbose){
	_level = LogLevel::INFO;
	_logToStdin = logToStdin;
	_verbose = verbose;
	_mute = false;
	_ignoreUntilFlush = false;
	_appendPrefix = false;
	// Create file if it doesnt exist.
	setFile(filePath, false);
}

void Log::setFile(const std::string & filePath, const bool flushExisting){
	if(flushExisting){
		_stream << std::endl;
		flush();
	}
	if(_file.is_open()){
		_file.close();
	}
	_file.open(filePath, std::ofstream::app);
	if(_file.is_open()){
		_file << "-- New session - " << time(NULL) << " -------------------------------" << std::endl;
	} else {
		std::cerr << "[Logger] Unable to create log file at path " << filePath << "." << std::endl;
	}
}

void Log::setVerbose(const bool verbose){
	_verbose = verbose;
}

void Log::mute(){
	flush();
	_mute = true;
}

void Log::unmute(){
	flush();
	_mute = false;
}

void Log::Mute(){
	_defaultLogger->mute();
}

void Log::Unmute(){
	_defaultLogger->unmute();
}


void Log::setDefaultFile(const std::string & filePath){
	_defaultLogger->setFile(filePath);
}

void Log::setDefaultVerbose(const bool verbose){
	_defaultLogger->setVerbose(verbose);
}

Log& Log::Info(){
	_defaultLogger->set(LogLevel::INFO);
	return *_defaultLogger;
}

Log& Log::Warning(){
	_defaultLogger->set(LogLevel::WARNING);
	return *_defaultLogger;
}

Log& Log::Error(){
	_defaultLogger->set(LogLevel::ERROR);
	return *_defaultLogger;
}

void Log::flush(){
	if(!_ignoreUntilFlush && !_mute){
		
		const std::string finalStr =  _stream.str();
		
		if(_logToStdin){
			if(_level == LogLevel::INFO){
				std::cout << finalStr << std::flush;
			} else {
				std::cerr << finalStr << std::flush;
			}
		}
		if(_file.is_open()){
			_file << finalStr << std::flush;
		}
		
		_fullLog.append(finalStr);
		
	}
	_ignoreUntilFlush = false;
	_appendPrefix = false;
	_stream.str(std::string());
	_stream.clear();
	_level = LogLevel::INFO;
}

void Log::display(){
	if(ImGui::Begin("Log")){
		static bool setScroll = true;
		if(ImGui::Button("Clear")){ _fullLog = ""; }
		ImGui::SameLine();
		const bool shouldCopy = ImGui::Button("Copy");
		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
		if(shouldCopy){ ImGui::LogToClipboard(); }
		ImGui::TextUnformatted(_fullLog.c_str());
		if(setScroll){ImGui::SetScrollHere(1.0f); setScroll = false;}
		ImGui::EndChild();
	}
	ImGui::End();
}

void Log::appendIfNeeded(){
	if(_appendPrefix){
		_appendPrefix = false;
		_stream << _levelStrings[_level];
	}
}

Log & Log::operator<<(const LogDomain& domain){
	if(domain != Verbose){
		_stream << "[" << _domainStrings[domain] << "] ";
		appendIfNeeded();
	} else if(!_verbose){
		// In this case, we want to ignore until the next flush.
		_ignoreUntilFlush = true;
	}
	return *this;
}

Log& Log::operator<<(std::ostream& (*modif)(std::ostream&)){
	appendIfNeeded();

	modif(_stream);
	if(modif == static_cast<std::ostream& (*)(std::ostream&)>(std::flush) ||
	   modif == static_cast<std::ostream& (*)(std::ostream&)>(std::endl)){
		flush();
	}
	return *this;
}

Log& Log::operator<<(std::ios_base& (*modif)(std::ios_base&)){
	modif(_stream);
	return *this;
}

// GLM types support.
Log & Log::operator<<(const glm::mat4& input){
	appendIfNeeded();
	_stream << "mat4( " << input[0][0] << ", " << input[0][1] << ", " << input[0][2] << ", " << input[0][3] << " | ";
	_stream << input[1][0] << ", " << input[1][1] << ", " << input[1][2] << ", " << input[1][3] << " | ";
	_stream <<  input[2][0] << ", " << input[2][1] << ", " << input[2][2] << ", " << input[2][3] << " | ";
	_stream << input[3][0] << ", " << input[3][1] << ", " << input[3][2] << ", " << input[3][3] << " )";
	return *this;
}

Log & Log::operator<<(const glm::mat3& input){
	appendIfNeeded();
	_stream << "mat3( " << input[0][0] << ", " << input[0][1] << ", " << input[0][2] << " | ";
	_stream << input[1][0] << ", " << input[1][1] << ", " << input[1][2] << " | ";
	_stream << input[2][0] << ", " << input[2][1] << ", " << input[2][2] << " )";
	return *this;
}

Log & Log::operator<<(const glm::mat2& input){
	appendIfNeeded();
	_stream << "mat2( " << input[0][0] << ", " << input[0][1] << " | ";
	_stream << input[1][0] << ", " << input[1][1] << " )";
	return *this;
}

Log & Log::operator<<(const glm::vec4& input){
	appendIfNeeded();
	_stream << "vec4( " << input[0] << ", " << input[1] << ", " << input[2] << ", " << input[3] << " )";
	return *this;
}

Log & Log::operator<<(const glm::vec3& input){
	appendIfNeeded();
	_stream << "vec3( " << input[0] << ", " << input[1] << ", " << input[2] << " )";
	return *this;
}

Log & Log::operator<<(const glm::vec2& input){
	appendIfNeeded();
	_stream << "vec2( " << input[0] << ", " << input[1] << " )";
	return *this;
}

Log &Log::operator<<(const ST::string& ststr){
	appendIfNeeded();
	_stream << ststr.to_std_string();
	return *this;
}

Log & Log::operator<<(const hsMatrix44& input){
	appendIfNeeded();
	_stream << input.toString().to_std_string();
	return *this;
}

Log & Log::operator<<(const hsVector3& input){
	appendIfNeeded();
	_stream << "vec3( " << input.X << ", " << input.Y << ", " << input.Z << " )";
	return *this;
}

Log & Log::operator<<(const hsColorRGBA& input){
	appendIfNeeded();
	_stream << "(" << input.r << ", " << input.g << ", " << input.b << ")";
	return *this;
}
