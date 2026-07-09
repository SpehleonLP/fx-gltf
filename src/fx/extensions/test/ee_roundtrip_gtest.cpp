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

TEST(EeRoundTrip, NodeGpuInstancingAccessor)
{
    fx::gltf::Document doc = LoadFixture("instancing.gltf");
    fx::ExtensionsAndExtras ee; ee.Unpack(doc);
    ASSERT_EQ(ee.nodes.size(), 1u);
    EXPECT_EQ(ee.nodes[0].extensions.gpuInstancing.attributes["TRANSLATION"], 0);

    // repack → the blob keeps the values
    ee.Pack(doc);
    auto& next = doc.nodes[0].extensionsAndExtras["extensions"];
    ASSERT_EQ(next.count("EXT_mesh_gpu_instancing"), 1u);
    EXPECT_EQ(next["EXT_mesh_gpu_instancing"]["attributes"]["TRANSLATION"].get<int>(), 0);
}

TEST(EeRoundTrip, DocLevelLightsAndNodeLightIndex)
{
    fx::gltf::Document doc = LoadFixture("lights_punctual.gltf");
    fx::ExtensionsAndExtras ee; ee.Unpack(doc);

    ASSERT_EQ(ee.document.extensions.punctualLights.size(), 2u);

    auto const& point = ee.document.extensions.punctualLights[0];
    EXPECT_EQ(point.type, "point");
    EXPECT_FALSE(point.isSpot);
    EXPECT_FLOAT_EQ(point.color[0], 1.0f);
    EXPECT_FLOAT_EQ(point.color[1], 0.5f);
    EXPECT_FLOAT_EQ(point.color[2], 0.25f);
    EXPECT_FLOAT_EQ(point.intensity, 10.0f);
    EXPECT_EQ(point.name, "PointLight");

    auto const& spot = ee.document.extensions.punctualLights[1];
    EXPECT_EQ(spot.type, "spot");
    EXPECT_TRUE(spot.isSpot);
    EXPECT_FLOAT_EQ(spot.intensity, 5.0f);
    EXPECT_FLOAT_EQ(spot.spotInner, 0.1f);
    EXPECT_FLOAT_EQ(spot.spotOuter, 0.5f);

    ASSERT_EQ(ee.nodes.size(), 2u);
    EXPECT_EQ(ee.nodes[0].extensions.lightIndex, 0);
    EXPECT_EQ(ee.nodes[1].extensions.lightIndex, 1);

    // repack → the blob keeps the values
    ee.Pack(doc);
    auto& lights = doc.extensionsAndExtras["extensions"]["KHR_lights_punctual"]["lights"];
    ASSERT_EQ(lights.size(), 2u);
    EXPECT_EQ(lights[0]["type"].get<std::string>(), "point");
    EXPECT_EQ(lights[1]["spot"]["innerConeAngle"].get<float>(), 0.1f);

    EXPECT_EQ(doc.nodes[0].extensionsAndExtras["extensions"]["KHR_lights_punctual"]["light"].get<int>(), 0);
    EXPECT_EQ(doc.nodes[1].extensionsAndExtras["extensions"]["KHR_lights_punctual"]["light"].get<int>(), 1);
}

TEST(EeRoundTrip, MaterialsVariants)
{
    fx::gltf::Document doc = LoadFixture("variants.gltf");
    fx::ExtensionsAndExtras ee; ee.Unpack(doc);

    // Doc-level variant names (index-stable, never GC-compacted).
    ASSERT_EQ(ee.document.extensions.variantNames.size(), 2u);
    EXPECT_EQ(ee.document.extensions.variantNames[0], "wet");
    EXPECT_EQ(ee.document.extensions.variantNames[1], "dry");

    // The one mesh primitive flattens to index 0 (mesh-major/primitive-minor).
    ASSERT_EQ(ee.primitives.size(), 1u);
    auto& vm = ee.primitives[0].extensions.variantMappings;
    ASSERT_EQ(vm.size(), 2u);
    EXPECT_EQ(vm[0].material, 0);
    ASSERT_EQ(vm[0].variants.size(), 1u);
    EXPECT_EQ(vm[0].variants[0], 0);
    EXPECT_EQ(vm[1].material, 1);
    ASSERT_EQ(vm[1].variants.size(), 1u);
    EXPECT_EQ(vm[1].variants[0], 1);

    // repack → the blobs keep the values
    ee.Pack(doc);
    auto& dvar = doc.extensionsAndExtras["extensions"]["KHR_materials_variants"]["variants"];
    ASSERT_EQ(dvar.size(), 2u);
    EXPECT_EQ(dvar[0]["name"].get<std::string>(), "wet");
    EXPECT_EQ(dvar[1]["name"].get<std::string>(), "dry");

    auto& pmap = doc.meshes[0].primitives[0]
        .extensionsAndExtras["extensions"]["KHR_materials_variants"]["mappings"];
    ASSERT_EQ(pmap.size(), 2u);
    EXPECT_EQ(pmap[1]["material"].get<int>(), 1);
    EXPECT_EQ(pmap[1]["variants"][0].get<int>(), 1);

    // Re-Unpack the just-Packed doc to prove the flatten invariant is a true
    // round-trip (Pack and Unpack traverse mesh-major/primitive-minor identically).
    fx::ExtensionsAndExtras ee2; ee2.Unpack(doc);
    ASSERT_EQ(ee2.document.extensions.variantNames.size(), 2u);
    ASSERT_EQ(ee2.primitives.size(), 1u);
    ASSERT_EQ(ee2.primitives[0].extensions.variantMappings.size(), 2u);
    EXPECT_EQ(ee2.primitives[0].extensions.variantMappings[1].material, 1);
}
