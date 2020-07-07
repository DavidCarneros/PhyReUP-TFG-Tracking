
/*************************************************************************
 *
 * Project          : Practica 3 de Sistemas Operativos II
 *
 * Program name     : cine.cpp
 *
 * Author           : David Carneros Prado
 *
 * Date created     : 04/04/2019
 *
 *
 * **********************************************************************/

#include <iostream>
#include <array>
#include <map>
#include <vector>
#include <k4a/k4a.h>
#include <k4abt.h>
#include <thread>
#include <stdarg.h> 
#include <winsock.h>
#pragma comment(lib,"WS2_32")
#include "UDPClient.hpp"
#include <mutex>

struct InputSettings
{
    k4a_depth_mode_t DepthCameraMode = K4A_DEPTH_MODE_NFOV_2X2BINNED;// K4A_DEPTH_MODE_NFOV_2X2BINNED K4A_DEPTH_MODE_NFOV_UNBINNED
    bool CpuOnlyMode = false;
    std::string serverIP;
};

/*----------- DECLARACION DE FUNCIONES -----------*/
char*        processResultAndSend(k4abt_skeleton_t skeleton);
char*        storePrintf(const char* fmt, ...);
bool         ParseInputSettingsFromArg(int argc, char** argv, InputSettings& inputSettings);
void         getAndSendResult(int id, InputSettings inputSettings);
void         GetDataFromDevice(InputSettings inputSettings);
void         Verify(k4a_result_t result, std::string error);
k4a_device_t openAndConfigureDevice(InputSettings inputSettings);
void         captureFrames(k4a_device_t device);
int64_t      CloseCallback(void* /*context*/);

/*------------------ VARIABLES -------------------*/
bool s_isRunning = true;
k4abt_tracker_t tracker;
UDPClient     udpClient;

/*------------------ SEMAFOROS -------------------*/
std::mutex sem_queue;

/*------------------ VECTORES -------------------*/
std::vector<std::thread> threads;


/*------------------ MAIN ------------------------*/
int main(int argc, char** argv)
{
    InputSettings inputSettings;
    if (ParseInputSettingsFromArg(argc, argv, inputSettings)) {
        GetDataFromDevice(inputSettings);
    }
}

k4a_device_t openAndConfigureDevice(InputSettings inputSettings)
{
    k4a_device_t device = nullptr;
    Verify(k4a_device_open(0, &device), "The device cannot be opened");
    std::cout << "Device Open! " << std::endl;

    k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    deviceConfig.depth_mode = inputSettings.DepthCameraMode;
    deviceConfig.color_resolution = K4A_COLOR_RESOLUTION_OFF;
    Verify(k4a_device_start_cameras(device, &deviceConfig), "Cannot start cameras");
    std::cout << "Start cameras" << std::endl;


    k4a_calibration_t sensorCalibration;
    Verify(k4a_device_get_calibration(device, deviceConfig.depth_mode, deviceConfig.color_resolution, &sensorCalibration), "Cannot get camera calibration");

    k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;
    tracker_config.processing_mode = inputSettings.CpuOnlyMode ? K4ABT_TRACKER_PROCESSING_MODE_CPU : K4ABT_TRACKER_PROCESSING_MODE_GPU;
    Verify(k4abt_tracker_create(&sensorCalibration, tracker_config, &tracker), "Cannot create tracker");

    return device;
}

void captureFrames(k4a_device_t device)
{
    while (s_isRunning)
    {

        k4a_capture_t sensorCapture = nullptr;
        k4a_wait_result_t getCaptureResult = k4a_device_get_capture(device, &sensorCapture, 0);

        if (getCaptureResult == K4A_WAIT_RESULT_SUCCEEDED) {
            k4a_wait_result_t  queueCaptureResult = k4abt_tracker_enqueue_capture(tracker, sensorCapture, 0);
            k4a_capture_release(sensorCapture);

            if (queueCaptureResult == K4A_WAIT_RESULT_FAILED) {
                std::cout << "Error! Add capture to tracker process queue failed!" << std::endl;
                break;
            }
        }
        else if (getCaptureResult != K4A_WAIT_RESULT_TIMEOUT) {
            std::cout << "Error! Add capture to tracker process queue failed!" << std::endl;
            break;
        }


    }
}
/*----------------- FUNCIONES_PRINCIPALES --------------------*/
void GetDataFromDevice(InputSettings inputSettings) {

    
    k4a_device_t device = openAndConfigureDevice(inputSettings);


    for (int i = 0; i < 2; i++) {
        threads.push_back(std::thread(getAndSendResult, i, inputSettings));
    }

    captureFrames(device);

    k4abt_tracker_shutdown(tracker);
    k4abt_tracker_destroy(tracker);
    k4a_device_stop_cameras(device);
    k4a_device_close(device);

}

