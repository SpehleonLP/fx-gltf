#ifndef KRE_RINTINTIN_H
#define KRE_RINTINTIN_H
#include <array>
#include <vector>

namespace KRE
{

struct Transform
{
	std::array<float, 4> rotation;
	std::array<float, 3> translation;
	std::array<float, 3> scaling;
	
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
		std::array<float, 3> min, max{0};	 // AABB of affected verticies
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
