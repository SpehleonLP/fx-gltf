// An embedded base64 buffer must decode to at least its declared byteLength,
// because consumers bound bufferViews against byteLength.
#include "fx/gltf.h"
#include <gtest/gtest.h>
#include <sstream>

namespace
{
auto LoadEmbedded(uint32_t byteLength)
{
	std::istringstream json(
		R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":)" + std::to_string(byteLength) +
		R"(,"uri":"data:application/octet-stream;base64,AAAA"}]})");
	return fx::gltf::LoadFromText(json, "");
}
}

TEST(BufferUri, EmbeddedBufferShorterThanByteLengthIsRefused)
{
	auto exact = LoadEmbedded(3);
	ASSERT_TRUE(exact.has_value());
	EXPECT_EQ(exact->buffers[0].data.size(), 3u);

	EXPECT_FALSE(LoadEmbedded(64).has_value());
}
