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

// variants_multiprim.gltf — retires the m2 coverage gap. The earlier
// MaterialsVariants test had a single mesh / single primitive, so the ee
// mesh-major/primitive-minor flatten counter only ever ran at index 0. Here two
// meshes span three primitives: mesh0 has [p0 (no variants), p1 (variants)] and
// mesh1 has [p0 (variants)]. So variant mappings land at FLATTEN INDICES 1 and 2
// (counter > 0), proving the flatten traversal — and its Pack/Unpack round-trip
// invariant — holds past the first primitive of the first mesh.
TEST(EeRoundTrip, MaterialsVariantsMultiPrimitiveFlatten)
{
    fx::gltf::Document doc = LoadFixture("variants_multiprim.gltf");
    fx::ExtensionsAndExtras ee; ee.Unpack(doc);

    // All three primitives flatten (mesh-major/primitive-minor): 0,1,2.
    ASSERT_EQ(ee.primitives.size(), 3u);
    // Flatten index 0 = mesh0.p0 — no variants.
    EXPECT_TRUE(ee.primitives[0].extensions.variantMappings.empty());
    // Flatten index 1 = mesh0.p1 — variants at counter > 0.
    ASSERT_EQ(ee.primitives[1].extensions.variantMappings.size(), 1u);
    EXPECT_EQ(ee.primitives[1].extensions.variantMappings[0].material, 1);
    ASSERT_EQ(ee.primitives[1].extensions.variantMappings[0].variants.size(), 1u);
    EXPECT_EQ(ee.primitives[1].extensions.variantMappings[0].variants[0], 0);
    // Flatten index 2 = mesh1.p0 — variants at counter > 0 (second mesh).
    ASSERT_EQ(ee.primitives[2].extensions.variantMappings.size(), 1u);
    EXPECT_EQ(ee.primitives[2].extensions.variantMappings[0].material, 2);
    EXPECT_EQ(ee.primitives[2].extensions.variantMappings[0].variants[0], 1);

    // Pack rebuilds the identical traversal — each mapping lands back on the same
    // primitive (index N denotes the same primitive on both sides).
    ee.Pack(doc);
    EXPECT_EQ(doc.meshes[0].primitives[0].extensionsAndExtras.count("extensions"), 0u)
        << "no-variant primitive gained a variants blob — flatten misaligned";
    auto& m0p1 = doc.meshes[0].primitives[1]
        .extensionsAndExtras["extensions"]["KHR_materials_variants"]["mappings"];
    ASSERT_EQ(m0p1.size(), 1u);
    EXPECT_EQ(m0p1[0]["material"].get<int>(), 1);
    auto& m1p0 = doc.meshes[1].primitives[0]
        .extensionsAndExtras["extensions"]["KHR_materials_variants"]["mappings"];
    ASSERT_EQ(m1p0.size(), 1u);
    EXPECT_EQ(m1p0[0]["material"].get<int>(), 2);

    // Re-Unpack proves the round-trip is stable at counter > 0.
    fx::ExtensionsAndExtras ee2; ee2.Unpack(doc);
    ASSERT_EQ(ee2.primitives.size(), 3u);
    EXPECT_TRUE(ee2.primitives[0].extensions.variantMappings.empty());
    ASSERT_EQ(ee2.primitives[1].extensions.variantMappings.size(), 1u);
    EXPECT_EQ(ee2.primitives[1].extensions.variantMappings[0].material, 1);
    ASSERT_EQ(ee2.primitives[2].extensions.variantMappings.size(), 1u);
    EXPECT_EQ(ee2.primitives[2].extensions.variantMappings[0].material, 2);
}

