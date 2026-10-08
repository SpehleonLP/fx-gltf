#include "agi_articulation.h"
#include "Support/unsafe_view.hpp"
#include <fx/gltf.h>
#include <cctype>

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
	READ(initialValue);
	READ(type);
	fx::gltf::detail::ReadExtensionsAndExtras(json, obj.extensionsAndExtras);

	// maximumSpeed/maximumAcceleration are NOT top-level Stage properties --
	// they live in extras.chachaMaximumSpeed/chachaMaximumAcceleration (see
	// to_json below), so pull them back out of the extras blob
	// ReadExtensionsAndExtras just captured, rather than READ()ing a
	// top-level key. Deliberately NO fallback to the legacy top-level
	// "maximumVelocity"/"maximumEffort" keys this struct used to serialize:
	// the user explicitly ruled out backward compatibility here. A file
	// written by an older fx-gltf that still has those top-level keys loses
	// those values on reload -- both members fall back to their 1.f default.
	obj.maximumSpeed = 1.f;
	obj.maximumAcceleration = 1.f;
	if (auto extras_it = obj.extensionsAndExtras.find("extras"); extras_it != obj.extensionsAndExtras.end())
	{
		if (auto it = extras_it->find("chachaMaximumSpeed"); it != extras_it->end())
			obj.maximumSpeed = it->get<float>();
		if (auto it = extras_it->find("chachaMaximumAcceleration"); it != extras_it->end())
			obj.maximumAcceleration = it->get<float>();
	}
}

inline void to_json(nlohmann::json & json, Articulations::Articulation::Stage const& obj)
{
	json["name"        ] = obj.name;
	json["type"        ] = obj.type;
	json["minimumValue"] = obj.minimumValue;
	json["maximumValue"] = obj.maximumValue;
	json["initialValue"] = obj.initialValue;

	// Write the caller's extensionsAndExtras FIRST (this replaces
	// json["extras"]/json["extensions"] wholesale with whatever the caller
	// set by hand -- see WriteExtensions), then merge chacha's own extras
	// keys in on top of that. Precedence, made explicit and deterministic:
	// the typed maximumSpeed/maximumAcceleration members are the struct's
	// source of truth and always win over a hand-authored
	// extras.chachaMaximumSpeed/chachaMaximumAcceleration set to a
	// different value under the same key -- they overwrite it rather than
	// silently losing to it or duplicating the key. Anything else the
	// caller put in extras (any other key) is preserved untouched, since
	// only these two specific keys are ever assigned here. Only written
	// when they differ from the 1.f default, so a stage nobody set these on
	// doesn't gain a meaningless extras entry, and round-trips exactly.
	fx::gltf::detail::WriteExtensions(json, obj.extensionsAndExtras);
	if (obj.maximumSpeed != 1.f)
		json["extras"]["chachaMaximumSpeed"] = obj.maximumSpeed;
	if (obj.maximumAcceleration != 1.f)
		json["extras"]["chachaMaximumAcceleration"] = obj.maximumAcceleration;
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

// A node name opts axes out of articulation: `LOCK…<group><axes>…[:rest]`, groups T, R, S with
// axes x y z (u, uniform scale, in S), or a whole `_`-delimited token `all`. The parse stops at
// ':' and never reads past the view; each group is read once.
AGI_Lock AGI_LockFromString(std::string_view name)
{
	if(name.substr(0, 4) != "LOCK")
		return {};

	std::string_view const tokens = name.substr(0, name.find(':'));
	// `all` is a whole token, so `LOCK_Tx_ball` is not All; the scan starts after the prefix.
	for(size_t b = 4; b <= tokens.size();)
	{
		size_t e = tokens.find('_', b);
		if(e == std::string_view::npos) e = tokens.size();
		if(tokens.substr(b, e - b) == "all")
			return All;
		b = e + 1;
	}

	int lock = 0;
	for(size_t i = tokens.find_first_of("TRS"); i != std::string_view::npos; i = tokens.find_first_of("TRS", i))
	{
		char const group = tokens[i++];
		int flags = 0;
		for(; i < tokens.size(); ++i)
		{
			int const c = std::tolower(static_cast<unsigned char>(tokens[i]));
			if('x' <= c && c <= 'z')           flags |= 1 << (c - 'x');
			else if(c == 'u' && group == 'S')  flags |= 8;
			else                               break;
		}
		lock |= flags << (group == 'T' ? 0 : group == 'R' ? 3 : 6);
	}
	return (AGI_Lock)lock;
}

}
