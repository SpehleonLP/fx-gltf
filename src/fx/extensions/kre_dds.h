#ifndef KRE_DDS_H
#define KRE_DDS_H
#include "RHI/rhi_pod.h"
#include <cstdint>

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


}


#endif // KRE_DDS_H