// msft_lod.gltf: node 0 ("lod0") carries MSFT_lod.ids=[1] (extension scope) and
// extras.MSFT_screencoverage=[0.5,0.01]; node 1 ("lod1") is the standalone lower
// LOD. Round-trips the typed ee fields and asserts the re-packed JSON blob.
TEST(EeRoundTrip, NodeMsftLodIdsAndScreencoverage)
{
    fx::gltf::Document doc = LoadFixture("msft_lod.gltf");
    fx::ExtensionsAndExtras ee; ee.Unpack(doc);
    ASSERT_EQ(ee.nodes.size(), 2u);

    ASSERT_EQ(ee.nodes[0].extensions.msftLodIds.size(), 1u);
    EXPECT_EQ(ee.nodes[0].extensions.msftLodIds[0], 1);

    ASSERT_EQ(ee.nodes[0].extras.msftScreencoverage.size(), 2u);
    EXPECT_FLOAT_EQ(ee.nodes[0].extras.msftScreencoverage[0], 0.5f);
    EXPECT_FLOAT_EQ(ee.nodes[0].extras.msftScreencoverage[1], 0.01f);

    // Repack → the blob keeps both the extension ids and the extras coverage.
    ee.Pack(doc);
    auto& next = doc.nodes[0].extensionsAndExtras["extensions"];
    ASSERT_EQ(next.count("MSFT_lod"), 1u);
    ASSERT_EQ(next["MSFT_lod"]["ids"].size(), 1u);
    EXPECT_EQ(next["MSFT_lod"]["ids"][0].get<int>(), 1);
    auto& nxtra = doc.nodes[0].extensionsAndExtras["extras"];
    ASSERT_EQ(nxtra.count("MSFT_screencoverage"), 1u);
    EXPECT_FLOAT_EQ(nxtra["MSFT_screencoverage"][0].get<float>(), 0.5f);
    EXPECT_FLOAT_EQ(nxtra["MSFT_screencoverage"][1].get<float>(), 0.01f);

    // Re-Unpack the just-Packed doc — proves a true typed round-trip.
    fx::ExtensionsAndExtras ee2; ee2.Unpack(doc);
    ASSERT_EQ(ee2.nodes.size(), 2u);
    ASSERT_EQ(ee2.nodes[0].extensions.msftLodIds.size(), 1u);
    EXPECT_EQ(ee2.nodes[0].extensions.msftLodIds[0], 1);
    ASSERT_EQ(ee2.nodes[0].extras.msftScreencoverage.size(), 2u);
    EXPECT_FLOAT_EQ(ee2.nodes[0].extras.msftScreencoverage[1], 0.01f);
}

// msft_lod_mesh.gltf: mesh 0 carries MSFT_lod.ids=[1] + extras coverage. This
// fixture is mesh-bearing/buffer-less, so it uses the ee Unpack->Pack->re-Unpack
// path (NOT SaveReloadText, which throws on such fixtures).
TEST(EeRoundTrip, MeshMsftLodIdsAndScreencoverage)
{
    fx::gltf::Document doc = LoadFixture("msft_lod_mesh.gltf");
    fx::ExtensionsAndExtras ee; ee.Unpack(doc);
    ASSERT_EQ(ee.meshes.size(), 2u);

    ASSERT_EQ(ee.meshes[0].extensions.msftLodIds.size(), 1u);
    EXPECT_EQ(ee.meshes[0].extensions.msftLodIds[0], 1);
    ASSERT_EQ(ee.meshes[0].extras.msftScreencoverage.size(), 2u);
    EXPECT_FLOAT_EQ(ee.meshes[0].extras.msftScreencoverage[0], 0.8f);
    EXPECT_FLOAT_EQ(ee.meshes[0].extras.msftScreencoverage[1], 0.02f);

    // Repack → blob keeps the values.
    ee.Pack(doc);
    auto& mext = doc.meshes[0].extensionsAndExtras["extensions"];
    ASSERT_EQ(mext.count("MSFT_lod"), 1u);
    EXPECT_EQ(mext["MSFT_lod"]["ids"][0].get<int>(), 1);
    auto& mxtra = doc.meshes[0].extensionsAndExtras["extras"];
    ASSERT_EQ(mxtra.count("MSFT_screencoverage"), 1u);
    EXPECT_FLOAT_EQ(mxtra["MSFT_screencoverage"][0].get<float>(), 0.8f);

    // Re-Unpack the just-Packed doc — true typed round-trip.
    fx::ExtensionsAndExtras ee2; ee2.Unpack(doc);
    ASSERT_EQ(ee2.meshes.size(), 2u);
    ASSERT_EQ(ee2.meshes[0].extensions.msftLodIds.size(), 1u);
    EXPECT_EQ(ee2.meshes[0].extensions.msftLodIds[0], 1);
    ASSERT_EQ(ee2.meshes[0].extras.msftScreencoverage.size(), 2u);
    EXPECT_FLOAT_EQ(ee2.meshes[0].extras.msftScreencoverage[0], 0.8f);
}

