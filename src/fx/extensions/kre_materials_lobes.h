#ifndef KRE_MATERIALS_LOBES_H
#define KRE_MATERIALS_LOBES_H
#include <fx/gltf.h>
#include <cstdint>

// The cooked form of sheen. KHR_materials_sheen is rewritten into this at cook
// and REMOVED from the material; the loader reads sheen from here only.
//
// Sheen is FACTORS ONLY (spec 2026-08-29-material-lobe-table-design §Rulings):
// a sheenColorTexture is reduced to tint/level at cook, never carried per pixel.
// The subsurface lobe this extension used to carry is gone with it:
// KHR_materials_diffuse_transmission and KHR_materials_volume are read natively
// by the loader and pass through the cook untouched.
namespace KRE
{
namespace materials
{
struct sheen_lobe
{
	//	sheenColour = mix(vec3(1), baseColor, tint) * level -- the light pass's whole
	//	view of sheen (unpack_principled.h.glsl). Folded from sheenColorFactor by
	//	SheenTintLevel (lobe_constants.h), which the cook and the loader share.
	float tint{0.f};
	float level{0.f};
	float roughnessFactor{0.f};

	bool is_empty{true};
	bool empty() const noexcept { return is_empty; }
	bool operator==(sheen_lobe const& b) const;
};

struct lobes
{
	sheen_lobe sheen;

	bool empty() const noexcept { return sheen.empty(); }
	bool operator==(lobes const& b) const { return sheen == b.sheen; }
};

void from_json(nlohmann::json const& json, sheen_lobe & lobe);
void to_json(nlohmann::json & json, sheen_lobe const& lobe);
void from_json(nlohmann::json const& json, lobes & lobe);
void to_json(nlohmann::json & json, lobes const& lobe);
}
}

#endif // KRE_MATERIALS_LOBES_H
