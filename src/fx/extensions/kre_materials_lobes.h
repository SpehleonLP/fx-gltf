#ifndef KRE_MATERIALS_LOBES_H
#define KRE_MATERIALS_LOBES_H
#include <fx/gltf.h>
#include <array>
#include <cstdint>

// The cooked form of sheen. KHR_materials_sheen is rewritten into this at cook
// and REMOVED from the material; the loader reads sheen from here only.
//
// Sheen is FACTORS ONLY (spec 2026-08-29-material-lobe-table-design §Rulings):
// a sheenColorTexture is reduced at cook, never carried per pixel.
// The subsurface lobe this extension used to carry is gone with it:
// KHR_materials_diffuse_transmission and KHR_materials_volume are read natively
// by the loader and pass through the cook untouched.
namespace KRE
{
namespace materials
{
struct sheen_lobe
{
	//	The light pass's whole view of sheen: a free RGB fuzz colour and a roughness
	//	(unpack_principled.h.glsl reads them off the LobeConstants row). Free, not a
	//	baseColor tint: two-tone velvet (pink ground, green pile) is an ordinary fabric,
	//	so the tint/level fold of 2026-08-29 was retired on 2026-08-30. A texture in
	//	either KHR slot cooks to its per-channel AVERAGE (convert_materials.cpp).
	std::array<float, 3> colorFactor{0.f, 0.f, 0.f};
	float                roughnessFactor{0.f};

	//	READ-ONLY: the per-pixel slots an older cook wrote. The loader ignores them,
	//	but a re-cooked old document still references its textures through them, so
	//	the cook's texture GC must see them or those textures vanish. to_json never
	//	writes them and operator== ignores them, so nothing new can carry them on.
	fx::gltf::Material::Texture legacyColorTexture;
	fx::gltf::Material::Texture legacyRoughnessTexture;

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