// ibl.gltf: doc-level EXT_lights_image_based.lights (1 entry: 2 mips x 6 faces
// of specularImages, 9 SH irradianceCoefficients, rotation/intensity/
// specularImageSize) + scene EXT_lights_image_based.light index. Exercises the
// promoted Extensions::Scene struct and the nested-array (de)serialization.
TEST(EeRoundTrip, DocLevelIblLightAndSceneIndex)
{
    fx::gltf::Document doc = LoadFixture("ibl.gltf");
    fx::ExtensionsAndExtras ee; ee.Unpack(doc);

    // Doc-level IBL light — never GC-compacted, one entry.
    ASSERT_EQ(ee.document.extensions.iblLights.size(), 1u);
    auto const& L = ee.document.extensions.iblLights[0];
    EXPECT_EQ(L.name, "env0");
    EXPECT_FLOAT_EQ(L.rotation[0], 0.0f);
    EXPECT_FLOAT_EQ(L.rotation[3], 1.0f);
    EXPECT_FLOAT_EQ(L.intensity, 2.0f);
    EXPECT_EQ(L.specularImageSize, 256);

    // 9 SH coefficient triples, exact values.
    for(int i = 0; i < 9; ++i)
    {
        EXPECT_FLOAT_EQ(L.irradianceCoefficients[i][0], (i + 1) * 0.10f);
        EXPECT_FLOAT_EQ(L.irradianceCoefficients[i][1], (i + 1) * 0.10f + 0.01f);
        EXPECT_FLOAT_EQ(L.irradianceCoefficients[i][2], (i + 1) * 0.10f + 0.02f);
    }

    // Nested specularImages: 2 mips x 6 faces.
    ASSERT_EQ(L.specularImages.size(), 2u);
    ASSERT_EQ(L.specularImages[0].size(), 6u);
    ASSERT_EQ(L.specularImages[1].size(), 6u);
    for(int f = 0; f < 6; ++f)
    {
        EXPECT_EQ(L.specularImages[0][f], f);
        EXPECT_EQ(L.specularImages[1][f], 6 + f);
    }

    // Scene index (promoted Extensions::Scene).
    ASSERT_EQ(ee.scenes.size(), 1u);
    EXPECT_EQ(ee.scenes[0].extensions.iblLightIndex, 0);

    // Repack → the blob keeps the nested values.
    ee.Pack(doc);
    auto& jibl = doc.extensionsAndExtras["extensions"]["EXT_lights_image_based"]["lights"];
    ASSERT_EQ(jibl.size(), 1u);
    EXPECT_EQ(jibl[0]["specularImageSize"].get<int>(), 256);
    EXPECT_FLOAT_EQ(jibl[0]["intensity"].get<float>(), 2.0f);
    auto& jsi = jibl[0]["specularImages"];
    ASSERT_EQ(jsi.size(), 2u);
    ASSERT_EQ(jsi[1].size(), 6u);
    EXPECT_EQ(jsi[1][5].get<int>(), 11);
    auto& jsh = jibl[0]["irradianceCoefficients"];
    ASSERT_EQ(jsh.size(), 9u);
    EXPECT_FLOAT_EQ(jsh[8][2].get<float>(), 0.92f);
    EXPECT_EQ(doc.scenes[0].extensionsAndExtras["extensions"]["EXT_lights_image_based"]["light"].get<int>(), 0);

    // Re-Unpack the just-Packed doc — proves a true typed round-trip.
    fx::ExtensionsAndExtras ee2; ee2.Unpack(doc);
    ASSERT_EQ(ee2.document.extensions.iblLights.size(), 1u);
    auto const& L2 = ee2.document.extensions.iblLights[0];
    ASSERT_EQ(L2.specularImages.size(), 2u);
    EXPECT_EQ(L2.specularImages[1][5], 11);
    EXPECT_EQ(L2.specularImageSize, 256);
    EXPECT_FLOAT_EQ(L2.irradianceCoefficients[4][1], 0.51f);
    ASSERT_EQ(ee2.scenes.size(), 1u);
    EXPECT_EQ(ee2.scenes[0].extensions.iblLightIndex, 0);
}

