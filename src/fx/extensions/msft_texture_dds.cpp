#include "msft_texture_dds.h"
#include "fx/gltf.h"

namespace MSFT
{

void to_json(nlohmann::json & json, texture_dds const& db)
{
	fx::gltf::detail::WriteField("source", json, db.source, -1);
	fx::gltf::detail::WriteField("uncompressedSize", json, db.uncompressedSize, 0u);
}

void from_json(const nlohmann::json & json, texture_dds & db)
{
	fx::gltf::detail::ReadRequiredField("source", json, db.source);
	fx::gltf::detail::ReadOptionalField("uncompressedSize", json, db.uncompressedSize);
}

}

namespace LF
{

static SWIZZLE GetSwizzleValue(char value)
{
	switch(value)
	{
	case '0': return SWIZZLE::zero;
	case '1': return SWIZZLE::one;
	case 'R':
	case 'r': return SWIZZLE::red;
	case 'G':
	case 'g': return SWIZZLE::green;
	case 'B':
	case 'b': return SWIZZLE::blue;
	case 'A':
	case 'a': return SWIZZLE::alpha;
	default:
		throw fx::gltf::invalid_gltf_document("LF_Swizzle.mask must match [01rgbaRGBA]{4}");
	}

	return SWIZZLE::zero;
}

void to_json(nlohmann::json & json, Swizzle const& db)
{
	static const char GetCharValue[6] = {'0', '1', 'r', 'g', 'b', 'a'};

	std::string mask("\0", 4);

	mask[0] = GetCharValue[(int)db.r % 6];
	mask[1] = GetCharValue[(int)db.g % 6];
	mask[2] = GetCharValue[(int)db.b % 6];
	mask[3] = GetCharValue[(int)db.a % 6];

	fx::gltf::detail::WriteField("mask", json, mask);
}

void from_json(const nlohmann::json & json, Swizzle & db)
{
	std::string mask;
	fx::gltf::detail::ReadRequiredField("mask", json, mask);

	if(mask.size() != 4)
		throw fx::gltf::invalid_gltf_document("LF_Swizzle.mask must match [01rgbaRGBA]{4}");

	db.r = GetSwizzleValue(mask[0]);
	db.g = GetSwizzleValue(mask[1]);
	db.b = GetSwizzleValue(mask[2]);
	db.a = GetSwizzleValue(mask[3]);
}

void to_json(nlohmann::json & json, texture_cmp const& db)
{
	fx::gltf::detail::WriteField("bc", json, db.bc, (short)-1);
	fx::gltf::detail::WriteField("swizzle", json, db.swizzle);

}

void from_json(nlohmann::json const& json, texture_cmp  & db)
{
	fx::gltf::detail::ReadRequiredField("bc", json, db.bc);
	fx::gltf::detail::ReadOptionalField("swizzle", json, db.swizzle);
}

};

