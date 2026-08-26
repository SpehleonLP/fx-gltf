#ifndef KRE_MATERIALS_LOBES_H
#define KRE_MATERIALS_LOBES_H
#include <fx/gltf.h>
#include <array>
#include <cstdint>

// The cooked form of sheen and subsurface. Source KHR extensions (sheen,
// diffuse_transmission, transmission+volume) are rewritten into this at cook
// and REMOVED from the material; the loader reads lobes from here only.
// See docs/superpowers/specs/2026-08-25-material-extension-cook-design.md.
namespace KRE
{
namespace materials
{
typedef fx::gltf::Material::Texture Texture;

struct sheen_lobe
{
	std::array<float, 3> colorFactor{0.f, 0.f, 0.f};
	float                roughnessFactor{0.f};
	Texture              colorTexture;
	Texture              roughnessTexture;   // roughness is read from .a, as KHR authors it

	bool is_empty{true};
	bool empty() const noexcept { return is_empty; }
	bool operator==(sheen_lobe const& b) const;
};

// Which KHR extension fed this lobe: only the loader's warning text cares,
// but the file must say so or the warning cannot.
enum class SubsurfaceSource : uint8_t { DiffuseTransmission, Transmission };

struct subsurface_lobe
{
	float                weightFactor{0.f};          // 1-weight scales the front diffuse
	std::array<float, 3> colorFactor{1.f, 1.f, 1.f};
	float                roughnessFactor{1.f};       // radius == transmitted-lobe roughness
	Texture              weightTexture;              // .r
	Texture              colorTexture;               // .rgb
	Texture              roughnessTexture;           // .g (volume thickness) -- absent for diffuse_transmission
	SubsurfaceSource     source{SubsurfaceSource::DiffuseTransmission};

	bool is_empty{true};
	bool empty() const noexcept { return is_empty; }
	bool operator==(subsurface_lobe const& b) const;
};

struct lobes
{
	sheen_lobe      sheen;
	subsurface_lobe subsurface;

	bool empty() const noexcept { return sheen.empty() && subsurface.empty(); }
	bool operator==(lobes const& b) const { return sheen == b.sheen && subsurface == b.subsurface; }
};

char const* ToString(SubsurfaceSource);
SubsurfaceSource SubsurfaceSourceFromString(std::string_view);

void from_json(nlohmann::json const& json, sheen_lobe & lobe);
void to_json(nlohmann::json & json, sheen_lobe const& lobe);
void from_json(nlohmann::json const& json, subsurface_lobe & lobe);
void to_json(nlohmann::json & json, subsurface_lobe const& lobe);
void from_json(nlohmann::json const& json, lobes & lobe);
void to_json(nlohmann::json & json, lobes const& lobe);
}
}

#endif // KRE_MATERIALS_LOBES_H
