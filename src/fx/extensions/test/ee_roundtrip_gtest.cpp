#include <gtest/gtest.h>
#include <fx/gltf.h>
#include <fx/extensions/extensionsandextras.h>
#include <string>

#include <sstream>

static std::string FixturePath(const char* name)
{
    return std::string(GLTF_EXTENSION_FIXTURES_ROOT) + "/" + name;
}

// Shared helpers — used by EVERY later test. LoadFromText/LoadFromBinary return
// tl::expected<Document, JsonError>, so unwrap with .value() (throws on error).
static fx::gltf::Document LoadFixture(const char* name)
{
    return fx::gltf::LoadFromText(FixturePath(name)).value();
}

// True serialize round-trip through text, entirely in memory (ostream Save +
// istream LoadFromText, both exist in the fork). rootPath "" = no external files.
static fx::gltf::Document SaveReloadText(fx::gltf::Document const& doc)
{
    std::ostringstream out;
    auto err = fx::gltf::Save(doc, out, /*documentRootPath*/ "", /*useBinaryFormat*/ false);
    EXPECT_FALSE(err.has_value()) << "Save failed";
    std::istringstream in(out.str());
    return fx::gltf::LoadFromText(in, /*documentRootPath*/ "").value();
}

TEST(EeRoundTrip, MinimalFixtureLoads)
{
    fx::gltf::Document doc = LoadFixture("minimal.gltf");
    ASSERT_EQ(doc.nodes.size(), 1u);
    EXPECT_EQ(doc.nodes[0].name, "root");

    fx::ExtensionsAndExtras ee;
    ee.Unpack(doc);
    ee.Pack(doc);
    fx::gltf::Document rt = SaveReloadText(doc);   // exercise the real serialize path
    EXPECT_EQ(rt.nodes.size(), 1u);
}
