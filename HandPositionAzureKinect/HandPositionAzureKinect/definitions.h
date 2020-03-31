#pragma once

#include <k4a/k4a.hpp>
#include <k4abt.hpp>

/*
struct hands_data_t {
	k4a_float3_t hand_right_position;
	k4a_float3_t hand_left_position;
	k4a_quaternion_t hand_right_orientation;
	k4a_quaternion_t hand_left_orientation;
	k4abt_joint_confidence_level_t hand_right_confidence_level;
	k4abt_joint_confidence_level_t hand_left_confidence_level;
};
*/

struct hands_data_t {
//	k4a_quaternion_t hand_right_orientation;
//	k4a_quaternion_t hand_left_orientation;
	k4abt_joint_confidence_level_t hand_right_confidence_level;
	k4abt_joint_confidence_level_t hand_left_confidence_level;
};
