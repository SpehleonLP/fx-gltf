#include "kre_materials_lobes.h"

namespace KRE { namespace materials {

bool sheen_lobe::operator==(sheen_lobe const& b) const
{
	return tint == b.tint && level == b.level && roughnessFactor == b.roughnessFactor;
}

void from_json(nlohmann::json const& json, sheen_lobe & lobe)
{
	fx::gltf::detail::ReadOptionalField("tint",            json, lobe.tint);
	fx::gltf::detail::ReadOptionalField("level",           json, lobe.level);
	fx::gltf::detail::ReadOptionalField("roughnessFactor", json, lobe.roughnessFactor);
	lobe.is_empty = false;
}

void to_json(nlohmann::json & json, sheen_lobe const& lobe)
{
	json = nlohmann::json::object();   // all-default must still be {}
	fx::gltf::detail::WriteField("tint",            json, lobe.tint, 0.f);
	fx::gltf::detail::WriteField("level",           json, lobe.level, 0.f);
	fx::gltf::detail::WriteField("roughnessFactor", json, lobe.roughnessFactor, 0.f);
}

void from_json(nlohmann::json const& json, lobes & lobe)
{
	fx::gltf::detail::ReadOptionalField("sheen", json, lobe.sheen);
}

void to_json(nlohmann::json & json, lobes const& lobe)
{
	json = nlohmann::json::object();
	if(!lobe.sheen.empty()) json["sheen"] = lobe.sheen;
}

}}
