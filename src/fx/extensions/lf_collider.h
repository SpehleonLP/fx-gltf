#ifndef UE4COLLIIDERS_H
#define UE4COLLIIDERS_H
#include "gltf/gltftransform.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <iomanip>

class RepackDocument;

namespace LF
{

struct Collider
{
	enum class Type : int8_t
	{
		None = -1,
		Sphere,
		Capsule,
		Cube,
		Convex
	};

	static const char * GetTypeName(LF::Collider::Type type);

	Type type{Type::None};
//enforce only one joint so we can premultiply by inverse bind matrix
	int32_t mesh{-1};
	int32_t joint{-1};
	float   radius{};

        transform_t local; // where we are relative to the closest armature node

//convex
	int32_t positions{-1};
	int32_t indices{-1};

	bool empty() const { return false; }

	bool operator==(const LF::Collider & it) const;
};

}

#endif // UE4COLLIIDERS_H
