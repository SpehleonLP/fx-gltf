#include "extensionsandextras.h"

static const char * g_FamiliarExtensions[] =
{
	// in-house / already handled (was g_ExtensionsSupported)
	"LF_animRoot", "LF_root_motion", "LF_swizzle", "LF_colliders", "LF_compression",
	"LF_alternate", "MSFT_texture_dds", "MSFT_packing_normalRoughnessMetallic",
	"MSFT_packing_occlusionRoughnessMetallic", "AGI_articulations", "LF_RINTINTIN",
	"KHR_materials_pbrSpecularGlossiness", "KHR_materials_unlit", "KHR_materials_sheen",
	"KHR_materials_clearcoat", "KHR_mesh_quantization", "KHR_texture_transform",
	// yes-list (this slice)
	"KHR_materials_emissive_strength", "KHR_materials_ior", "KHR_node_visibility",
	"KHR_materials_specular", "EXT_mesh_gpu_instancing", "KHR_lights_punctual",
	"KHR_materials_variants", "MSFT_lod", "EXT_lights_image_based", "KHR_animation_pointer",
	""
};

bool fx::IsFamiliarExtension(std::string_view name)
{
	for(const char** p = g_FamiliarExtensions; **p != '\0'; ++p)
		if(name == *p) return true;
	return false;
}

void to_json(nlohmann::json & , Empty const& ) {}
void from_json(const nlohmann::json & , Empty & ) {}

template<typename S, typename T>
static void Pack(std::vector<S> & doc, std::vector<T> const& ee)
{
	size_t N = std::min(doc.size(), ee.size());

	for(size_t i = 0; i < N; ++i)
	{
		fx::gltf::detail::WriteField("extensions", doc[i].extensionsAndExtras, ee[i].extensions);
		fx::gltf::detail::WriteField("extras", doc[i].extensionsAndExtras, ee[i].extras);
	}
}

template<typename S, typename T>
static void Unpack(std::vector<S> const& doc, std::vector<T> & ee)
{
	for(size_t i = 0; i < doc.size(); ++i)
	{
		if(doc[i].extensionsAndExtras.empty())
			continue;

		if(ee.empty())
			ee.resize(doc.size());

		fx::gltf::detail::ReadOptionalField("extensions", doc[i].extensionsAndExtras, ee[i].extensions);
		fx::gltf::detail::ReadOptionalField("extras", doc[i].extensionsAndExtras, ee[i].extras);
	}
}

void fx::ExtensionsAndExtras::Pack(fx::gltf::Document & doc) const
{
#define Pack_MACRO(x) ::Pack(doc.x, x)

	Pack_MACRO(accessors);
	Pack_MACRO(animations);
	Pack_MACRO(buffers);
	Pack_MACRO(bufferViews);
	Pack_MACRO(cameras);
	Pack_MACRO(images);
	Pack_MACRO(materials);
	Pack_MACRO(meshes);
	Pack_MACRO(nodes);
	Pack_MACRO(samplers);
	Pack_MACRO(scenes);
	Pack_MACRO(skins);
	Pack_MACRO(textures);

#undef Pack_MACRO

	fx::gltf::detail::WriteField("extensions", doc.extensionsAndExtras, document.extensions);
	fx::gltf::detail::WriteField("extras", doc.extensionsAndExtras, document.extras);
	
//	doc.extensionsUsed.push_back("KHR_mesh_quantization"); 	
//	doc.extensionsRequired.push_back("KHR_mesh_quantization"); 
}

void fx::ExtensionsAndExtras::Unpack(fx::gltf::Document const& doc)
{
	for(auto const& key : doc.extensionsRequired)
	{
		if(fx::IsFamiliarExtension(key)) continue;

		char buffer[256];
		snprintf(buffer, sizeof(buffer), "unknown gltf extension: '%s' required", key.c_str());
		throw std::runtime_error(buffer);
	}

#define Pack_MACRO(x) ::Unpack(doc.x, x)
	Pack_MACRO(accessors);
	Pack_MACRO(animations);
	Pack_MACRO(buffers);
	Pack_MACRO(bufferViews);
	Pack_MACRO(cameras);
	Pack_MACRO(images);
	Pack_MACRO(materials);
	Pack_MACRO(meshes);
	Pack_MACRO(nodes);
	Pack_MACRO(samplers);
	Pack_MACRO(scenes);
	Pack_MACRO(skins);
	Pack_MACRO(textures);

#undef Pack_MACRO

	fx::gltf::detail::ReadOptionalField("extensions", doc.extensionsAndExtras, document.extensions);
	fx::gltf::detail::ReadOptionalField("extras", doc.extensionsAndExtras, document.extras);
}

