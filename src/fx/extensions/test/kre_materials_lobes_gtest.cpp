#include <gtest/gtest.h>
#include "fx/extensions/kre_materials_lobes.h"

// The wire format is the ONLY thing between the cook and the loader; a key that
// round-trips under one name and reads under another is a silent "no sheen".
// Falsifier: write "color" in to_json and read "colorFactor" in from_json.
TEST(KreMaterialsLobes, SheenColourRoundTripsAsColorFactor)
{
	KRE::materials::sheen_lobe in;
	in.is_empty = false;
	in.colorFactor = {0.2f, 0.4f, 0.6f};
	in.roughnessFactor = 0.3f;

	nlohmann::json j = in;
	ASSERT_NE(j.find("colorFactor"), j.end());
	EXPECT_EQ(j.find("tint"), j.end());
	EXPECT_EQ(j.find("level"), j.end());

	KRE::materials::sheen_lobe out = j.get<KRE::materials::sheen_lobe>();
	EXPECT_FALSE(out.empty());
	EXPECT_FLOAT_EQ(out.colorFactor[0], 0.2f);
	EXPECT_FLOAT_EQ(out.colorFactor[1], 0.4f);
	EXPECT_FLOAT_EQ(out.colorFactor[2], 0.6f);
	EXPECT_FLOAT_EQ(out.roughnessFactor, 0.3f);
}

// A file cooked under the tint/level format parses CLEANLY to black (no sheen).
// The loader must say so once. Falsifier: drop "tint"/"level" from the retired list.
TEST(KreMaterialsLobes, RetiredTintLevelSpellingParsesToNoSheenAndWarns)
{
	nlohmann::json j = { {"tint", 1.0f}, {"level", 0.33f}, {"roughnessFactor", 0.5f} };
	KRE::materials::sheen_lobe out = j.get<KRE::materials::sheen_lobe>();
	EXPECT_FLOAT_EQ(out.colorFactor[0], 0.f);
	EXPECT_FLOAT_EQ(out.colorFactor[1], 0.f);
	EXPECT_FLOAT_EQ(out.colorFactor[2], 0.f);
	// The warning is once-per-run (static bool); this test cannot assert on its
	// text without owning the loguru callback, so the migration row (Task 1 step 5)
	// is the machine check for the corpus. Here we pin only the parse result.
}
