#include "kre_materials_lobes.h"
#include <loguru/loguru.hpp>

namespace KRE { namespace materials {

//	The wire format of this extension changed with no version key to hang a check
//	on, and ReadOptionalField ignores what it does not know -- so a `.lf_glb` cooked
//	before the change parses CLEANLY to tint 0 / level 0, which the loader reads as
//	"no sheen" and the asset silently loses its fuzz. Say so, once per run: a stale
//	cooked artifact is a recook, not a per-file error, and this is a loader on the
//	boot path (ERROR/FATAL would take the engine down over a build product).
static void WarnRetired(char const* spelling)
{
	static bool warned = false;
	if(warned) return;
	warned = true;
	LOG_F(WARNING, "KRE_materials_lobes: cooked asset carries the retired \"%s\"; "
	               "this loader reads sheen as tint/level/roughnessFactor only, so the "
	               "lobe is being dropped. Recook the .lf_glb (delete it and re-run the "
	               "cook) -- KreAssetMigration gates this.", spelling);
}

bool sheen_lobe::operator==(sheen_lobe const& b) const
{
	return tint == b.tint && level == b.level && roughnessFactor == b.roughnessFactor;
}

void from_json(nlohmann::json const& json, sheen_lobe & lobe)
{
	fx::gltf::detail::ReadOptionalField("tint",            json, lobe.tint);
	fx::gltf::detail::ReadOptionalField("level",           json, lobe.level);
	fx::gltf::detail::ReadOptionalField("roughnessFactor", json, lobe.roughnessFactor);
	if(json.is_object())
	{
		for(char const* retired : { "colorFactor", "colorTexture", "roughnessTexture" })
			if(json.find(retired) != json.end())
				WarnRetired(retired);
	}
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
	//	The subsurface lobe was retired outright (the loader reads
	//	KHR_materials_diffuse_transmission / _volume natively now); a cooked file that
	//	still carries it has its transmission read from nowhere.
	if(json.is_object() && json.find("subsurface") != json.end())
		WarnRetired("subsurface");
}

void to_json(nlohmann::json & json, lobes const& lobe)
{
	json = nlohmann::json::object();
	if(!lobe.sheen.empty()) json["sheen"] = lobe.sheen;
}

}}