void getAndSendResult(int id, InputSettings inputSettings) {
    UDPClient udpClient;
    udpClient.start_client(inputSettings.serverIP.c_str(), 8000);

    while (s_isRunning) {
        k4abt_frame_t bodyFrame = nullptr;
        sem_queue.lock();
        k4a_wait_result_t popFrameResult = k4abt_tracker_pop_result(tracker, &bodyFrame, 0);
        sem_queue.unlock();
        if (popFrameResult == K4A_WAIT_RESULT_SUCCEEDED) {
            int num_bodies = k4abt_frame_get_num_bodies(bodyFrame);
            if (num_bodies > 0) {
                k4abt_skeleton_t skeleton;
                k4abt_frame_get_body_skeleton(bodyFrame, 0, &skeleton);
                char* dataSend = processResultAndSend(skeleton);
                udpClient.send_data(dataSend);
            }

        }
    }
}

char* processResultAndSend(k4abt_skeleton_t skeleton) {

    k4a_float3_t right_hand = skeleton.joints[K4ABT_JOINT_HAND_RIGHT].position;
    k4a_float3_t left_hand = skeleton.joints[K4ABT_JOINT_HAND_LEFT].position;
    float right_hand_vector[3] = { -1 * right_hand.v[0] / 1000, -1 * right_hand.v[1] / 1000, -1 * right_hand.v[2] / 1000 };
    float left_hand_vector[3] = { -1 * left_hand.v[0] / 1000, -1 * left_hand.v[1] / 1000, -1 * left_hand.v[2] / 1000 };
    k4abt_joint_confidence_level_t right_hand_level = skeleton.joints[K4ABT_JOINT_HAND_RIGHT].confidence_level;
    k4abt_joint_confidence_level_t left_hand_level = skeleton.joints[K4ABT_JOINT_HAND_LEFT].confidence_level;



    const char* json_struct = "{\"right\":{\"x\":%f,\"y\":%f,\"z\":%f},\"right_level\":%d,\"left\":{\"x\":%f,\"y\":%f,\"z\":%f},\"left_level\":%d}";
    char* json_send = storePrintf(json_struct, right_hand_vector[0], right_hand_vector[1], right_hand_vector[2], right_hand_level,
        left_hand_vector[0], left_hand_vector[1], left_hand_vector[2], left_hand_level);

    return json_send;
}

/*----------------- FUNCIONES_SECUNDARIAS --------------------*/

void Verify(k4a_result_t result, std::string error) {
    if (result != K4A_RESULT_SUCCEEDED) {
        std::cout << error << std::endl;
        s_isRunning = false;
        exit(1);
    }
}

int64_t CloseCallback(void* /*context*/)
{
    s_isRunning = false;
    return 1;
}

bool ParseInputSettingsFromArg(int argc, char** argv, InputSettings& inputSettings)
{
    for (int i = 1; i < argc; i++)
    {
        std::string inputArg(argv[i]);
        if (inputArg == std::string("NFOV_UNBINNED"))
        {
            inputSettings.DepthCameraMode = K4A_DEPTH_MODE_NFOV_UNBINNED;
        }
        else if (inputArg == std::string("NFOV_BINNED")) {
            inputSettings.DepthCameraMode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
        } 
        else if (inputArg == std::string("WFOV_BINNED")) {
            inputSettings.DepthCameraMode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
        }
        else if (inputArg == std::string("CPU"))
        {
            inputSettings.CpuOnlyMode = true;
        }
        else if (inputArg == std::string("IP")) {
            if (i < argc - 1) {
                inputSettings.serverIP = argv[i + 1];
                i++;
            }
            else {
                return false;
            }
        }
        else {
            std::cout << "Error command not valid: " << inputArg.c_str() << std::endl;
            return false;
        }
    }
    return true;
}

char* storePrintf(const char* fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    size_t sz = snprintf(NULL, 0, fmt, arg);
    char* buf = (char*)malloc(sz + 1);
#pragma warning(disable: 4996)
    vsprintf(buf, fmt, arg);
    va_end(arg);
    return buf;
}

