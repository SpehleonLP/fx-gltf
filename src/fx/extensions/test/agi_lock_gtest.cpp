#include <gtest/gtest.h>
#include "fx/extensions/agi_articulation.h"

// The cook runs this on every node name of a file with articulations; the frozen parse never
// advanced past a T/R/S group and hung on these. Run this suite under `timeout`.
TEST(AgiLock, ParsesEachGroupOnce)
{
	EXPECT_EQ(int(AGI::AGI_LockFromString("LOCK_Txyz_Sxyz:hair_root")), 455);
	EXPECT_EQ(int(AGI::AGI_LockFromString("LOCK_Rz")), 32);
	EXPECT_EQ(int(AGI::AGI_LockFromString("LOCK_Tx:Ry")), 1) << "only the part before ':' is the lock";
}

TEST(AgiLock, UIsUniformScaleInTheScaleGroupOnly)
{
	EXPECT_EQ(int(AGI::AGI_LockFromString("LOCK_Su")), 512);
	EXPECT_EQ(int(AGI::AGI_LockFromString("LOCK_Tu")), 0) << "u means nothing to T";
}

TEST(AgiLock, AllAndNothing)
{
	EXPECT_EQ(int(AGI::AGI_LockFromString("LOCK_all")), 1023);
	EXPECT_EQ(int(AGI::AGI_LockFromString("LOCK_Tx_all")), 1023) << "all is a whole token, wherever it sits";
	EXPECT_EQ(int(AGI::AGI_LockFromString("LOCK_Tx_ball")), 1) << "all inside another token is not All";
	EXPECT_EQ(int(AGI::AGI_LockFromString("LOCK_ALL")), 0) << "case-sensitive, like the axis letters' groups";
	EXPECT_EQ(int(AGI::AGI_LockFromString("LOCK")), 0) << "frozen returned All here: npos tested true";
	EXPECT_EQ(int(AGI::AGI_LockFromString("hair_root")), 0);
	EXPECT_EQ(int(AGI::AGI_LockFromString("")), 0);
	EXPECT_EQ(int(AGI::AGI_LockFromString(std::string_view("LOCK_Txyz", 7))), 1) << "never reads past the view: \"LOCK_Tx\" is x only; an over-read gives 7";
	EXPECT_EQ(int(AGI::AGI_LockFromString(std::string_view("LOCK_Txyz", 6))), 0) << "a bare T locks nothing";
}