// anim_pointer.gltf: an animation channel whose target carries
// KHR_animation_pointer.pointer = "/materials/0/emissiveFactor" (task 10). The
// pointer lives on channel.target.extensions — outside the ee-macro reach — and
// the fork preserves target.extensionsAndExtras verbatim on Load/Save, so the
// string must survive a full round-trip with NO typed accessor and NO new
// (de)serialization code. Fixture is buffer-less but NOT mesh-bearing, so
// SaveReloadText is safe here.
TEST(EeRoundTrip, AnimationPointerTargetString)
{
    fx::gltf::Document doc = LoadFixture("anim_pointer.gltf");
    ASSERT_EQ(doc.animations.size(), 1u);
    ASSERT_EQ(doc.animations[0].channels.size(), 1u);

    auto ReadPointer = [](fx::gltf::Document const& d) -> std::string {
        auto const& tee = d.animations[0].channels[0].target.extensionsAndExtras;
        auto ext = tee.find("extensions");
        if(ext == tee.end()) return {};
        auto ap = ext->find("KHR_animation_pointer");
        if(ap == ext->end()) return {};
        auto p = ap->find("pointer");
        if(p == ap->end() || !p->is_string()) return {};
        return p->get<std::string>();
    };

    // Load preserves the target-scope blob.
    EXPECT_EQ(ReadPointer(doc), "/materials/0/emissiveFactor");

    // ee Unpack->Pack leaves the bespoke target scope untouched (buffer-less safe).
    fx::ExtensionsAndExtras ee; ee.Unpack(doc); ee.Pack(doc);
    EXPECT_EQ(ReadPointer(doc), "/materials/0/emissiveFactor");

    // Full serialize round-trip through text also preserves it (fixture is not
    // mesh-bearing, so SaveReloadText does not throw).
    fx::gltf::Document rt = SaveReloadText(doc);
    EXPECT_EQ(ReadPointer(rt), "/materials/0/emissiveFactor");
}

//	The names we register as familiar and the names we actually write were
//	allowed to drift apart once already (LF_compression vs LF_Compression), so
//	an unrecognized-extension warning fired on files we authored ourselves.
//	This walks the registered list and asserts every in-house name round-trips.
TEST(ExtensionNames, EveryInHouseNameIsWrittenAndRecognized)
{
	static char const* kInHouse[] = {
		"KRE_animRoot", "KRE_root_motion", "KRE_rintintin",
		"KRE_texture_dds",
	};

	for(auto const* name : kInHouse)
		EXPECT_TRUE(fx::IsFamiliarExtension(name)) << name;

	//	NOT in-house and NOT retired: a third-party standard we read and write on
	//	purpose. It was collateral damage of the rename -- our extension used to
	//	live in msft_texture_dds.cpp, so renaming the file moved the parsed key and
	//	this list filed the standard name with our own dead spellings. A texture
	//	using none of our additions is written under this name precisely so a stock
	//	viewer can open the asset.
	EXPECT_TRUE(fx::IsFamiliarExtension("MSFT_texture_dds"));

	//	The old spellings must be gone, not merely superseded: a tolerated read
	//	is exactly what this branch removed. The KRE_ spellings here were live
	//	names in this same codebase, so a re-added allow-list entry is the
	//	likeliest way they come back.
	static char const* kRetired[] = {
		"LF_animRoot", "LF_root_motion", "LF_RINTINTIN", "LF_colliders",
		"LF_compression", "LF_Compression", "LF_alternate",
		//	LZ4_texture_dds was OURS and stays retired; MSFT_texture_dds is not
		//	ours to retire and is asserted familiar above.
		"LF_swizzle", "LF_Swizzle", "LZ4_texture_dds",
		"KRE_texture_dds_lz4",
		"KRE_colliders", "KRE_compression", "KRE_alternate", "KRE_swizzle",
	};

	for(auto const* name : kRetired)
		EXPECT_FALSE(fx::IsFamiliarExtension(name)) << name;
}

