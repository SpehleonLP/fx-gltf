#ifndef EXTENSIONSANDEXTRAS_H
#define EXTENSIONSANDEXTRAS_H
#include "agi_articulation.h"
#include "msft_texture_dds.h"
#include "khr_materials.h"
#include "lf_rintintin.h"
#include "lf_root_motion.h"
#include <fx/gltf.h>
#include <array>
#include <map>
#include <vector>

struct Empty
{
	bool empty() const { return true; }
	bool operator==(Empty const&) const { return true; }
};

void to_json(nlohmann::json & , Empty const& );
void from_json(const nlohmann::json & , Empty & );

namespace Extensions
{
typedef ::Empty Accessor;

struct Animation
{
	LF::RootMotion lf_rootMotion;
	bool empty() const { return lf_rootMotion.empty(); }
	bool operator==(Animation const& it) const { return lf_rootMotion == it.lf_rootMotion; }
};

typedef ::Empty Buffer;
typedef ::Empty BufferView;
typedef ::Empty Camera;

struct Image
{
//this is instructions for the compressor to re-export this image.
	LF::texture_cmp compression;
	uint32_t        uncompressedSize{};

	bool empty() const { return compression.empty(); }
	bool operator==(Image const& it) const { return compression == it.compression && uncompressedSize == it.uncompressedSize; }
};

struct Sampler
{
	LF::Swizzle     swizzle;

	bool empty() const { return swizzle.empty(); }
	bool operator==(Sampler const& it) const { return swizzle == it.swizzle; }
};

struct Material
{
	struct Texture
	{
		KHR::Texture::Transform textureTransform;
	};

	KHR::materials::pbrSpecularGlossiness pbrSpecularGlossiness;
	KHR::materials::clearcoat			  clearcoat;
	KHR::materials::sheen				  sheen;
	KHR::materials::specular			  specular;

	KHR::materials::unlit unlit;

	float emissiveStrength{1.f};
	float ior{1.5f};

	bool empty() const { return pbrSpecularGlossiness.empty() && unlit.empty() && clearcoat.empty() && sheen.empty() && specular.empty()
		&& emissiveStrength == 1.f && ior == 1.5f; }
	bool operator==(Material const& it) const { return pbrSpecularGlossiness == it.pbrSpecularGlossiness && unlit == it.unlit && clearcoat == it.clearcoat && sheen == it.sheen && specular == it.specular
		&& emissiveStrength == it.emissiveStrength && ior == it.ior; }
};

typedef ::Empty Mesh;

struct Node
{
	AGI::NodeArticulation	AGI_articulations;
	AGI::NodeArticulation	ANIM_articulations;
	LF::RinTinTin			rintintin;
	bool					visible{true};
	int32_t					lightIndex{-1};	// KHR_lights_punctual: index into Document::punctualLights

	struct GpuInstancing
	{
		std::map<std::string,int32_t> attributes;
		bool empty() const { return attributes.empty(); }
		bool operator==(GpuInstancing const& it) const { return attributes == it.attributes; }
	} gpuInstancing;

	bool empty() const { return AGI_articulations.empty() && rintintin.empty() && visible && gpuInstancing.empty() && lightIndex == -1; }
	bool operator==(Node const& it) const { return AGI_articulations == it.AGI_articulations && visible == it.visible && gpuInstancing == it.gpuInstancing && lightIndex == it.lightIndex; }

};

typedef ::Empty Scene;
typedef ::Empty Skin;

struct Texture
{
	MSFT::texture_dds   dds;
	MSFT::texture_dds   lz4_dds;

	bool empty() const { return dds.empty(); }
	bool operator==(Texture const& it) const { return dds == it.dds; }
};

// KHR_lights_punctual — one entry of the doc-level `lights` array.
struct PunctualLight
{
	std::string				type;					// "directional"|"point"|"spot"
	std::array<float, 3>	color{1, 1, 1};
	float					intensity{1.f};
	float					range{0.f};				// 0 = infinite -> omit on write
	std::string				name;
	float					spotInner{0.f};
	float					spotOuter{0.7853982f};	// pi/4
	bool					isSpot{false};

	bool operator==(PunctualLight const& it) const
	{
		return type == it.type && color == it.color && intensity == it.intensity && range == it.range
			&& name == it.name && spotInner == it.spotInner && spotOuter == it.spotOuter && isSpot == it.isSpot;
	}
};

struct Document
{
	AGI::Articulations AGI_articulations;
	AGI::Articulations ANIM_articulations;

