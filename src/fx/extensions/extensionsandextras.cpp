#include "extensionsandextras.h"

static const char * g_FamiliarExtensions[] =
{
	// in-house / already handled (was g_ExtensionsSupported)
	"KRE_animRoot", "KRE_root_motion", "KRE_rintintin",
	"KRE_texture_dds", "MSFT_packing_normalRoughnessMetallic",
	"MSFT_packing_occlusionRoughnessMetallic", "AGI_articulations",
	"KHR_materials_pbrSpecularGlossiness", "KHR_materials_unlit", "KHR_materials_sheen",
	"KHR_mesh_quantization", "KHR_texture_transform",
	// used to use but we now don't use cause it never came up and took up a spot in the g buffer for nothing.
	"KHR_materials_clearcoat",
	// yes-list, we can parse but they do nothing, to-add
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

	// FLATTENED primitive scope (KHR_materials_variants). Primitives are nested
	// (doc.meshes[m].primitives[p]) with no doc-level array, so we walk them with
	// a running counter. INVARIANT: the flattened index is defined by MESH-MAJOR/
	// PRIMITIVE-MINOR traversal; Unpack (below) walks the SAME order so index N
	// refers to the same primitive on both sides (the same positional contract the
	// generic per-array Pack/Unpack above already rely on). Never reorder either.
	{
		size_t counter = 0;
		for(auto & m : doc.meshes)
			for(auto & p : m.primitives)
			{
				if(counter < primitives.size())
				{
					fx::gltf::detail::WriteField("extensions", p.extensionsAndExtras, primitives[counter].extensions);
					fx::gltf::detail::WriteField("extras", p.extensionsAndExtras, primitives[counter].extras);
				}
				++counter;
			}
	}

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

	// FLATTENED primitive scope (KHR_materials_variants) — see the matching Pack
	// loop for the indexing invariant. Count all primitives (mesh-major/primitive-
	// minor); only allocate the flattened vector if some primitive blob is non-
	// empty (mirrors the generic Unpack's lazy-resize). Pack rebuilds the IDENTICAL
	// traversal, so flattened index N always denotes the same primitive.
	{
		size_t total = 0;
		bool   any   = false;
		for(auto const& m : doc.meshes)
			for(auto const& p : m.primitives)
			{
				++total;
				if(!p.extensionsAndExtras.empty())
					any = true;
			}

		if(any && total)
		{
			primitives.resize(total);
			size_t counter = 0;
			for(auto const& m : doc.meshes)
				for(auto const& p : m.primitives)
				{
					fx::gltf::detail::ReadOptionalField("extensions", p.extensionsAndExtras, primitives[counter].extensions);
					fx::gltf::detail::ReadOptionalField("extras", p.extensionsAndExtras, primitives[counter].extras);
					++counter;
				}
		}
	}
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

namespace KRE
{
// to_json/from_json(texture_dds) now declared in kre_dds.h itself.
void from_json(const nlohmann::json & json, RootMotion & db);
void from_json(const nlohmann::json & json, RinTinTin & db);

void to_json(nlohmann::json & json, RootMotion const& db);
void to_json(nlohmann::json & json, RinTinTin const& db);
}

namespace Rhi
{
void from_json(const nlohmann::json & json, ComponentMapping & db);
void to_json(nlohmann::json & json, ComponentMapping const& db);
}

namespace Extensions
{

void to_json(nlohmann::json & json, Animation const& db)
{
	fx::gltf::detail::WriteField("KRE_root_motion", json, db.rootMotion);
}

void from_json(const nlohmann::json & json, Animation & db)
{
	fx::gltf::detail::ReadOptionalField("KRE_root_motion", json, db.rootMotion);
}

static void to_json(nlohmann::json & json, PunctualLight const& light)
{
	json["type"] = light.type;
	json["intensity"] = light.intensity;

	if(light.color != std::array<float, 3>{1, 1, 1})
		fx::gltf::detail::WriteField("color", json, (std::array<float, 3>&)light.color);

	if(light.range > 0.f)
		json["range"] = light.range;

	if(!light.name.empty())
		json["name"] = light.name;

	if(light.isSpot)
	{
		nlohmann::json spot;
		spot["innerConeAngle"] = light.spotInner;
		spot["outerConeAngle"] = light.spotOuter;
		json["spot"] = std::move(spot);
	}
}

static void from_json(const nlohmann::json & json, PunctualLight & light)
{
	fx::gltf::detail::ReadOptionalField("type", json, light.type);
	fx::gltf::detail::ReadOptionalField("color", json, (std::array<float, 3>&)light.color);
	fx::gltf::detail::ReadOptionalField("intensity", json, light.intensity);
	fx::gltf::detail::ReadOptionalField("range", json, light.range);
	fx::gltf::detail::ReadOptionalField("name", json, light.name);

	if(auto it = json.find("spot"); it != json.end())
	{
		light.isSpot = true;
		fx::gltf::detail::ReadOptionalField("innerConeAngle", *it, light.spotInner);
		fx::gltf::detail::ReadOptionalField("outerConeAngle", *it, light.spotOuter);
	}
}

// EXT_lights_image_based — one doc-level IBL light. Serialized field-by-field
// so the nested per-mip x per-face specularImages and the 9 SH coefficient
// triples round-trip byte-faithfully. Required properties (irradianceCoefficients,
// specularImages, specularImageSize) are always emitted; rotation/intensity are
// emitted only when non-default; name only when present.
static void to_json(nlohmann::json & json, IblLight const& light)
{
	if(!light.name.empty())
		json["name"] = light.name;

	if(light.rotation != std::array<float, 4>{0, 0, 0, 1})
		json["rotation"] = light.rotation;   // m4: field is already std::array<float,4>

	if(light.intensity != 1.f)
		json["intensity"] = light.intensity;

	nlohmann::json sh = nlohmann::json::array();
	for(auto const& c : light.irradianceCoefficients)
		sh.push_back(c);   // m4: c is already std::array<float,3>
	json["irradianceCoefficients"] = std::move(sh);

	nlohmann::json mips = nlohmann::json::array();
	for(auto const& mip : light.specularImages)
		mips.push_back(mip);
	json["specularImages"] = std::move(mips);

	json["specularImageSize"] = light.specularImageSize;
}

static void from_json(const nlohmann::json & json, IblLight & light)
{
	fx::gltf::detail::ReadOptionalField("name", json, light.name);
	fx::gltf::detail::ReadOptionalField("rotation", json, light.rotation);   // m4: already std::array<float,4>
	fx::gltf::detail::ReadOptionalField("intensity", json, light.intensity);
	fx::gltf::detail::ReadOptionalField("specularImageSize", json, light.specularImageSize);

	if(auto it = json.find("irradianceCoefficients"); it != json.end() && it->is_array())
		for(size_t i = 0; i < light.irradianceCoefficients.size() && i < it->size(); ++i)
			light.irradianceCoefficients[i] = (*it)[i].get<std::array<float, 3>>();   // m4

	if(auto it = json.find("specularImages"); it != json.end() && it->is_array())
	{
		light.specularImages.clear();
		light.specularImages.reserve(it->size());
		for(auto const& mip : *it)
		{
			std::vector<int32_t> faces;
			if(mip.is_array())
				for(auto const& face : mip)
					if(face.is_number_integer())
						faces.push_back(face.get<int32_t>());
			light.specularImages.push_back(std::move(faces));
		}
	}
}

void to_json(nlohmann::json & json, Document const& db)
{
	fx::gltf::detail::WriteField("AGI_articulations", json, db.AGI_articulations);

	if(!db.punctualLights.empty())
	{
		nlohmann::json lights = nlohmann::json::array();
		for(auto const& light : db.punctualLights)
		{
			nlohmann::json j;
			to_json(j, light);
			lights.push_back(std::move(j));
		}
		json["KHR_lights_punctual"]["lights"] = std::move(lights);
	}

	// KHR_materials_variants — doc-level `variants` names as [{ "name": ... }].
	if(!db.variantNames.empty())
	{
		nlohmann::json variants = nlohmann::json::array();
		for(auto const& name : db.variantNames)
		{
			nlohmann::json j;
			j["name"] = name;
			variants.push_back(std::move(j));
		}
		json["KHR_materials_variants"]["variants"] = std::move(variants);
	}

	// EXT_lights_image_based — doc-level `lights` array (never GC-compacted).
	if(!db.iblLights.empty())
	{
		nlohmann::json lights = nlohmann::json::array();
		for(auto const& light : db.iblLights)
		{
			nlohmann::json j;
			to_json(j, light);
			lights.push_back(std::move(j));
		}
		json["EXT_lights_image_based"]["lights"] = std::move(lights);
	}
}

void from_json(const nlohmann::json & json, Document & db)
{
	fx::gltf::detail::ReadOptionalField("AGI_articulations", json, db.AGI_articulations);

	if(auto it = json.find("KHR_lights_punctual"); it != json.end())
	{
		if(auto lights = it->find("lights"); lights != it->end() && lights->is_array())
		{
			db.punctualLights.reserve(lights->size());
			for(auto const& j : *lights)
			{
				PunctualLight light;
				from_json(j, light);
				db.punctualLights.push_back(std::move(light));
			}
		}
	}

	// KHR_materials_variants — doc-level `variants` names (index-stable).
	if(auto it = json.find("KHR_materials_variants"); it != json.end())
	{
		if(auto v = it->find("variants"); v != it->end() && v->is_array())
		{
			db.variantNames.reserve(v->size());
			for(auto const& j : *v)
			{
				std::string name;
				fx::gltf::detail::ReadOptionalField("name", j, name);
				db.variantNames.push_back(std::move(name));
			}
		}
	}

	// EXT_lights_image_based — doc-level `lights` array (index-stable).
	if(auto it = json.find("EXT_lights_image_based"); it != json.end())
	{
		if(auto lights = it->find("lights"); lights != it->end() && lights->is_array())
		{
			db.iblLights.reserve(lights->size());
			for(auto const& j : *lights)
			{
				IblLight light;
				from_json(j, light);
				db.iblLights.push_back(std::move(light));
			}
		}
	}
}

// KHR_materials_variants — per-primitive `mappings` (material + variant indices).
void to_json(nlohmann::json & json, Primitive const& db)
{
	if(db.variantMappings.empty())
		return;

	nlohmann::json mappings = nlohmann::json::array();
	for(auto const& m : db.variantMappings)
	{
		nlohmann::json j;
		// Emit `material` only when meaningful (>= 0). A default-constructed
		// VariantMapping carries material{-1}; writing the sentinel would emit an
		// invalid negative index (byte-faithful "emit only meaningful" rule). m1.
		if(m.material >= 0)
			j["material"] = m.material;
		j["variants"] = m.variants;
		mappings.push_back(std::move(j));
	}
	json["KHR_materials_variants"]["mappings"] = std::move(mappings);
}

void from_json(const nlohmann::json & json, Primitive & db)
{
	if(auto it = json.find("KHR_materials_variants"); it != json.end())
	{
		if(auto m = it->find("mappings"); m != it->end() && m->is_array())
		{
			db.variantMappings.reserve(m->size());
			for(auto const& j : *m)
			{
				Primitive::VariantMapping vm;
				fx::gltf::detail::ReadOptionalField("material", j, vm.material);
				fx::gltf::detail::ReadOptionalField("variants", j, vm.variants);
				db.variantMappings.push_back(std::move(vm));
			}
		}
	}
}

void to_json(nlohmann::json & json, Node const& db)
{
	fx::gltf::detail::WriteField("AGI_articulations", json, db.AGI_articulations);
	fx::gltf::detail::WriteField("KRE_rintintin",      json, db.rintintin);
	if(!db.visible)
		json["KHR_node_visibility"]["visible"] = false;

	if(!db.gpuInstancing.empty())
	{
		nlohmann::json attrs;
		for(auto const& kv : db.gpuInstancing.attributes) attrs[kv.first] = kv.second;
		json["EXT_mesh_gpu_instancing"]["attributes"] = std::move(attrs);
	}

	if(db.lightIndex != -1)
		json["KHR_lights_punctual"]["light"] = db.lightIndex;

	// MSFT_lod — alternate (lower-LOD) node indices.
	if(!db.msftLodIds.empty())
		json["MSFT_lod"]["ids"] = db.msftLodIds;
}

void from_json(const nlohmann::json & json, Node & db)
{
	fx::gltf::detail::ReadOptionalField("AGI_articulations", json, db.AGI_articulations);
	fx::gltf::detail::ReadOptionalField("KRE_rintintin",      json, db.rintintin);
	if(auto it = json.find("KHR_node_visibility"); it != json.end())
		fx::gltf::detail::ReadOptionalField("visible", *it, db.visible);

	if(auto it = json.find("EXT_mesh_gpu_instancing"); it != json.end())
		if(auto a = it->find("attributes"); a != it->end() && a->is_object())
			for(auto k = a->begin(); k != a->end(); ++k)
				db.gpuInstancing.attributes[k.key()] = k->get<int32_t>();

	if(auto it = json.find("KHR_lights_punctual"); it != json.end())
		fx::gltf::detail::ReadOptionalField("light", *it, db.lightIndex);

	// MSFT_lod — alternate (lower-LOD) node indices.
	if(auto it = json.find("MSFT_lod"); it != json.end())
		fx::gltf::detail::ReadOptionalField("ids", *it, db.msftLodIds);
}

// EXT_lights_image_based — per-scene `light` index into the doc-level IBL
// lights array.
void to_json(nlohmann::json & json, Scene const& db)
{
	if(db.iblLightIndex != -1)
		json["EXT_lights_image_based"]["light"] = db.iblLightIndex;
}

void from_json(const nlohmann::json & json, Scene & db)
{
	if(auto it = json.find("EXT_lights_image_based"); it != json.end())
		fx::gltf::detail::ReadOptionalField("light", *it, db.iblLightIndex);
}

// MSFT_lod — mesh-scope `ids` = alternate (lower-LOD) mesh indices.
void to_json(nlohmann::json & json, Mesh const& db)
{
	if(!db.msftLodIds.empty())
		json["MSFT_lod"]["ids"] = db.msftLodIds;
}

void from_json(const nlohmann::json & json, Mesh & db)
{
	if(auto it = json.find("MSFT_lod"); it != json.end())
		fx::gltf::detail::ReadOptionalField("ids", *it, db.msftLodIds);
}

void to_json(nlohmann::json & json, Texture const& extras)
{
	fx::gltf::detail::WriteField("KRE_texture_dds", json, extras.dds);
}

void from_json(const nlohmann::json & json, Texture & extras)
{
	fx::gltf::detail::ReadOptionalField("KRE_texture_dds", json, extras.dds);
}

// json-output-only: the standard extension carries `source` and nothing else
// -- no subtype, no swizzle -- so it does not route through to_json(Texture)
// above, which always names KRE_texture_dds.
nlohmann::json ToMsftTextureDds(int32_t source)
{
	nlohmann::json json;
	json["source"] = source;
	return json;
}

void to_json(nlohmann::json & json, Material const& material)
{
	fx::gltf::detail::WriteField("KHR_materials_pbrSpecularGlossiness", json, material.pbrSpecularGlossiness);
	fx::gltf::detail::WriteField("KHR_materials_unlit", json, material.unlit);

	fx::gltf::detail::WriteField("KHR_materials_clearcoat", json, material.clearcoat);
	fx::gltf::detail::WriteField("KHR_materials_sheen", json, material.sheen);
	fx::gltf::detail::WriteField("KHR_materials_specular", json, material.specular);

	if(material.emissiveStrength != 1.f)
		json["KHR_materials_emissive_strength"]["emissiveStrength"] = material.emissiveStrength;
	if(material.ior != 1.5f)
		json["KHR_materials_ior"]["ior"] = material.ior;
}

void from_json(const nlohmann::json & json, Material & material)
{
	fx::gltf::detail::ReadOptionalField("KHR_materials_pbrSpecularGlossiness", json, material.pbrSpecularGlossiness);
	fx::gltf::detail::ReadOptionalField("KHR_materials_unlit", json, material.unlit);

	fx::gltf::detail::ReadOptionalField("KHR_materials_clearcoat", json, material.clearcoat);
	fx::gltf::detail::ReadOptionalField("KHR_materials_sheen", json, material.sheen);
	fx::gltf::detail::ReadOptionalField("KHR_materials_specular", json, material.specular);

	if(auto it = json.find("KHR_materials_emissive_strength"); it != json.end())
		fx::gltf::detail::ReadOptionalField("emissiveStrength", *it, material.emissiveStrength);
	if(auto it = json.find("KHR_materials_ior"); it != json.end())
		fx::gltf::detail::ReadOptionalField("ior", *it, material.ior);
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
	fx::gltf::detail::WriteField("MSFT_screencoverage", json, db.msftScreencoverage);
}

void from_json(const nlohmann::json & json, Node & db)
{
	static std::string _capability = "capability";
	fx::gltf::detail::ReadOptionalField("Lifaundi_PartId", json, db.Lifaundi_PartId);
	fx::gltf::detail::ReadOptionalField("Lifaundi_Parent", json, db.Lifaundi_Parent);
	fx::gltf::detail::ReadOptionalField("MSFT_screencoverage", json, db.msftScreencoverage);

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
	fx::gltf::detail::WriteField("MSFT_screencoverage", json, db.msftScreencoverage);
}

void from_json(const nlohmann::json & json, Mesh & db)
{
	fx::gltf::detail::ReadOptionalField("Lifaundi_JointsUsed", json, db.Lifaundi_JointsUsed);
	fx::gltf::detail::ReadOptionalField("targetNames", json, db.targetNames);
	fx::gltf::detail::ReadOptionalField("MSFT_screencoverage", json, db.msftScreencoverage);
}

}

namespace KRE
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

