#ifndef KRE_DDS_H
#define KRE_DDS_H
#include "RHI/rhi_pod.h"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <optional>
#include <string_view>

namespace KRE
{

// Chosen by measured size, not authored -- decoding is byte-identical either way.
enum class DdsStorage : uint8_t { Raw, Lz4, DeinterleavedLz4 };

// Shared by images[].mimeType and texture_dds's "storage" field. nullopt = not
// a KRE dds subtype (e.g. a PNG mimeType).
char const* MimeForStorage(DdsStorage);
std::optional<DdsStorage> StorageFromMime(std::string_view mime);

// MSFT_texture_dds's README names the referenced image's mimeType with its own
// spelling, not ours. We emit it for textures we downgrade, so we must read it
// back -- without this the round-trip through our own output loses the storage.
constexpr char const* kMsftDdsMime = "image/vnd-ms.dds";

struct texture_dds
{
	int32_t                source{-1};
	uint32_t                uncompressedSize{};
	DdsStorage              storage{DdsStorage::Raw};
	//	Per-texture channel permutation of the STORED image: which stored channel
	//	each output channel reads. Engaged means the cook authored one -- INCLUDING
	//	an explicit identity, which says "do not apply the loader's format-derived
	//	two-channel remap" and is a different instruction from absence, where that
	//	remap is exactly what the asset was cooked to expect. Value alone cannot
	//	carry that distinction, so presence is carried separately.
	std::optional<Rhi::ComponentMapping> swizzle;

	bool empty() const { return source == -1; }

	//	MSFT_texture_dds is a strict subset of this record: a source index and
	//	nothing else. A texture using none of our additions IS one, and is written
	//	as one so a stock viewer can open the asset without being asked to require
	//	an extension it has never heard of. BC block compression inside the DDS is
	//	deliberately NOT a disqualifier -- carrying BC is what DDS is for.
	bool IsPlainDds() const
	{
		return source >= 0 && storage == DdsStorage::Raw && !swizzle.has_value();
	}
	bool operator==(texture_dds const& it) const
	{
		return source == it.source && uncompressedSize == it.uncompressedSize
		    && storage == it.storage && swizzle == it.swizzle;
	}
};

// Declared here (defined in kre_dds.cpp) so any TU that names texture_dds can
// nlohmann::json::get<>/assign it via ADL. Previously only forward-declared
// privately inside extensionsandextras.cpp, which worked for THAT TU alone and
// left a static_assert trap for the next one -- gltf_output.cpp hit it.
void to_json(nlohmann::json & json, texture_dds const& db);
void from_json(nlohmann::json const& json, texture_dds & db);

// nullopt on anything not a well-formed [01rgbaRGBA]{3,4} mask; shared with the
// gltf sidecar parser so the permutation grammar has one implementation.
std::optional<Rhi::ComponentMapping> ParseMask(std::string_view mask);

}


#endif // KRE_DDS_H