//	The allow-list test above cannot catch a WriteField/ReadOptionalField
//	literal drifting from the allow-list (that is exactly the bug class this
//	rename existed to fix — LF_compression vs LF_Compression). This test goes
//	through the real Pack()/Unpack() path instead: it sets each field via its
//	typed struct, inspects the ACTUAL emitted JSON key, and round-trips it
//	back through Unpack(), so a wrong or stale string literal in either
//	direction shows up here. KRE_animRoot has no typed field anywhere in
//	ExtensionsAndExtras, so there is nothing to round-trip for it; the
//	allow-list test above remains its only coverage.
TEST(EeRoundTrip, InHouseExtensionWireNamesRoundTrip)
{
	fx::gltf::Document doc;
	doc.animations.resize(1);
	doc.textures.resize(1);
	doc.nodes.resize(1);

	fx::ExtensionsAndExtras ee;
	ee.animations.resize(1);
	ee.textures.resize(1);
	ee.nodes.resize(1);

	// KRE_root_motion
	ee.animations[0].extensions.rootMotion.attach_node      = 3;
	ee.animations[0].extensions.rootMotion.translation_mask = 0b101;

	// KRE_texture_dds — non-identity swizzle, so WriteField's default-omit
	// doesn't skip it.
	ee.textures[0].extensions.dds.source  = 0;
	ee.textures[0].extensions.dds.swizzle = Rhi::ComponentMapping{
		Rhi::ComponentSwizzle::G, Rhi::ComponentSwizzle::R,
		Rhi::ComponentSwizzle::B, Rhi::ComponentSwizzle::A };

	// KRE_rintintin
	KRE::RinTinTin::Metrics metrics{};
	metrics.volume = 42.5f;
	ee.nodes[0].extensions.rintintin.metrics.push_back(metrics);

	ee.Pack(doc);

	// --- Assert the REAL emitted JSON keys, not the allow-list. ---
	auto& aext = doc.animations[0].extensionsAndExtras["extensions"];
	ASSERT_EQ(aext.count("KRE_root_motion"), 1u) << aext.dump();
	EXPECT_EQ(aext["KRE_root_motion"]["attach_node"].get<int32_t>(), 3);

	auto& text = doc.textures[0].extensionsAndExtras["extensions"];
	ASSERT_EQ(text.count("KRE_texture_dds"), 1u) << text.dump();
	EXPECT_EQ(text["KRE_texture_dds"]["swizzle"].get<std::string>(), "grba");

	auto& next = doc.nodes[0].extensionsAndExtras["extensions"];
	ASSERT_EQ(next.count("KRE_rintintin"), 1u) << next.dump();
	ASSERT_EQ(next["KRE_rintintin"]["metrics"].size(), 1u);
	EXPECT_FLOAT_EQ(next["KRE_rintintin"]["metrics"][0]["volume"].get<float>(), 42.5f);

	// --- Unpack back into a fresh ee and check the values survive. ---
	fx::ExtensionsAndExtras ee2;
	ee2.Unpack(doc);

	ASSERT_EQ(ee2.animations.size(), 1u);
	EXPECT_EQ(ee2.animations[0].extensions.rootMotion.attach_node, 3);
	EXPECT_EQ(ee2.animations[0].extensions.rootMotion.translation_mask, 0b101);

	ASSERT_EQ(ee2.textures.size(), 1u);
	ASSERT_TRUE(ee2.textures[0].extensions.dds.swizzle.has_value());
	EXPECT_EQ(ee2.textures[0].extensions.dds.swizzle->r, Rhi::ComponentSwizzle::G);
	EXPECT_EQ(ee2.textures[0].extensions.dds.swizzle->g, Rhi::ComponentSwizzle::R);
	EXPECT_EQ(ee2.textures[0].extensions.dds.swizzle->b, Rhi::ComponentSwizzle::B);
	EXPECT_EQ(ee2.textures[0].extensions.dds.swizzle->a, Rhi::ComponentSwizzle::A);

	ASSERT_EQ(ee2.nodes.size(), 1u);
	ASSERT_EQ(ee2.nodes[0].extensions.rintintin.metrics.size(), 1u);
	EXPECT_FLOAT_EQ(ee2.nodes[0].extensions.rintintin.metrics[0].volume, 42.5f);
}