namespace KHR
{
namespace materials
{

void from_json(nlohmann::json const& json, unlit & material);
void from_json(nlohmann::json const& json, pbrSpecularGlossiness & material);

void to_json(nlohmann::json & json, unlit const& material);
void to_json(nlohmann::json & json, pbrSpecularGlossiness const& material);

}
}

namespace AGI
{
void from_json(const nlohmann::json & json,  Articulations & obj);
void from_json(const nlohmann::json & json,  NodeArticulation & obj);

void to_json(nlohmann::json & json, Articulations const& obj);
void to_json(nlohmann::json & json, NodeArticulation const& obj);
}

namespace MSFT
{
void to_json(nlohmann::json & json, texture_dds const& db);
void from_json(const nlohmann::json & json, texture_dds & db);
}

namespace LF
{
void from_json(const nlohmann::json & json, Swizzle & db);
void from_json(const nlohmann::json & json, texture_cmp & db);
void from_json(const nlohmann::json & json, RootMotion & db);
void from_json(const nlohmann::json & json, RinTinTin & db);

void to_json(nlohmann::json & json, Swizzle const& db);
void to_json(nlohmann::json & json, texture_cmp const& db);
void to_json(nlohmann::json & json, RootMotion const& db);
void to_json(nlohmann::json & json, RinTinTin const& db);
}

namespace Extensions
{

void to_json(nlohmann::json & json, Animation const& db)
{
	fx::gltf::detail::WriteField("LF_root_motion", json, db.lf_rootMotion);
}

void from_json(const nlohmann::json & json, Animation & db)
{
	fx::gltf::detail::ReadOptionalField("LF_root_motion", json, db.lf_rootMotion);
}

void to_json(nlohmann::json & json, Image const& db)
{
	fx::gltf::detail::WriteField("LF_Compression", json, db.compression);
}

void from_json(const nlohmann::json & json, Image & db)
{
	fx::gltf::detail::ReadOptionalField("LF_Compression", json, db.compression);
}

void to_json(nlohmann::json & json, Sampler const& db)
{
	fx::gltf::detail::WriteField("LF_Swizzle", json, db.swizzle);
}

void from_json(const nlohmann::json & json, Sampler & db)
{
	fx::gltf::detail::ReadOptionalField("LF_Swizzle", json, db.swizzle);
}


void to_json(nlohmann::json & json, Document const& db)
{
	fx::gltf::detail::WriteField("AGI_articulations", json, db.AGI_articulations);
}

void from_json(const nlohmann::json & json, Document & db)
{
	fx::gltf::detail::ReadOptionalField("AGI_articulations", json, db.AGI_articulations);
}

void to_json(nlohmann::json & json, Node const& db)
{
	fx::gltf::detail::WriteField("AGI_articulations", json, db.AGI_articulations);
	fx::gltf::detail::WriteField("LF_RINTINTIN",      json, db.rintintin);
}

void from_json(const nlohmann::json & json, Node & db)
{
	fx::gltf::detail::ReadOptionalField("AGI_articulations", json, db.AGI_articulations);
	fx::gltf::detail::ReadOptionalField("LF_RINTINTIN",      json, db.rintintin);
}

void to_json(nlohmann::json & json, Texture const& extras)
{
	fx::gltf::detail::WriteField("MSFT_texture_dds", json,   extras.dds);
	fx::gltf::detail::WriteField("LZ4_texture_dds", json,   extras.lz4_dds);
}

void from_json(const nlohmann::json & json, Texture & extras)
{
	fx::gltf::detail::ReadOptionalField("MSFT_texture_dds", json,   extras.dds);
	fx::gltf::detail::ReadOptionalField("LZ4_texture_dds", json,   extras.lz4_dds);
}

void to_json(nlohmann::json & json, Material const& material)
{
	fx::gltf::detail::WriteField("KHR_materials_pbrSpecularGlossiness", json, material.pbrSpecularGlossiness);
	fx::gltf::detail::WriteField("KHR_materials_unlit", json, material.unlit);

	fx::gltf::detail::WriteField("KHR_materials_clearcoat", json, material.clearcoat);
	fx::gltf::detail::WriteField("KHR_materials_sheen", json, material.sheen);
}

void from_json(const nlohmann::json & json, Material & material)
{
	fx::gltf::detail::ReadOptionalField("KHR_materials_pbrSpecularGlossiness", json, material.pbrSpecularGlossiness);
	fx::gltf::detail::ReadOptionalField("KHR_materials_unlit", json, material.unlit);

	fx::gltf::detail::ReadOptionalField("KHR_materials_clearcoat", json, material.clearcoat);
	fx::gltf::detail::ReadOptionalField("KHR_materials_sheen", json, material.sheen);
}

}

