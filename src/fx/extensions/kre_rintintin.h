#ifndef KRE_RINTINTIN_H
#define KRE_RINTINTIN_H
#include <array>
#include <vector>

namespace KRE
{

struct Transform
{
	// Match the omission sentinels in kre_rintintin.cpp's WriteOptField calls --
	// an omitted field must round-trip to this default, not to garbage.
	std::array<float, 4> rotation{0, 0, 0, 1};
	std::array<float, 3> translation{0, 0, 0};
	std::array<float, 3> scaling{1, 1, 1};

	// Disagrees with the sentinel above -- {1,0,0,0} here, {0,0,0,1} there --
	// so a default-constructed Transform is not empty(). Left alone: changing
	// it would change cooked bytes.
	inline bool empty() const
	{ 
		return scaling == std::array<float, 3>{1.f, 1.f, 1.f} 
		&& translation == std::array<float, 3>{0.f, 0.f, 0.f} 
		&& rotation == std::array<float, 4>{1.f, 0.f, 0.f, 0.f}; 
	} 
};

// gets added to nodes with both a skin and a mesh
struct RinTinTin
{
	struct Eigen
	{
		std::array<float, 4> rotation;
		std::array<float, 3> lambda; // min, mid, max ordering

		inline bool empty() const { return false; }
	};

	struct Metrics
	{
		float volume{};
		float surfaceArea{};

		std::array<float, 3> centroid{0};
		std::array<float, 3> min{0}, max{0};	 // AABB of affected verticies
		// Second moment tensor about the centroid: integral of (x-c)(x-c)^T dV.
		// Layout: xx yy zz xy xz yz. Units m^5 (pre-multiplied by volume, unit density).
		// Inertia tensor recovered as: I = trace(M)*Id - M (off-diagonals negated).
		std::array<float, 6> secondMoment{0};

		inline bool empty() const { return false; }
	};

	std::vector<Metrics> metrics;
	std::vector<Eigen> eigenDecompositions;
	std::vector<Transform> orientedBoundedBoxes;
	
	inline bool empty() const { return metrics.empty() && eigenDecompositions.empty() && orientedBoundedBoxes.empty(); } 
};

}

#endif // KRE_RINTINTIN_H
