#ifndef EXTENSIONSANDEXTRAS_H
#define EXTENSIONSANDEXTRAS_H
#include "agi_articulation.h"
#include "msft_texture_dds.h"
#include "khr_materials.h"
#include "lf_rintintin.h"
#include "lf_root_motion.h"
#include <fx/gltf.h>

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

	KHR::materials::unlit unlit;


	bool empty() const { return pbrSpecularGlossiness.empty() && unlit.empty() && clearcoat.empty() && sheen.empty(); }
	bool operator==(Material const& it) const { return pbrSpecularGlossiness == it.pbrSpecularGlossiness && unlit == it.unlit && clearcoat == it.clearcoat && sheen == it.sheen; }
};

typedef ::Empty Mesh;

struct Node
{
	AGI::NodeArticulation	AGI_articulations;
	AGI::NodeArticulation	ANIM_articulations;
	LF::RinTinTin			rintintin;
			
	bool empty() const { return AGI_articulations.empty() && rintintin.empty(); }
	bool operator==(Node const& it) const { return AGI_articulations == it.AGI_articulations; }

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

struct Document
{
	AGI::Articulations AGI_articulations;
	AGI::Articulations ANIM_articulations;

	bool empty() const { return AGI_articulations.empty(); };
	bool operator==(Document const& it) const {
		return AGI_articulations == it.AGI_articulations; }
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