//	An explicitly authored IDENTITY mask is not the same asset as one with no mask:
//	on a BC5 it says "do not apply the loader's two-channel remap", where absence
//	says "do". WriteField-with-default would drop it and silently turn the first
//	into the second, so presence has to survive the wire on its own.
TEST(EeRoundTrip, ExplicitIdentitySwizzleSurvivesTheWire)
{
	fx::gltf::Document doc;
	doc.images.resize(1);
	doc.textures.resize(2);

	fx::ExtensionsAndExtras ee;
	ee.textures.resize(2);

	//	texture 0 authored identity; texture 1 authored nothing at all.
	ee.textures[0].extensions.dds.source  = 0;
	ee.textures[0].extensions.dds.swizzle = Rhi::ComponentMapping{};
	ee.textures[1].extensions.dds.source  = 0;

	ee.Pack(doc);

	auto& t0 = doc.textures[0].extensionsAndExtras["extensions"];
	ASSERT_EQ(t0.count("KRE_texture_dds"), 1u) << t0.dump();
	ASSERT_EQ(t0["KRE_texture_dds"].count("swizzle"), 1u)
		<< "an authored identity must be WRITTEN, not defaulted away: " << t0.dump();
	EXPECT_EQ(t0["KRE_texture_dds"]["swizzle"].get<std::string>(), "rgba");

	//	Texture 1 authored nothing, so it is a plain dds and is written under the
	//	STANDARD name -- which is a sharper statement of the same fact this leg has
	//	always made: MSFT_texture_dds has no swizzle field at all, so an absent mask
	//	cannot come back engaged. Asserting KRE_texture_dds here would now be
	//	asserting that we ask a viewer to require an extension for a texture that
	//	needs nothing from it.
	auto& t1 = doc.textures[1].extensionsAndExtras["extensions"];
	ASSERT_EQ(t1.count("MSFT_texture_dds"), 1u) << t1.dump();
	EXPECT_EQ(t1.count("KRE_texture_dds"), 0u)
		<< "a maskless Raw texture must not drag our extension into the file";
	EXPECT_EQ(t1["MSFT_texture_dds"].count("swizzle"), 0u)
		<< "an absent mask must stay absent, or every legacy asset gains one";

	//	Through the real serialize path, not just Pack/Unpack in memory.
	fx::gltf::Document rt = SaveReloadText(doc);
	fx::ExtensionsAndExtras ee2;
	ee2.Unpack(rt);

	ASSERT_EQ(ee2.textures.size(), 2u);
	ASSERT_TRUE(ee2.textures[0].extensions.dds.swizzle.has_value())
		<< "the authored identity came back as 'no mask'";
	EXPECT_EQ(*ee2.textures[0].extensions.dds.swizzle, Rhi::ComponentMapping{});
	EXPECT_FALSE(ee2.textures[1].extensions.dds.swizzle.has_value());
}

