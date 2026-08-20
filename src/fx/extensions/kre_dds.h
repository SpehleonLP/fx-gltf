#ifndef KRE_DDS_H
#define KRE_DDS_H
#include "RHI/rhi_pod.h"
#include <cstdint>
#include <optional>
#include <string_view>

namespace KRE
{

struct texture_dds
{
	int32_t source{-1};
	uint32_t uncompressedSize{};

	bool empty() const { return source == -1; }
	bool operator==(texture_dds const& it) const
	{
		return source == it.source && uncompressedSize == it.uncompressedSize;
	}
};

struct texture_cmp
{
	short                 bc{-1};
	Rhi::ComponentMapping swizzle{};

	bool empty() const { return bc == -1 && swizzle == Rhi::ComponentMapping{}; }
	bool operator==(texture_cmp const& it) const { return bc == it.bc && swizzle == it.swizzle; }
};

// nullopt on anything not a well-formed [01rgbaRGBA]{3,4} mask; shared with the
// gltf sidecar parser so the permutation grammar has one implementation.
std::optional<Rhi::ComponentMapping> ParseMask(std::string_view mask);

}


#endif // KRE_DDS_H