namespace Extras
{
inline void to_json(nlohmann::json & json, Material const& material)
{
	fx::gltf::detail::WriteField("RENDER_ORDER", json, material.RENDER_ORDER, 0.f);
}

inline void from_json(nlohmann::json const& json, Material & material)
{
	fx::gltf::detail::ReadOptionalField("RENDER_ORDER", json, material.RENDER_ORDER);
#if HAVE_TEXTURE_PROJECTION
	for(auto i = json.cbegin(); i != json.cend(); ++i)
	{
		std::string key = i.key();

		if(i.key().find("PROJECTOR=") != std::string::npos)
		{
			material.projector = i.key().substr(10);
			break;
		}
	}
#endif
}

void to_json(nlohmann::json & json, Mesh::JointsUsed const& db)
{
	fx::gltf::detail::WriteField("skin", json, db.skin, -1);
	fx::gltf::detail::WriteField("joints", json, db.joints);
	
}

void from_json(const nlohmann::json & json, Mesh::JointsUsed & db)
{
	fx::gltf::detail::ReadRequiredField("skin", json, db.skin);
	fx::gltf::detail::ReadRequiredField("joints", json, db.joints);
}


void to_json(nlohmann::json & json, Mesh const& db)
{
	fx::gltf::detail::WriteField("Lifaundi_JointsUsed", json, db.Lifaundi_JointsUsed);
	fx::gltf::detail::WriteField("targetNames", json, db.targetNames);
}

void from_json(const nlohmann::json & json, Node & db)
{
	static std::string _capability = "capability";
	fx::gltf::detail::ReadOptionalField("Lifaundi_PartId", json, db.Lifaundi_PartId);
	fx::gltf::detail::ReadOptionalField("Lifaundi_Parent", json, db.Lifaundi_Parent);
	
	for(auto i = json.cbegin(); i != json.cend(); ++i)
	{
		std::string key = i.key();
		
		if(key.size() != _capability.size())
			continue;

		bool match=true;
		for(auto j = 0u; j < key.size(); ++j)
		{
			if(tolower(key[j]) != _capability[j])
			{
				match = false;
				break;
			}
		}
		
		if(match)
		{
			try
			{
				db.capability = i->get<std::string>();	
			}
			catch(...)
			{
			}
		}
	}
	
}

void to_json(nlohmann::json & json, Node const& db)
{
	fx::gltf::detail::WriteField("Lifaundi_PartId", json, db.Lifaundi_PartId, -1);
	fx::gltf::detail::WriteField("Lifaundi_Parent", json, db.Lifaundi_Parent, -1);
	
}

void from_json(const nlohmann::json & json, Mesh & db)
{
	fx::gltf::detail::ReadOptionalField("Lifaundi_JointsUsed", json, db.Lifaundi_JointsUsed);
	fx::gltf::detail::ReadOptionalField("targetNames", json, db.targetNames);
}

}

