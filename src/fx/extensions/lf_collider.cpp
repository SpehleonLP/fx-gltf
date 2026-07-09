#include <fx/gltf.h>
#include "lf_collider.h"

#include "glm_json.hpp"
#include "glm_iostream.hpp"
#include <iomanip>

#define READ(x) detail::ReadOptionalField(#x, json, obj.x)
#define WRITE(x) detail::WriteField(#x, json, obj.x)
#define WRITE2(x, y) detail::WriteField(#x, json, obj.x, y)

namespace detail   = fx::gltf::detail;
namespace defaults = fx::gltf::defaults;

void to_json(nlohmann::json & json, transform_t const& obj)
{
	WRITE2(translation, glm::vec3(0, 0, 0));
	WRITE2(rotation,    glm::quat(1, 0, 0, 0));
	WRITE2(scaling,     glm::vec3(1, 1, 1));
}

void from_json(nlohmann::json const& json, transform_t & obj)
{
    obj = transform_t::Identity;

	READ(translation);
	READ(rotation);
	READ(scaling);
}

std::ostream & operator<< (std::ostream & stream, LF::Collider::Type type)
{
	stream << LF::Collider::GetTypeName(type);
	return stream;
}

std::ostream & operator<< (std::ostream & stream, const LF::Collider & collider)
{
#define PRINT(x) stream << "\t" << std::left << std::setw(20) << #x ":   " << collider.x << std::endl
	PRINT(type);
	PRINT(joint);
	PRINT(local.translation);
	PRINT(local.rotation);
	PRINT(local.scaling);
	PRINT(positions);
	PRINT(indices);
#undef PRINT
	return stream;
}


namespace LF
{

const char * Collider::GetTypeName(Collider::Type type)
{
	switch(type)
	{
	case LF::Collider::Type::None:    return "None";
	case LF::Collider::Type::Sphere:  return "Sphere";
	case LF::Collider::Type::Capsule: return "Capsule";
	case LF::Collider::Type::Cube:    return "Cube";
	case LF::Collider::Type::Convex:  return "Convex";
	default: return "";
	}
}

inline void to_json(nlohmann::json & json, Collider::Type const& colliderType)
{
	switch (colliderType)
	{
		case LF::Collider::Type::Sphere:  json = "SPHERE";  break;
		case LF::Collider::Type::Capsule: json = "CAPSULE"; break;
		case LF::Collider::Type::Cube:    json = "CUBE";    break;
		case LF::Collider::Type::Convex:  json = "CONVEX";  break;
		default: throw fx::gltf::invalid_gltf_document("Unknown ue4Collider.type value");
	}
}

inline void from_json(const nlohmann::json & json, Collider::Type & colliderType)
{
	std::string type = json.get<std::string>();

	     if(type == "SPHERE" ) colliderType = LF::Collider::Type::Sphere;
	else if(type == "CAPSULE") colliderType = LF::Collider::Type::Capsule;
	else if(type == "CUBE"   ) colliderType = LF::Collider::Type::Cube;
	else if(type == "CONVEX" ) colliderType = LF::Collider::Type::Convex;
	else throw fx::gltf::invalid_gltf_document("Unknown ue4Collider.type value", type);
}

void to_json(nlohmann::json & json, Collider const& obj)
{
	WRITE2(type,        LF::Collider::Type::None);
	WRITE2(joint,       -1);
	WRITE2(mesh,        -1);
	WRITE (local);
	WRITE2(positions,   -1);
	WRITE2(indices,     -1);
}

void from_json(const nlohmann::json & json, Collider & obj)
{
	READ(type);
	READ(mesh);
	READ(joint);
	READ(local);
	READ(positions);
	READ(indices);
}

}
