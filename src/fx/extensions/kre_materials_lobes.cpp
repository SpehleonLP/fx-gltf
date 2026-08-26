#include "kre_materials_lobes.h"
#include <string_view>

namespace fx { namespace gltf {
void to_json(nlohmann::json & json, Material::Texture const& materialTexture);
void from_json(nlohmann::json const& json, Material::Texture & materialTexture);
}}

namespace KRE { namespace materials {

char const* ToString(SubsurfaceSource s)
{
	return s == SubsurfaceSource::Transmission ? "transmission" : "diffuse_transmission";
}

SubsurfaceSource SubsurfaceSourceFromString(std::string_view s)
{
	return s == "transmission" ? SubsurfaceSource::Transmission : SubsurfaceSource::DiffuseTransmission;
}

bool sheen_lobe::operator==(sheen_lobe const& b) const
{
	return colorFactor == b.colorFactor && roughnessFactor == b.roughnessFactor
	    && colorTexture == b.colorTexture && roughnessTexture == b.roughnessTexture;
}

bool subsurface_lobe::operator==(subsurface_lobe const& b) const
{
	return weightFactor == b.weightFactor && colorFactor == b.colorFactor
	    && roughnessFactor == b.roughnessFactor && weightTexture == b.weightTexture
	    && colorTexture == b.colorTexture && roughnessTexture == b.roughnessTexture
	    && source == b.source;
}

void from_json(nlohmann::json const& json, sheen_lobe & lobe)
{
	fx::gltf::detail::ReadOptionalField("colorFactor",      json, lobe.colorFactor);
	fx::gltf::detail::ReadOptionalField("roughnessFactor",  json, lobe.roughnessFactor);
	fx::gltf::detail::ReadOptionalField("colorTexture",     json, lobe.colorTexture);
	fx::gltf::detail::ReadOptionalField("roughnessTexture", json, lobe.roughnessTexture);
	lobe.is_empty = false;
}

void to_json(nlohmann::json & json, sheen_lobe const& lobe)
{
	json = nlohmann::json::object();   // all-default must still be {}
	fx::gltf::detail::WriteField("colorFactor",      json, lobe.colorFactor, {0.f, 0.f, 0.f});
	fx::gltf::detail::WriteField("roughnessFactor",  json, lobe.roughnessFactor, 0.f);
	fx::gltf::detail::WriteField("colorTexture",     json, lobe.colorTexture);
	fx::gltf::detail::WriteField("roughnessTexture", json, lobe.roughnessTexture);
}

void from_json(nlohmann::json const& json, subsurface_lobe & lobe)
{
	fx::gltf::detail::ReadOptionalField("weightFactor",     json, lobe.weightFactor);
	fx::gltf::detail::ReadOptionalField("colorFactor",      json, lobe.colorFactor);
	fx::gltf::detail::ReadOptionalField("roughnessFactor",  json, lobe.roughnessFactor);
	fx::gltf::detail::ReadOptionalField("weightTexture",    json, lobe.weightTexture);
	fx::gltf::detail::ReadOptionalField("colorTexture",     json, lobe.colorTexture);
	fx::gltf::detail::ReadOptionalField("roughnessTexture", json, lobe.roughnessTexture);

	std::string source = "diffuse_transmission";
	fx::gltf::detail::ReadOptionalField("source", json, source);
	lobe.source = SubsurfaceSourceFromString(source);
	lobe.is_empty = false;
}

void to_json(nlohmann::json & json, subsurface_lobe const& lobe)
{
	json = nlohmann::json::object();
	fx::gltf::detail::WriteField("weightFactor",     json, lobe.weightFactor, 0.f);
	fx::gltf::detail::WriteField("colorFactor",      json, lobe.colorFactor, {1.f, 1.f, 1.f});
	fx::gltf::detail::WriteField("roughnessFactor",  json, lobe.roughnessFactor, 1.f);
	fx::gltf::detail::WriteField("weightTexture",    json, lobe.weightTexture);
	fx::gltf::detail::WriteField("colorTexture",     json, lobe.colorTexture);
	fx::gltf::detail::WriteField("roughnessTexture", json, lobe.roughnessTexture);
	if(lobe.source != SubsurfaceSource::DiffuseTransmission)
		json["source"] = ToString(lobe.source);
}

void from_json(nlohmann::json const& json, lobes & lobe)
{
	fx::gltf::detail::ReadOptionalField("sheen",      json, lobe.sheen);
	fx::gltf::detail::ReadOptionalField("subsurface", json, lobe.subsurface);
}

void to_json(nlohmann::json & json, lobes const& lobe)
{
	json = nlohmann::json::object();
	if(!lobe.sheen.empty())      json["sheen"]      = lobe.sheen;
	if(!lobe.subsurface.empty()) json["subsurface"] = lobe.subsurface;
}

}}