namespace LF
{

static void to_json(nlohmann::json & json, RootMotion::HermiteVec3 const& db)
{
	fx::gltf::detail::WriteField("v0",     json, (std::array<float, 3>&)db.v0,     fx::gltf::defaults::NullVec3);
	fx::gltf::detail::WriteField("t0_in",  json, (std::array<float, 3>&)db.t0_in,  fx::gltf::defaults::NullVec3);
	fx::gltf::detail::WriteField("t0_out", json, (std::array<float, 3>&)db.t0_out, fx::gltf::defaults::NullVec3);
	fx::gltf::detail::WriteField("v1",     json, (std::array<float, 3>&)db.v1,     fx::gltf::defaults::NullVec3);
	fx::gltf::detail::WriteField("t1_in",  json, (std::array<float, 3>&)db.t1_in,  fx::gltf::defaults::NullVec3);
	fx::gltf::detail::WriteField("t1_out", json, (std::array<float, 3>&)db.t1_out, fx::gltf::defaults::NullVec3);
}

static void from_json(nlohmann::json const& json, RootMotion::HermiteVec3 & db)
{
	fx::gltf::detail::ReadOptionalField("v0",     json, (std::array<float, 3>&)db.v0);
	fx::gltf::detail::ReadOptionalField("t0_in",  json, (std::array<float, 3>&)db.t0_in);
	fx::gltf::detail::ReadOptionalField("t0_out", json, (std::array<float, 3>&)db.t0_out);
	fx::gltf::detail::ReadOptionalField("v1",     json, (std::array<float, 3>&)db.v1);
	fx::gltf::detail::ReadOptionalField("t1_in",  json, (std::array<float, 3>&)db.t1_in);
	fx::gltf::detail::ReadOptionalField("t1_out", json, (std::array<float, 3>&)db.t1_out);
}

void to_json(nlohmann::json & json, RootMotion const& db)
{
	fx::gltf::detail::WriteField("attach_node",      json, db.attach_node,      -1);
	fx::gltf::detail::WriteField("translation_mask", json, db.translation_mask, uint8_t(0));
	fx::gltf::detail::WriteField("scaling_mask",     json, db.scaling_mask,     uint8_t(0));

	if(db.rotation_swing || db.rotation_twist || db.rotation_axis != glm::vec3(0, 1, 0))
	{
		nlohmann::json rot;
		fx::gltf::detail::WriteField("axis",  rot, (std::array<float, 3>&)db.rotation_axis, std::array<float, 3>{0, 1, 0});
		fx::gltf::detail::WriteField("swing", rot, db.rotation_swing, false);
		fx::gltf::detail::WriteField("twist", rot, db.rotation_twist, false);
		json["rotation"] = std::move(rot);
	}

	if(db.translation_mask != 0)
	{
		json["translation_curve"] = db.translation;
	}
	if(db.scaling_mask != 0)
	{
		json["scaling_curve"] = db.scaling;
	}
	if(db.rotation_swing)
	{
		json["swing_curve"] = db.swing;
	}
	if(db.rotation_twist)
	{
		json["twist_curve"] = db.twist;
	}

	if(db.residual_input >= 0 || db.residual_output >= 0 || db.residual_target_node >= 0)
	{
		nlohmann::json res;
		fx::gltf::detail::WriteField("input",       res, db.residual_input,       -1);
		fx::gltf::detail::WriteField("output",      res, db.residual_output,      -1);
		fx::gltf::detail::WriteField("target_node", res, db.residual_target_node, -1);
		json["residual"] = std::move(res);
	}
}

void from_json(nlohmann::json const& json, RootMotion & db)
{
	fx::gltf::detail::ReadOptionalField("attach_node",      json, db.attach_node);
	fx::gltf::detail::ReadOptionalField("translation_mask", json, db.translation_mask);
	fx::gltf::detail::ReadOptionalField("scaling_mask",     json, db.scaling_mask);

	auto rot_it = json.find("rotation");
	if(rot_it != json.end())
	{
		fx::gltf::detail::ReadOptionalField("axis",  *rot_it, (std::array<float, 3>&)db.rotation_axis);
		fx::gltf::detail::ReadOptionalField("swing", *rot_it, db.rotation_swing);
		fx::gltf::detail::ReadOptionalField("twist", *rot_it, db.rotation_twist);
	}

	auto tc_it = json.find("translation_curve");
	if(tc_it != json.end())
		from_json(*tc_it, db.translation);

	auto sc_it = json.find("scaling_curve");
	if(sc_it != json.end())
		from_json(*sc_it, db.scaling);

	auto sw_it = json.find("swing_curve");
	if(sw_it != json.end())
		from_json(*sw_it, db.swing);

	auto tw_it = json.find("twist_curve");
	if(tw_it != json.end())
		from_json(*tw_it, db.twist);

	auto res_it = json.find("residual");
	if(res_it != json.end())
	{
		fx::gltf::detail::ReadOptionalField("input",       *res_it, db.residual_input);
		fx::gltf::detail::ReadOptionalField("output",      *res_it, db.residual_output);
		fx::gltf::detail::ReadOptionalField("target_node", *res_it, db.residual_target_node);
	}
}

}

