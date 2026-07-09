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

TEST(EeRoundTrip, MaterialScalarsAndVisibility)
{
    fx::gltf::Document doc = LoadFixture("mat_scalars.gltf");
    fx::ExtensionsAndExtras ee; ee.Unpack(doc);
    ASSERT_EQ(ee.materials.size(), 1u);
    EXPECT_FLOAT_EQ(ee.materials[0].extensions.emissiveStrength, 4.0f);
    EXPECT_FLOAT_EQ(ee.materials[0].extensions.ior, 1.4f);
    ASSERT_EQ(ee.nodes.size(), 1u);
    EXPECT_FALSE(ee.nodes[0].extensions.visible);
    // repack → the blob keeps the values
    ee.Pack(doc);
    auto& mext = doc.materials[0].extensionsAndExtras["extensions"];
    EXPECT_FLOAT_EQ(mext["KHR_materials_emissive_strength"]["emissiveStrength"].get<float>(), 4.0f);
    EXPECT_FALSE(doc.nodes[0].extensionsAndExtras["extensions"]["KHR_node_visibility"]["visible"].get<bool>());
}

TEST(EeRoundTrip, MaterialSpecularTextures)
{
    fx::gltf::Document doc = LoadFixture("specular.gltf");
    fx::ExtensionsAndExtras ee; ee.Unpack(doc);
    ASSERT_EQ(ee.materials.size(), 1u);

    auto& sp = ee.materials[0].extensions.specular;
    EXPECT_FALSE(sp.empty());
    EXPECT_FLOAT_EQ(sp.factor, 0.5f);
    EXPECT_FLOAT_EQ(sp.colorFactor[0], 0.1f);
    EXPECT_FLOAT_EQ(sp.colorFactor[1], 0.2f);
    EXPECT_FLOAT_EQ(sp.colorFactor[2], 0.3f);
    EXPECT_EQ(sp.texture.index, 0);
    EXPECT_EQ(sp.colorTexture.index, 1);

    // repack → the blob keeps the values
    ee.Pack(doc);
    auto& mext = doc.materials[0].extensionsAndExtras["extensions"];
    ASSERT_EQ(mext.count("KHR_materials_specular"), 1u);
    auto& j = mext["KHR_materials_specular"];
    EXPECT_FLOAT_EQ(j["specularFactor"].get<float>(), 0.5f);
    EXPECT_EQ(j["specularTexture"]["index"].get<int>(), 0);
    EXPECT_EQ(j["specularColorTexture"]["index"].get<int>(), 1);
}