//	An authored identity and a mask that failed to parse must not look the same to
//	the loader: identity is engaged and SUPPRESSES the format-derived remap, so a
//	hand-broken "rgx1" on a BC5 that degraded to identity would upload (R,G,0,1)
//	and read metallic as 0 everywhere -- a third behaviour nobody authored. Absent
//	is the only safe reading of a mask we could not understand.
TEST(EeRoundTrip, MalformedSwizzleLeavesTheMaskUnengaged)
{
	fx::gltf::Document doc;
	doc.images.resize(1);
	doc.textures.resize(1);

	doc.textures[0].extensionsAndExtras["extensions"]["KRE_texture_dds"] =
		nlohmann::json{ {"source", 0}, {"swizzle", "rgx1"} };

	fx::ExtensionsAndExtras ee;
	ee.Unpack(doc);

	ASSERT_EQ(ee.textures.size(), 1u);
	EXPECT_EQ(ee.textures[0].extensions.dds.source, 0);
	EXPECT_FALSE(ee.textures[0].extensions.dds.swizzle.has_value())
		<< "an unparseable mask came back ENGAGED, which suppresses the format remap";
}

//	A third-party asset carries the standard extension, and it is the strict
//	subset of ours: a source and nothing else, which is exactly a Raw, maskless
//	record. The KRE rename retired this reader as collateral -- our extension used
//	to live in msft_texture_dds.cpp, so renaming the file moved the parsed key too.
TEST(EeRoundTrip, StandardExtensionIsRead)
{
	fx::gltf::Document doc;
	doc.images.resize(1);
	doc.textures.resize(1);
	doc.textures[0].extensionsAndExtras["extensions"]["MSFT_texture_dds"]["source"] = 0;

	fx::ExtensionsAndExtras ee;
	ee.Unpack(doc);

	ASSERT_EQ(ee.textures.size(), 1u);
	EXPECT_EQ(ee.textures[0].extensions.dds.source, 0);
	EXPECT_EQ(ee.textures[0].extensions.dds.storage, KRE::DdsStorage::Raw);
	EXPECT_FALSE(ee.textures[0].extensions.dds.swizzle.has_value());
}

//	Out of spec (MSFT_texture_dds is used-not-required, since the core source is
//	the fallback) but real assets do it, and an unfamiliar REQUIRED extension is
//	the one case Unpack throws on -- a hard load failure for a file we can read.
TEST(EeRoundTrip, StandardExtensionInExtensionsRequiredDoesNotThrow)
{
	fx::gltf::Document doc;
	doc.images.resize(1);
	doc.textures.resize(1);
	doc.textures[0].extensionsAndExtras["extensions"]["MSFT_texture_dds"]["source"] = 0;
	doc.extensionsRequired.push_back("MSFT_texture_dds");

	fx::ExtensionsAndExtras ee;
	EXPECT_NO_THROW(ee.Unpack(doc));
}

//	Both present is a hand-edited or double-cooked file. Ours is the strictly
//	richer record -- it can express everything the standard one can -- so it wins,
//	and the conflict is reported rather than resolved in silence.
TEST(EeRoundTrip, KreWinsWhenBothExtensionsArePresent)
{
	fx::gltf::Document doc;
	doc.images.resize(2);
	doc.textures.resize(1);

	auto& ext = doc.textures[0].extensionsAndExtras["extensions"];
	ext["KRE_texture_dds"]["source"]   = 1;
	ext["KRE_texture_dds"]["storage"]  = "image/dds+lz4";
	ext["MSFT_texture_dds"]["source"]  = 0;

	fx::ExtensionsAndExtras ee;
	ee.Unpack(doc);

	ASSERT_EQ(ee.textures.size(), 1u);
	EXPECT_EQ(ee.textures[0].extensions.dds.source, 1)
		<< "the MSFT record overwrote the richer one";
	EXPECT_EQ(ee.textures[0].extensions.dds.storage, KRE::DdsStorage::Lz4)
		<< "the subtype was lost, which is the whole point of keeping ours";
}
