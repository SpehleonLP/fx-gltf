#include "kre_dds.h"
#include "fx/gltf.h"
#include <optional>
#include <string_view>

namespace KRE
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

namespace {

//	One char per output channel, in r,g,b,a order. Upper and lower case are the
//	same source channel: the mask is a permutation, not a case-sensitive token,
//	and authored files use both spellings.
char SwizzleToChar(Rhi::ComponentSwizzle s)
{
	switch(s)
	{
	case Rhi::ComponentSwizzle::Zero: return '0';
	case Rhi::ComponentSwizzle::One:  return '1';
	case Rhi::ComponentSwizzle::R:    return 'r';
	case Rhi::ComponentSwizzle::G:    return 'g';
	case Rhi::ComponentSwizzle::B:    return 'b';
	case Rhi::ComponentSwizzle::A:    return 'a';
	}
	return 'r';
}

//	Returns false for anything outside the alphabet so the caller can report the
//	offending mask rather than silently substituting a channel.
bool CharToSwizzle(char c, Rhi::ComponentSwizzle& out)
{
	switch(c)
	{
	case '0':           out = Rhi::ComponentSwizzle::Zero; return true;
	case '1':           out = Rhi::ComponentSwizzle::One;  return true;
	case 'r': case 'R': out = Rhi::ComponentSwizzle::R;    return true;
	case 'g': case 'G': out = Rhi::ComponentSwizzle::G;    return true;
	case 'b': case 'B': out = Rhi::ComponentSwizzle::B;    return true;
	case 'a': case 'A': out = Rhi::ComponentSwizzle::A;    return true;
	default: return false;
	}
}

}  // namespace

namespace KRE
{

//	Exposed rather than folded into Rhi::from_json: the sidecar parser needs the
//	same spelling with a real error on failure, and a second implementation of a
//	permutation parser is exactly how two spellings drift apart.
std::optional<Rhi::ComponentMapping> ParseMask(std::string_view mask)
{
	std::string s(mask);

	//	3 chars means opaque: alpha is not stored, so it reads as 1.
	if(s.size() == 3)
		s.push_back('1');

	if(s.size() != 4)
		return std::nullopt;

	Rhi::ComponentMapping out;
	if(!CharToSwizzle(s[0], out.r) || !CharToSwizzle(s[1], out.g)
	|| !CharToSwizzle(s[2], out.b) || !CharToSwizzle(s[3], out.a))
		return std::nullopt;

	return out;
}

}  // namespace KRE

// to_json/from_json for Rhi::ComponentMapping live in Rhi's own namespace -- ADL
// resolves them from that namespace, not from KRE, since ComponentMapping is a
// Rhi type. KRE::texture_cmp::swizzle serializes through these transitively.
namespace Rhi
{

void to_json(nlohmann::json& json, ComponentMapping const& db)
{
	char buf[5] = { SwizzleToChar(db.r), SwizzleToChar(db.g),
	                SwizzleToChar(db.b), SwizzleToChar(db.a), 0 };
	json = std::string(buf);
}

void from_json(nlohmann::json const& json, ComponentMapping& db)
{
	//	A malformed mask keeps the identity rather than guessing which channels
	//	the author meant. The repackager rejects the file before it can be
	//	written this way; reaching here means a hand-edited asset.
	db = KRE::ParseMask(json.get<std::string>()).value_or(ComponentMapping{});
}

}  // namespace Rhi

namespace KRE
{

void to_json(nlohmann::json & json, texture_cmp const& db)
{
	fx::gltf::detail::WriteField("bc", json, db.bc, (short)-1);
	fx::gltf::detail::WriteField("swizzle", json, db.swizzle, Rhi::ComponentMapping{});

}

void from_json(nlohmann::json const& json, texture_cmp  & db)
{
	fx::gltf::detail::ReadRequiredField("bc", json, db.bc);
	fx::gltf::detail::ReadOptionalField("swizzle", json, db.swizzle);
}

};
