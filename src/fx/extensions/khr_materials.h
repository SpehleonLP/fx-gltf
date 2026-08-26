#ifndef KHR_MATERIALS_H
#define KHR_MATERIALS_H
#include <fx/gltf.h>
#include <limits>

#define KHR_SHEEN 1

namespace KHR
{
namespace materials
{
typedef fx::gltf::Material::Texture Texture;
typedef fx::gltf::Material::NormalTexture NormalTexture;
	struct pbrSpecularGlossiness
	{
		std::array<float, 4> diffuseFactor = { fx::gltf::defaults::IdentityVec4 };
		Texture              diffuseTexture;
		std::array<float, 3> specularFactor = {fx::gltf::defaults::IdentityVec3 };
		float                glossinessFactor{1.f};
		Texture              specularGlossinessTexture;

		bool                 is_empty{true};

		bool empty() const noexcept { return is_empty; }

		bool operator==(const KHR::materials::pbrSpecularGlossiness & b) const;
	};

	struct unlit
	{
		bool                 is_empty{true};
		bool empty() const noexcept { return is_empty; }
		
		bool operator==(unlit const& it) const { return is_empty == it.is_empty; }
	};

	struct clearcoat
	{
		float			clearcoatFactor{0};
		float			clearcoatRoughnessFactor{0};
		Texture			clearcoatTexture;
		Texture			clearcoatRoughnessTexture;
		NormalTexture	clearcoatNormalTexture;

		bool            is_empty{true};

		bool empty() const noexcept { return is_empty; }

		bool operator==(const KHR::materials::clearcoat & b) const;
	};

	struct sheen
	{
		std::array<float, 3> sheenColorFactor{0, 0, 0};
		float				 sheenRoughnessFactor{0};
		Texture				 sheenColorTexture;
		Texture				 sheenRoughnessTexture;

		bool                 is_empty{true};

		bool empty() const noexcept { return is_empty; }

		bool operator==(const KHR::materials::sheen & b) const;
	};

	void from_json(nlohmann::json const& json, unlit & material);
	void to_json(nlohmann::json & json, unlit const& material);

	void from_json(nlohmann::json const& json, pbrSpecularGlossiness & material);
	void to_json(nlohmann::json & json, pbrSpecularGlossiness const& material);

	void from_json(nlohmann::json const& json, clearcoat & material);
	void to_json(nlohmann::json & json, clearcoat const& material);

	void from_json(nlohmann::json const& json, sheen & material);
	void to_json(nlohmann::json & json, sheen const& material);

	// KHR_materials_specular — first index-bearing extension in the yes-list
	// (task 4). factor/colorFactor scale the dielectric specular response;
	// texture/colorTexture reference doc.textures like the core material
	// texture slots. GC must mark+remap these like any other texture index.
	struct specular
	{
		float                 factor{1.f};
		std::array<float, 3>  colorFactor{1.f, 1.f, 1.f};
		Texture               texture;
		Texture               colorTexture;

		bool                  is_empty{true};

		bool empty() const noexcept { return is_empty; }

		bool operator==(const KHR::materials::specular & b) const;
	};

	void from_json(nlohmann::json const& json, specular & material);
	void to_json(nlohmann::json & json, specular const& material);

	// Round-trip only (no engine semantics yet): these exist so the cook
	// preserves the extension instead of scrubbing it as unfamiliar, and so GC
	// keeps their texture indices live. Field names/defaults follow the KHR specs.
	struct anisotropy
	{
		float    anisotropyStrength{0.f};
		float    anisotropyRotation{0.f};
		Texture  anisotropyTexture;

		bool     is_empty{true};
		bool empty() const noexcept { return is_empty; }
		bool operator==(const anisotropy & b) const;
	};

	struct iridescence
	{
		float    iridescenceFactor{0.f};
		float    iridescenceIor{1.3f};
		float    iridescenceThicknessMinimum{100.f};
		float    iridescenceThicknessMaximum{400.f};
		Texture  iridescenceTexture;
		Texture  iridescenceThicknessTexture;

		bool     is_empty{true};
		bool empty() const noexcept { return is_empty; }
		bool operator==(const iridescence & b) const;
	};

	struct transmission
	{
		float    transmissionFactor{0.f};
		Texture  transmissionTexture;

		bool     is_empty{true};
		bool empty() const noexcept { return is_empty; }
		bool operator==(const transmission & b) const;
	};

	struct volume
	{
		float                thicknessFactor{0.f};
		Texture              thicknessTexture;
		// Spec default is +infinity (no attenuation); absent on read means
		// infinite and infinite on write is omitted.
		float                attenuationDistance{std::numeric_limits<float>::infinity()};
		std::array<float, 3> attenuationColor{1.f, 1.f, 1.f};

		bool                 is_empty{true};
		bool empty() const noexcept { return is_empty; }
		bool operator==(const volume & b) const;
	};

	struct diffuse_transmission
	{
		float                diffuseTransmissionFactor{0.f};
		Texture              diffuseTransmissionTexture;
		std::array<float, 3> diffuseTransmissionColorFactor{1.f, 1.f, 1.f};
		Texture              diffuseTransmissionColorTexture;

		bool                 is_empty{true};
		bool empty() const noexcept { return is_empty; }
		bool operator==(const diffuse_transmission & b) const;
	};

	void from_json(nlohmann::json const& json, anisotropy & material);
	void to_json(nlohmann::json & json, anisotropy const& material);
	void from_json(nlohmann::json const& json, iridescence & material);
	void to_json(nlohmann::json & json, iridescence const& material);
	void from_json(nlohmann::json const& json, transmission & material);
	void to_json(nlohmann::json & json, transmission const& material);
	void from_json(nlohmann::json const& json, volume & material);
	void to_json(nlohmann::json & json, volume const& material);
	void from_json(nlohmann::json const& json, diffuse_transmission & material);
	void to_json(nlohmann::json & json, diffuse_transmission const& material);
}

namespace Texture
{
struct Transform
{
	std::array<float, 2> offset{0, 0};
	float				 rotation{};
	std::array<float, 2> scale{1, 1};
	int32_t				 texCoord{-1};

	bool empty() const noexcept
	{
		return offset == std::array<float, 2>({0, 0})
			&& rotation == 0
			&& scale == std::array<float, 2>({1, 1})
			&& texCoord == -1; }

	bool operator==(const KHR::Texture::Transform & b) const;
};

void from_json(nlohmann::json const& json, Transform & material);
void to_json(nlohmann::json & json, Transform const& material);

}

};

#endif // KHR_MATERIALS_H
