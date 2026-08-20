#ifndef KRE_ROOT_MOTION_H
#define KRE_ROOT_MOTION_H
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace KRE
{

struct RootMotion
{
	// === Mask (informational; runtime uses to know what was extracted) ===
	uint8_t   translation_mask{0};   // bit 0/1/2 = x/y/z
	uint8_t   scaling_mask{0};       // bit 0/1/2 = x/y/z
	glm::vec3 rotation_axis{0, 1, 0};
	bool      rotation_swing{false};
	bool      rotation_twist{false};

	// === Cubic Hermite, single segment, t in [0, duration] ===
	// For unmasked components, leave default (zeros). Sentinel = mask bit clear.
	struct HermiteVec3
	{
		glm::vec3 v0{0, 0, 0}, t0_in{0, 0, 0}, t0_out{0, 0, 0};
		glm::vec3 v1{0, 0, 0}, t1_in{0, 0, 0}, t1_out{0, 0, 0};

		bool operator==(HermiteVec3 const& it) const
		{
			return v0 == it.v0 && t0_in == it.t0_in && t0_out == it.t0_out
				&& v1 == it.v1 && t1_in == it.t1_in && t1_out == it.t1_out;
		}
	};
	HermiteVec3 translation;
	HermiteVec3 scaling;

	// Rotation as two log-quat curves around the configured rotation_axis:
	//   swing : QuatLog(swing_q), components live in the plane perpendicular
	//           to rotation_axis (axis-aligned component is ~0 by construction).
	//   twist : QuatLog(twist_q), components live along rotation_axis (perpendicular
	//           components are ~0). Twist angle is unwrapped at write time so the
	//           curve can encode multi-turn rotations (delta exceeds [-pi, pi]).
	// Runtime decode: q(t) = exp(swing.eval(t)) * exp(twist.eval(t)).
	HermiteVec3 swing;
	HermiteVec3 twist;

	// === Residual variation (Synty / mesh-node case) ===
	// -1 = no residual stored on extension (Mixamo case: residual is in original channel).
	// When non-negative, both indices reference accessors / bufferViews in the doc;
	// Module E settles which (recommend accessors). Module F (GC) walks them.
	int32_t residual_input{-1};
	int32_t residual_output{-1};
	int32_t residual_target_node{-1};   // -1 if N/A

	// === Logical owner for runtime ===
	int32_t attach_node{-1};

	bool empty() const noexcept
	{
		return translation_mask == 0
			&& scaling_mask == 0
			&& !rotation_swing && !rotation_twist
			&& residual_input < 0
			&& residual_output < 0
			&& residual_target_node < 0
			&& attach_node < 0;
	}

	bool operator==(RootMotion const& it) const
	{
		return translation_mask == it.translation_mask
			&& scaling_mask == it.scaling_mask
			&& rotation_axis == it.rotation_axis
			&& rotation_swing == it.rotation_swing
			&& rotation_twist == it.rotation_twist
			&& translation == it.translation
			&& scaling == it.scaling
			&& swing == it.swing
			&& twist == it.twist
			&& residual_input == it.residual_input
			&& residual_output == it.residual_output
			&& residual_target_node == it.residual_target_node
			&& attach_node == it.attach_node;
	}
};

}

#endif // KRE_ROOT_MOTION_H
