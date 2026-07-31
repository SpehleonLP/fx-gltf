#ifndef AGI_ARTICULATION_H
#define AGI_ARTICULATION_H
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <nlohmann/json.hpp>

namespace AGI
{

struct Articulations
{
	struct Articulation
	{
		struct StageParameters
		{
			enum class StageType : char
			{
				None,
				xTranslate,
				yTranslate,
				zTranslate,
				xRotate,
				yRotate,
				zRotate,
				xScale,
				yScale,
				zScale,
				uniformScale,
				total,

				RotationMask = (1 << xRotate) |  (1 << yRotate) | (1 << zRotate)
			} type{StageType::None};

			float minimumValue{0};
			float maximumValue{0};
			float initialValue{0};
			// Named for what they actually are (ChaCha's measurements),
			// not glTF/AGI schema properties -- there are none by these
			// names or any names in the stage schema. Serialized through
			// extras.chachaMaximumSpeed/chachaMaximumAcceleration by
			// Stage's to_json/from_json in agi_articulation.cpp; these
			// members are never read from or written to a top-level JSON
			// key. See that file for the extras merge/precedence rule.
			float maximumSpeed{1.f};
			float maximumAcceleration{1.f};
		};
		
		struct Stage : public StageParameters
		{
		typedef StageParameters::StageType StageType;
			std::string name;
			// AGI_articulations' schema (articulation.stage.schema.json) only
			// defines name/type/minimumValue/maximumValue/initialValue plus the
			// standard extensions/extras hooks. extensionsAndExtras is the
			// sanctioned extension point for anything a producer wants to
			// attach beyond that -- maximumSpeed/maximumAcceleration above
			// are serialized through it (extras.chachaMaximumSpeed /
			// extras.chachaMaximumAcceleration) rather than as top-level
			// properties, since they aren't part of the spec either. Same
			// pattern as fx::gltf's own types (see gltf.h); read/written via
			// fx::gltf::detail::ReadExtensionsAndExtras/WriteExtensions in
			// agi_articulation.cpp's to_json/from_json.
			nlohmann::json extensionsAndExtras{};

			bool empty() const { return false; }
			bool operator==(Stage const& it) const
			{
				return name == it.name
					&& minimumValue == it.minimumValue
					&& maximumValue == it.maximumValue
					&& initialValue == it.initialValue;
			}
		};

		std::string name;
		std::vector<Stage> stages;
		std::array<float, 3> pointingVector{0,0,0};

		bool empty() const;
		bool operator==(Articulation const& it) const
		{
			return name == it.name
				&& pointingVector == it.pointingVector
				&& stages == it.stages;
		}
	};

	std::vector<Articulation> articulations;

	bool empty() const { return articulations.empty(); }
	bool operator==(Articulations const& it) const
	{
		return articulations == it.articulations;
	}
};

enum AGI_Lock : uint16_t {
	TranslationX	 = 1 << 0,
	TranslationY	 = 1 << 1,
	TranslationZ	 = 1 << 2,
	RotationRoll	 = 1 << 3,
	RotationPitch	 = 1 << 4,
	RotationYaw		 = 1 << 5,
	ScaleX			 = 1 << 6,
	ScaleY			 = 1 << 7,
	ScaleZ			 = 1 << 8,
	UniformScale     = 1 << 9,

	Translation = TranslationX | TranslationY | TranslationZ,
	Rotation	= RotationRoll | RotationPitch | RotationYaw,
	Scale		= ScaleX | ScaleY | ScaleZ | UniformScale,

	Position = Translation|Rotation,
	All		= Translation|Rotation|Scale
};

AGI_Lock AGI_LockFromString(std::string_view);
AGI_Lock AGI_LockFromArticulations(Articulations::Articulation const& it);

struct NodeArticulation
{
	std::string articulationName;
	bool        isAttachPoint{};

	bool empty() const
	{
		return articulationName.empty() && isAttachPoint == false;
	}

	bool operator==(NodeArticulation const& it) const
	{
		return articulationName == it.articulationName
			&& isAttachPoint == it.isAttachPoint;
	}
};

}

namespace std
{
std::string_view to_stringview(AGI::Articulations::Articulation::StageParameters::StageType type);
}

#endif // AGI_ARTICULATIONS_H
