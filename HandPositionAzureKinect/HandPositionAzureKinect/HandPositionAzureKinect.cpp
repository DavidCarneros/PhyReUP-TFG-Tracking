// HandPositionAzureKinect.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//
//#include <Windows.h>
#include <assert.h>
#include <iostream>


#include <k4a/k4a.hpp>
#include <k4abt.hpp>

#include <winsock.h>
#include <string.h>
#include <queue>
#include <thread>
#include <condition_variable>

#include "definitions.h"
#include "UDPClient.hpp"
#include "SemCounter.hpp"

void print_body_information(k4abt_body_t body);
//void print_hands_position(k4abt_body_t body, SOCKET UDPSocketClient, struct sockaddr_in UDPServer);
void print_hands_position(k4abt_body_t body);
void print_joint_information(const char* hand, k4a_float3_t position, k4a_quaternion_t orientation, k4abt_joint_confidence_level_t confidence_level);
void updClient_thread();

UDPClient udpClient;
//std::queue<hands_data_t> hand_data_queue;


int main()
{
	//sem.wait();
	std::cout << "Starting application!" << std::endl;

	try {
		udpClient.start_client("192.168.1.43",  8000);
		// std::thread udp_client_thread(updClient_thread);
		
		// configuracion 
		k4a_device_configuration_t device_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
		device_config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;

		k4a::device device = k4a::device::open(0);
		std::cout << "Device Open!" << std::endl;
		device.start_cameras(&device_config);
		std::cout << "Cameras started! " << std::endl; 

		k4a::calibration sensor_calibration = device.get_calibration(device_config.depth_mode, device_config.color_resolution);
		k4abt_tracker_configuration_t config;
		config.gpu_device_id = 0;
		config.processing_mode = K4ABT_TRACKER_PROCESSING_MODE_GPU;
		config.sensor_orientation = K4ABT_SENSOR_ORIENTATION_FLIP180;
		k4abt::tracker tracker = k4abt::tracker::create(sensor_calibration);
		std::cout << "Tracker created!" << std::endl;


		int frame_count = 0;

		do {

			k4a::capture sensor_capture;

			if (device.get_capture(&sensor_capture, std::chrono::milliseconds(K4A_WAIT_INFINITE))) {

				frame_count++;
				std::cout << "Start processing frame " << frame_count << std::endl;

				if (!tracker.enqueue_capture(sensor_capture)) {
					std::cout << "Error! Add capture to tracker process queue timeout!" << std::endl;
					break;
				}

				k4abt::frame body_frame = tracker.pop_result();
				if (body_frame != nullptr) {
					uint32_t num_bodies = body_frame.get_num_bodies();
					std::cout << num_bodies << " Bodies are detected! " << std::endl;

				
					if (num_bodies > 0) {
						k4abt_body_t body = body_frame.get_body(0);
						print_hands_position(body);
					}
					else {
						std::cout << "No body information! " << std::endl;
					}


				}
				else {
					std::cout << "Error! Pop body frame result time out!" << std::endl;
					break;
				}


			}
			else {
				std::cout << "Error! Get depth frame time out!" << std::endl;
				break;
			}


		} while (1);
		std::cout << "Finished body tracking processing!" << std::endl;
		 
	}
	catch (const std::exception & e) {
		std::cerr << "Failed with exception: " << std::endl
			<< "  " << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}

/*
void updClient_thread() {
	struct hands_data_t aux;
	std::cout << "Antes primer wait hilo" << std::endl;
//	try {
		while (1) {
			std::cout << "Antes wait " << std::endl;
			//sem.wait();
			std::cout << "despues wait" << std::endl;
			aux = hand_data_queue.front();
			hand_data_queue.pop();
			udpClient.send_hands_data(aux);
		}
//	}
//	catch (std::exception& e) {
//		std::cout << e.what() << std::endl;
//	}

}
*/

//void print_hands_position(k4abt_body_t body, SOCKET UDPSocketClient, struct sockaddr_in UDPServer){
void print_hands_position(k4abt_body_t body){

	k4a_float3_t hand_right_position = body.skeleton.joints[K4ABT_JOINT_HAND_RIGHT].position;
	k4a_float3_t hand_left_position = body.skeleton.joints[K4ABT_JOINT_HAND_LEFT].position;
	k4a_float3_t head_position = body.skeleton.joints[K4ABT_JOINT_NOSE].position;
	k4a_float3_t right_eye_position = body.skeleton.joints[K4ABT_JOINT_EYE_RIGHT].position;
	k4a_float3_t left_eye_position = body.skeleton.joints[K4ABT_JOINT_EYE_LEFT].position;

	k4a_float3_t middle_point;
	float middle[3];
	middle[0] = (right_eye_position.v[0] + left_eye_position.v[0]) / 2;
	middle[1] = (right_eye_position.v[1] + left_eye_position.v[1]) / 2;
	middle[2] = (right_eye_position.v[2] + left_eye_position.v[2]) / 2;


	//float rigth_hand_vector[3] = { middle[0] - hand_right_position.v[0], middle[1] - hand_right_position.v[1], middle[2] - hand_right_position.v[2] };
	float rigth_hand_vector[3] = {-1 * hand_right_position.v[0], -1 *  hand_right_position.v[1], -1 * hand_right_position.v[2] };
	
	float left_hand_vector[3] = { 1,2,3 };

	k4a_quaternion_t hand_right_orientation = body.skeleton.joints[K4ABT_JOINT_HAND_RIGHT].orientation;
	k4a_quaternion_t hand_left_orientation = body.skeleton.joints[K4ABT_JOINT_HAND_LEFT].orientation;

	k4abt_joint_confidence_level_t hand_right_confidence_level = body.skeleton.joints[K4ABT_JOINT_HAND_RIGHT].confidence_level;
	k4abt_joint_confidence_level_t hand_left_confidence_level = body.skeleton.joints[K4ABT_JOINT_HAND_LEFT].confidence_level;

	hands_data_t hand_data;
	hand_data.hand_right_confidence_level = hand_right_confidence_level;
	/*
	hands_data_t hand_data;
	hand_data.hand_right_position = hand_right_position;
	hand_data.hand_left_position = hand_left_position;
	hand_data.hand_right_orientation = hand_right_orientation;
	hand_data.hand_left_orientation = hand_left_orientation;
	hand_data.hand_right_confidence_level = hand_right_confidence_level;
	hand_data.hand_left_confidence_level = hand_left_confidence_level;
	*/
	//hand_data_queue.push(hand_data);
	//sem.signal();
	//sendto(UDPSocketClient, (char*)&hand_data, sizeof(hand_data), MSG_DONTROUTE, (SOCKADDR*)&UDPServer, sizeof(UDPServer));
	//sendto(UDPSocketClient, prueba, strlen(prueba), MSG_DONTROUTE, (SOCKADDR*)&UDPServer, sizeof(UDPServer));
	udpClient.send_hands_data(hand_data,rigth_hand_vector, hand_right_orientation);

	//print_joint_information("Right", hand_right_position, hand_right_orientation, hand_right_confidence_level);
	//print_joint_information("Left", hand_left_position, hand_left_orientation, hand_left_confidence_level);

}

void print_joint_information(const char* hand, k4a_float3_t position, k4a_quaternion_t orientation, k4abt_joint_confidence_level_t confidence_level) {
	printf("HAND %s -- Position ( %f, %f, %f ) :: Orientation ( %f, %f, %f, %f); Confidence Level (%d)  \n",
		hand, position.v[0], position.v[1], position.v[2], orientation.v[0], orientation.v[1], orientation.v[2], orientation.v[3], confidence_level);

}

void print_body_information(k4abt_body_t body) {
	std::cout << "Body ID: " << body.id << std::endl;

	for (int i = 0; i < (int)K4ABT_JOINT_COUNT; i++) {
		k4a_float3_t position = body.skeleton.joints[i].position;
		k4a_quaternion_t orientation = body.skeleton.joints[i].orientation;
		k4abt_joint_confidence_level_t confidence_level = body.skeleton.joints[i].confidence_level;

		printf("Joint[%d]: Position[mm] ( %f, %f, %f ); Orientation ( %f, %f, %f, %f); Confidence Level (%d)  \n",
			i, position.v[0], position.v[1], position.v[2], orientation.v[0], orientation.v[1], orientation.v[2], orientation.v[3], confidence_level);
	}
}
