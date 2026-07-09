#include "agi_articulation.h"
#include "Support/unsafe_view.hpp"
#include <fx/gltf.h>

#define READ(x) fx::gltf::detail::ReadOptionalField(#x, json, obj.x)
#define WRITE(x) fx::gltf::detail::WriteField(#x, json, obj.x)
#define WRITE2(x, y) fx::gltf::detail::WriteField(#x, json, obj.x, y)


std::string_view std::to_stringview(AGI::Articulations::Articulation::StageParameters::StageType type)
{
typedef AGI::Articulations::Articulation::StageParameters::StageType StageType;
	switch (type)
	{
	case StageType::xTranslate:   return "xTranslate";  break;
	case StageType::yTranslate:   return "yTranslate";  break;
	case StageType::zTranslate:   return "zTranslate";  break;
	case StageType::xRotate:      return "xRotate";  break;
	case StageType::yRotate:      return "yRotate";  break;
	case StageType::zRotate:      return "zRotate";  break;
	case StageType::xScale:       return "xScale";  break;
	case StageType::yScale:       return "yScale";  break;
	case StageType::zScale:       return "zScale";  break;
	case StageType::uniformScale: return "uniformScale";  break;
		default: throw fx::gltf::invalid_gltf_document("Unknown Articulation.Stage.StageType value");
	}
}

namespace AGI
{

inline void from_json(const nlohmann::json & json,  Articulations::Articulation::Stage::StageType & stageType)
{
	typedef Articulations::Articulation::Stage::StageType StageType;
	std::string type = json.get<std::string>();

	     if(type == "xTranslate"  ) stageType = StageType::xTranslate;
	else if(type == "yTranslate"  ) stageType = StageType::yTranslate;
	else if(type == "zTranslate"  ) stageType = StageType::zTranslate;
	else if(type == "xRotate"     ) stageType = StageType::xRotate;
	else if(type == "yRotate"     ) stageType = StageType::yRotate;
	else if(type == "zRotate"     ) stageType = StageType::zRotate;
	else if(type == "xScale"      ) stageType = StageType::xScale;
	else if(type == "yScale"      ) stageType = StageType::yScale;
	else if(type == "zScale"      ) stageType = StageType::zScale;
	else if(type == "uniformScale") stageType = StageType::uniformScale;
	else throw fx::gltf::invalid_gltf_document("Unknown ue4Collider.type value", type);
}

inline void to_json(nlohmann::json & json, Articulations::Articulation::Stage::StageType const& type)
{
	json = std::to_stringview(type);
}

inline void from_json(const nlohmann::json & json,  Articulations::Articulation::Stage & obj)
{
	READ(name);
	READ(minimumValue);
	READ(maximumValue);
	READ(maximumEffort);
	READ(maximumVelocity);
	READ(initialValue);
	READ(type);
}

inline void to_json(nlohmann::json & json, Articulations::Articulation::Stage const& obj)
{
	json["name"        ] = obj.name;
	json["type"        ] = obj.type;
	json["minimumValue"] = obj.minimumValue;
	json["maximumValue"] = obj.maximumValue;
	json["initialValue"] = obj.initialValue;
	json["maximumEffort"] = obj.maximumEffort;
	json["maximumVelocity"] = obj.maximumVelocity;
}

inline void from_json(const nlohmann::json & json,  Articulations::Articulation & obj)
{
	READ(name);
	READ(stages);
	READ(pointingVector);
}

inline void to_json(nlohmann::json & json, Articulations::Articulation const& obj)
{
	WRITE(name);
	WRITE(stages);
	WRITE(pointingVector);
}

void from_json(const nlohmann::json & json,  Articulations & obj)
{
	READ(articulations);
}

void to_json(nlohmann::json & json, Articulations const& obj)
{
	WRITE(articulations);
}

void from_json(const nlohmann::json & json,  NodeArticulation & obj)
{
	READ(articulationName);
	READ(isAttachPoint);
}

void to_json(nlohmann::json & json, NodeArticulation const& obj)
{
	WRITE(articulationName);
	WRITE2(isAttachPoint, false);
}

bool Articulations::Articulation::empty() const
{
	return stages.empty()
		&& name.empty()
		&& pointingVector == fx::gltf::defaults::NullVec3;
}

AGI_Lock AGI_LockFromArticulations(Articulations::Articulation const& it)
{
	int lock = AGI_Lock::All;

	for(auto const& stage : it.stages)
	{
		int id = 1 << ((int)stage.type-1);

		if(stage.minimumValue != stage.maximumValue)
		{
			(int&)lock &= ~id;
		}
	}
	
	return (AGI_Lock)lock;
}

AGI_Lock AGI_LockFromArticulations(unsafe_view<Articulations::Articulation::StageParameters> stages)
{
	int lock = AGI_Lock::All;

	for(auto const& stage : stages)
	{
		int id = 1 << ((int)stage.type-1);

		if(stage.minimumValue != stage.maximumValue)
		{
			(int&)lock &= ~id;
		}
	}
	
	return (AGI_Lock)lock;
}

AGI_Lock AGI_LockFromString(std::string_view name)
{
	if(name.find("LOCK") != 0)
		return {};

	std::string_view tokens = name.substr(0, name.find_first_of(':'));

	size_t i = 0;

	int lock = 0;
	
	while(true)
	{
		i = tokens.find_first_of("TRS", i);

		if(i > tokens.size())
		{
			if(tokens.find("all"))
				return All;

			break;
		}

		int flags = 0;

		for(auto p = &tokens[i+1]; ;++p)
		{
			auto c = tolower(*p);

			if('x' <= c && c <= 'z')
			{
				flags |= 1 << (c - 'x');
			}
			else if(c == 'u')
			{
				flags |= 4;
			}
			else
				break;
		}

		if(tokens[i] != 'S')
			flags &= 0x07;

		if(tokens[i] == 'T')
			flags <<= 0;
		else if(tokens[i] == 'R')
			flags <<= 3;
		else if(tokens[i] == 'S')
			flags <<= 6;

		lock |= flags;
	}
	
	return (AGI_Lock)lock;
}

}
