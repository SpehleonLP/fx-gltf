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
	               "this loader reads sheen as colorFactor/roughnessFactor only, so the "
	               "lobe is being dropped. Recook the .lf_glb (Engine --cook <source> --force) "
	               "-- KreAssetMigration gates this.", spelling);
}

//	The legacy texture slots stay out: they are read-only and never written back,
//	so two lobes that write the same JSON are equal.
bool sheen_lobe::operator==(sheen_lobe const& b) const
{
	return colorFactor == b.colorFactor && roughnessFactor == b.roughnessFactor;
}

void from_json(nlohmann::json const& json, sheen_lobe & lobe)
{
	fx::gltf::detail::ReadOptionalField("colorFactor",     json, lobe.colorFactor);
	fx::gltf::detail::ReadOptionalField("roughnessFactor", json, lobe.roughnessFactor);
	fx::gltf::detail::ReadOptionalField("colorTexture",     json, lobe.legacyColorTexture);
	fx::gltf::detail::ReadOptionalField("roughnessTexture", json, lobe.legacyRoughnessTexture);
	if(json.is_object())
	{
		for(char const* retired : { "tint", "level", "colorTexture", "roughnessTexture" })
			if(json.find(retired) != json.end())
				WarnRetired(retired);
	}
	lobe.is_empty = false;
}

void to_json(nlohmann::json & json, sheen_lobe const& lobe)
{
	json = nlohmann::json::object();   // all-default must still be {}
	std::array<float, 3> const black{0.f, 0.f, 0.f};
	if(lobe.colorFactor != black) json["colorFactor"] = lobe.colorFactor;
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
