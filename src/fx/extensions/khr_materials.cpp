#include "khr_materials.h"

#define _WriteField(x) \
	fx::gltf::detail::WriteField(#x, json, material. x);
#define _WriteFieldF(x, y) \
	fx::gltf::detail::WriteField(#x, json, material. x, y);

#define ReadOptField(x) \
	fx::gltf::detail::ReadOptionalField(#x, json, material. x);

namespace fx { namespace gltf {
void to_json(nlohmann::json & json, Material::Texture const& materialTexture);
void from_json(nlohmann::json const& json, Material::Texture & materialTexture);

//bool operator==(const Material::Texture & a, const Material::Texture & b);
}}

bool KHR::materials::pbrSpecularGlossiness::operator==(const pbrSpecularGlossiness & b) const
{
	return diffuseFactor             == b.diffuseFactor
		&& diffuseTexture            == b.diffuseTexture
		&& specularFactor            == b.specularFactor
		&& glossinessFactor          == b.glossinessFactor
		&& specularGlossinessTexture == b.specularGlossinessTexture;
}

bool KHR::materials::clearcoat::operator==(const clearcoat & b) const
{
	return clearcoatFactor             == b.clearcoatFactor
		&& clearcoatRoughnessFactor    == b.clearcoatRoughnessFactor
		&& clearcoatTexture            == b.clearcoatTexture
		&& clearcoatRoughnessTexture   == b.clearcoatRoughnessTexture
		&& clearcoatNormalTexture	   == b.clearcoatNormalTexture;
}

bool KHR::materials::sheen::operator==(const sheen & b) const
{
	return sheenColorFactor         == b.sheenColorFactor
		&& sheenRoughnessFactor     == b.sheenRoughnessFactor
		&& sheenColorTexture        == b.sheenColorTexture
		&& sheenRoughnessTexture	== b.sheenRoughnessTexture;
}

bool KHR::materials::specular::operator==(const specular & b) const
{
	return factor          == b.factor
		&& colorFactor      == b.colorFactor
		&& texture          == b.texture
		&& colorTexture     == b.colorTexture;
}

bool KHR::Texture::Transform::operator==(const Transform & b) const
{
	return offset       == b.offset
		&& rotation     == b.rotation
		&& scale        == b.scale
		&& texCoord		== b.texCoord;
}


void KHR::materials::from_json(nlohmann::json const& , unlit & material) { material.is_empty = false; }
void KHR::materials::from_json(nlohmann::json const& json, pbrSpecularGlossiness & material)
{
	fx::gltf::detail::ReadOptionalField("diffuseTexture", json, material.diffuseTexture);
	fx::gltf::detail::ReadOptionalField("specularFactor", json, material.specularFactor);
	fx::gltf::detail::ReadOptionalField("glossinessFactor", json, material.glossinessFactor);
	fx::gltf::detail::ReadOptionalField("specularGlossinessTexture", json, material.specularGlossinessTexture);

	material.is_empty = false;
}


// An extension with every field at its default still owns an OBJECT in the
// file: a null-valued key is a schema violation strict loaders reject.
void KHR::materials::to_json(nlohmann::json & json, unlit const& ) { json = nlohmann::json::object(); }
void KHR::materials::to_json(nlohmann::json & json, pbrSpecularGlossiness const& material)
{
	json = nlohmann::json::object();   // see unlit: all-default must still be {}
	_WriteField(diffuseTexture);
	_WriteField(specularFactor);
	_WriteFieldF(glossinessFactor, 1.f);
	_WriteField(specularGlossinessTexture);
}

void KHR::materials::from_json(nlohmann::json const& json, clearcoat & material)
{
	ReadOptField(clearcoatFactor);
	ReadOptField(clearcoatRoughnessFactor);
	ReadOptField(clearcoatTexture);
	ReadOptField(clearcoatRoughnessTexture);
	ReadOptField(clearcoatNormalTexture);

	material.is_empty = false;
}

void KHR::materials::to_json(nlohmann::json & json, clearcoat const& material)
{
	json = nlohmann::json::object();   // see unlit: all-default must still be {}
	_WriteFieldF(clearcoatFactor, 0.f);
	_WriteFieldF(clearcoatRoughnessFactor, 0.f);
	_WriteField(clearcoatTexture);
	_WriteField(clearcoatRoughnessTexture);
	_WriteField(clearcoatNormalTexture);
}

void KHR::materials::from_json(nlohmann::json const& json, sheen & material)
{
	ReadOptField(sheenColorFactor);
	ReadOptField(sheenRoughnessFactor);
	ReadOptField(sheenColorTexture);
	ReadOptField(sheenRoughnessTexture);

	material.is_empty = false;
}

void KHR::materials::to_json(nlohmann::json & json, sheen const& material)
{
	json = nlohmann::json::object();   // see unlit: all-default must still be {}
	_WriteFieldF(sheenColorFactor, fx::gltf::defaults::NullVec3);
	_WriteFieldF(sheenRoughnessFactor, 0.f);
	_WriteField(sheenColorTexture);
	_WriteField(sheenRoughnessTexture);
}

void KHR::materials::from_json(nlohmann::json const& json, specular & material)
{
	// glTF field names ("specularFactor", ...) don't match the C++ member
	// names (factor, ...), so this can't use the _WriteField/ReadOptField
	// macros above (they stringify the member name as the JSON key).
	fx::gltf::detail::ReadOptionalField("specularFactor", json, material.factor);
	fx::gltf::detail::ReadOptionalField("specularColorFactor", json, material.colorFactor);
	fx::gltf::detail::ReadOptionalField("specularTexture", json, material.texture);
	fx::gltf::detail::ReadOptionalField("specularColorTexture", json, material.colorTexture);

	material.is_empty = false;
}

void KHR::materials::to_json(nlohmann::json & json, specular const& material)
{
	json = nlohmann::json::object();   // see unlit: all-default must still be {}
	fx::gltf::detail::WriteField("specularFactor", json, material.factor, 1.f);
	fx::gltf::detail::WriteField("specularColorFactor", json, material.colorFactor, fx::gltf::defaults::IdentityVec3);
	fx::gltf::detail::WriteField("specularTexture", json, material.texture);
	fx::gltf::detail::WriteField("specularColorTexture", json, material.colorTexture);
}

bool KHR::materials::anisotropy::operator==(const anisotropy & b) const
{
	return anisotropyStrength == b.anisotropyStrength
		&& anisotropyRotation == b.anisotropyRotation
		&& anisotropyTexture  == b.anisotropyTexture;
}

bool KHR::materials::iridescence::operator==(const iridescence & b) const
{
	return iridescenceFactor           == b.iridescenceFactor
		&& iridescenceIor              == b.iridescenceIor
		&& iridescenceThicknessMinimum == b.iridescenceThicknessMinimum
		&& iridescenceThicknessMaximum == b.iridescenceThicknessMaximum
		&& iridescenceTexture          == b.iridescenceTexture
		&& iridescenceThicknessTexture == b.iridescenceThicknessTexture;
}

bool KHR::materials::transmission::operator==(const transmission & b) const
{
	return transmissionFactor  == b.transmissionFactor
		&& transmissionTexture == b.transmissionTexture;
}

bool KHR::materials::volume::operator==(const volume & b) const
{
	return thicknessFactor     == b.thicknessFactor
		&& thicknessTexture    == b.thicknessTexture
		&& attenuationDistance == b.attenuationDistance
		&& attenuationColor    == b.attenuationColor;
}

bool KHR::materials::diffuse_transmission::operator==(const diffuse_transmission & b) const
{
	return diffuseTransmissionFactor       == b.diffuseTransmissionFactor
		&& diffuseTransmissionTexture      == b.diffuseTransmissionTexture
		&& diffuseTransmissionColorFactor  == b.diffuseTransmissionColorFactor
		&& diffuseTransmissionColorTexture == b.diffuseTransmissionColorTexture;
}

void KHR::materials::from_json(nlohmann::json const& json, anisotropy & material)
{
	ReadOptField(anisotropyStrength);
	ReadOptField(anisotropyRotation);
	ReadOptField(anisotropyTexture);
	material.is_empty = false;
}

void KHR::materials::to_json(nlohmann::json & json, anisotropy const& material)
{
	json = nlohmann::json::object();   // see unlit: all-default must still be {}
	_WriteFieldF(anisotropyStrength, 0.f);
	_WriteFieldF(anisotropyRotation, 0.f);
	_WriteField(anisotropyTexture);
}

void KHR::materials::from_json(nlohmann::json const& json, iridescence & material)
{
	ReadOptField(iridescenceFactor);
	ReadOptField(iridescenceIor);
	ReadOptField(iridescenceThicknessMinimum);
	ReadOptField(iridescenceThicknessMaximum);
	ReadOptField(iridescenceTexture);
	ReadOptField(iridescenceThicknessTexture);
	material.is_empty = false;
}

void KHR::materials::to_json(nlohmann::json & json, iridescence const& material)
{
	json = nlohmann::json::object();   // see unlit: all-default must still be {}
	_WriteFieldF(iridescenceFactor, 0.f);
	_WriteFieldF(iridescenceIor, 1.3f);
	_WriteFieldF(iridescenceThicknessMinimum, 100.f);
	_WriteFieldF(iridescenceThicknessMaximum, 400.f);
	_WriteField(iridescenceTexture);
	_WriteField(iridescenceThicknessTexture);
}

void KHR::materials::from_json(nlohmann::json const& json, transmission & material)
{
	ReadOptField(transmissionFactor);
	ReadOptField(transmissionTexture);
	material.is_empty = false;
}

void KHR::materials::to_json(nlohmann::json & json, transmission const& material)
{
	json = nlohmann::json::object();   // see unlit: all-default must still be {}
	_WriteFieldF(transmissionFactor, 0.f);
	_WriteField(transmissionTexture);
}

void KHR::materials::from_json(nlohmann::json const& json, volume & material)
{
	ReadOptField(thicknessFactor);
	ReadOptField(thicknessTexture);
	ReadOptField(attenuationDistance);
	ReadOptField(attenuationColor);
	material.is_empty = false;
}

void KHR::materials::to_json(nlohmann::json & json, volume const& material)
{
	json = nlohmann::json::object();   // see unlit: all-default must still be {}
	_WriteFieldF(thicknessFactor, 0.f);
	_WriteField(thicknessTexture);
	// inf == inf, so the default-compare omits the infinite (spec-default) case.
	_WriteFieldF(attenuationDistance, std::numeric_limits<float>::infinity());
	_WriteFieldF(attenuationColor, fx::gltf::defaults::IdentityVec3);
}

void KHR::materials::from_json(nlohmann::json const& json, diffuse_transmission & material)
{
	ReadOptField(diffuseTransmissionFactor);
	ReadOptField(diffuseTransmissionTexture);
	ReadOptField(diffuseTransmissionColorFactor);
	ReadOptField(diffuseTransmissionColorTexture);
	material.is_empty = false;
}

void KHR::materials::to_json(nlohmann::json & json, diffuse_transmission const& material)
{
	json = nlohmann::json::object();   // see unlit: all-default must still be {}
	_WriteFieldF(diffuseTransmissionFactor, 0.f);
	_WriteField(diffuseTransmissionTexture);
	_WriteFieldF(diffuseTransmissionColorFactor, fx::gltf::defaults::IdentityVec3);
	_WriteField(diffuseTransmissionColorTexture);
}

void KHR::Texture::from_json(nlohmann::json const& json, Transform & material)
{
	ReadOptField(offset);
	ReadOptField(rotation);
	ReadOptField(scale);
	ReadOptField(texCoord);
}

void KHR::Texture::to_json(nlohmann::json & json, Transform const& material)
{
	std::array<float, 2> NullVec2{0, 0};
	std::array<float, 2> IdentityVec2{1, 1};

	_WriteFieldF(offset, NullVec2);
	_WriteFieldF(rotation, 0.f);
	_WriteFieldF(scale, IdentityVec2);
	_WriteFieldF(texCoord, -1);
}




#if 0

inline void to_json(nlohmann::json & json, MetaMaterial::Extras const& material)
{
	detail::WriteField("RENDER_ORDER", json, material.RENDER_ORDER, 0.f);
}

inline void from_json(nlohmann::json const& json, MetaMaterial::Extras & material)
{
	detail::ReadOptionalField("RENDER_ORDER", json, material.RENDER_ORDER);

	for(auto i = json.cbegin(); i != json.cend(); ++i)
	{
		std::string key = i.key();

		if(i.key().find("PROJECTOR=") != std::string::npos)
		{
			material.projector = i.key().substr(10);
			break;
		}
	}
}

#endif