	std::vector<PunctualLight> punctualLights;	// KHR_lights_punctual — doc-level array, never GC-compacted

	bool empty() const { return AGI_articulations.empty() && punctualLights.empty(); };
	bool operator==(Document const& it) const {
		return AGI_articulations == it.AGI_articulations && punctualLights == it.punctualLights; }
};


void to_json(nlohmann::json & json, Animation const& db);
void to_json(nlohmann::json & json, Image const& db);
void to_json(nlohmann::json & json, Sampler const& db);
void to_json(nlohmann::json & json, Document const& db);
void to_json(nlohmann::json & json, Node const& db);
void to_json(nlohmann::json & json, Texture const& extras);
void to_json(nlohmann::json & json, Material const& material);

void from_json(const nlohmann::json & json, Animation & db);
void from_json(const nlohmann::json & json, Image & db);
void from_json(const nlohmann::json & json, Sampler & db);
void from_json(const nlohmann::json & json, Document & db);
void from_json(const nlohmann::json & json, Node & db);
void from_json(const nlohmann::json & json, Texture & extras);
void from_json(const nlohmann::json & json, Material & material);

};

namespace Extras
{
typedef ::Empty Accessor;
typedef ::Empty Animation;
typedef ::Empty Buffer;
typedef ::Empty BufferView;
typedef ::Empty Camera;
typedef ::Empty Image;

struct Material
{
	typedef ::Empty Texture;

	float RENDER_ORDER{-1};


	bool empty() const { return RENDER_ORDER < 0; }
	bool operator==(Material const& it) const { return RENDER_ORDER == it.RENDER_ORDER; }
};

typedef ::Empty Primitive;

struct Mesh 
{
	struct JointsUsed
	{
		int skin;
		std::vector<int> joints;
		bool empty() const { return skin < 0; }
		bool operator==(JointsUsed const& it) const { return skin == it.skin && joints == it.joints; }
	};
	
	std::vector<JointsUsed> Lifaundi_JointsUsed;
	std::vector<std::string> targetNames;
	
	bool empty() const { return Lifaundi_JointsUsed.empty(); }
	bool operator==(Mesh const& it) const { return Lifaundi_JointsUsed == it.Lifaundi_JointsUsed; }
};

void to_json(nlohmann::json & , Mesh const& );
void from_json(const nlohmann::json & , Mesh & );

struct Node 
{
	int Lifaundi_PartId{-1};
	int Lifaundi_Parent{-1};
	std::string capability;
	
	bool empty() const { return Lifaundi_PartId == -1 && Lifaundi_Parent == -1; }
	bool operator==(Node const& it) const { return Lifaundi_PartId == it.Lifaundi_PartId && Lifaundi_Parent == it.Lifaundi_Parent; }
};

typedef ::Empty Sampler;
typedef ::Empty Scene;
typedef ::Empty Skin;
typedef ::Empty Texture;

typedef ::Empty Document;
};

namespace fx
{

bool IsFamiliarExtension(std::string_view name);

struct ExtensionsAndExtras
{
	template<typename S, typename T>
	struct Pair
	{
		S extensions{};
		T extras{};

		bool operator==(Pair<S, T> const& it) const
		{
			return extensions == it.extensions && extras == it.extras;
		}
	};

#define EE_t(x) typedef Pair<Extensions::x, Extras::x> x;
#define EE_v(x) EE_t(x) std::vector<Pair<Extensions::x, Extras::x>>

	void Pack(fx::gltf::Document & doc) const;
	void Unpack(gltf::Document const& doc);

	EE_v(Accessor)   accessors;
	EE_v(Animation)  animations;
	EE_v(Buffer)     buffers;
	EE_v(BufferView) bufferViews;
	EE_v(Camera)     cameras;
	EE_v(Image)      images;
	EE_v(Material)   materials;
	EE_v(Mesh)       meshes;
	EE_v(Node)       nodes;
	EE_v(Sampler)    samplers;
	EE_v(Scene)      scenes;
	EE_v(Skin)       skins;
	EE_v(Texture)    textures;

	EE_t(Document)
	Document document;

#undef EE_t
#undef EE_v
};


}





#endif // EXTENSIONSANDEXTRAS_H
