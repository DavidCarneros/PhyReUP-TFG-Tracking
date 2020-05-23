// AzureKinect-HandsTracking.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include <iostream>
#include <array>
#include <map>
#include <vector>
#include <k4a/k4a.h>
//#include <k4a/k4a.h>
#include <k4abt.h>
//#include <k4abt.h>
#include <thread>
#include <stdarg.h> 
#include <winsock.h>
#pragma comment(lib,"WS2_32")
#include "UDPClient.hpp"

struct InputSettings
{
    k4a_depth_mode_t DepthCameraMode = K4A_DEPTH_MODE_NFOV_2X2BINNED;// K4A_DEPTH_MODE_NFOV_2X2BINNED K4A_DEPTH_MODE_NFOV_UNBINNED
    bool CpuOnlyMode = false;
    std::string serverIP;
};

// Declaracion de funciones
int64_t CloseCallback(void* /*context*/);
bool ParseInputSettingsFromArg(int argc, char** argv, InputSettings& inputSettings);
void getAndSendResult(int id, InputSettings inputSettings);
void getAndSendResult2();
char* processResultAndSend(k4abt_skeleton_t skeleton);
void GetDataFromDevice(InputSettings inputSettings);
char* storePrintf(const char* fmt, ...);

// Estado global 
bool s_isRunning = true;
int depthWidth;
int depthHeight;
//k4abt::tracker tracker;
k4abt_tracker_t tracker;
//UDPClient udpClient;
std::vector<std::thread> threads;

// Funciones
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
        else if (inputArg == std::string("WFOV_BINNED")) {
            inputSettings.DepthCameraMode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
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


void GetDataFromDevice(InputSettings inputSettings) {

    //k4a::device device = nullptr;
    //device = k4a::device::open(0);
    k4a_device_t device = nullptr;
    k4a_device_open(0, &device);

    // COMPROBAR ERROR
    std::cout << "Device Open! " << std::endl;


    k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    deviceConfig.depth_mode = inputSettings.DepthCameraMode;
    deviceConfig.color_resolution = K4A_COLOR_RESOLUTION_OFF;
    k4a_device_start_cameras(device, &deviceConfig);

    // Start Camera
   // device.start_cameras(&deviceConfig)
    std::cout << "Start cameras" << std::endl;

    //k4a::calibration sensorCalibration;
    //sensorCalibration = device.get_calibration(deviceConfig.depth_mode, deviceConfig.color_resolution);
    k4a_calibration_t sensorCalibration;
    k4a_device_get_calibration(device, deviceConfig.depth_mode, deviceConfig.color_resolution, &sensorCalibration);
    depthHeight = sensorCalibration.depth_camera_calibration.resolution_height;
    depthWidth = sensorCalibration.depth_camera_calibration.resolution_width;

    // Tracker
    k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;
    tracker_config.processing_mode = inputSettings.CpuOnlyMode ? K4ABT_TRACKER_PROCESSING_MODE_CPU : K4ABT_TRACKER_PROCESSING_MODE_GPU;
    //tracker = k4abt::tracker::create(sensorCalibration, tracker_config);
    k4abt_tracker_create(&sensorCalibration, tracker_config, &tracker);

    /// INICIAR 3D windows controller
    for (int i = 0; i < 2; i++) {
       threads.push_back(std::thread(getAndSendResult, i,inputSettings));
    }

    while (s_isRunning)
    {
        //k4a::capture sensorCapture = nullptr;
        k4a_capture_t sensorCapture = nullptr;

        //k4a_wait_result_t getCaptureResult = 
        //bool capture = device.get_capture(&sensorCapture,std::chrono::milliseconds(0));
        k4a_wait_result_t getCaptureResult = k4a_device_get_capture(device, &sensorCapture, 0);

        if (getCaptureResult == K4A_WAIT_RESULT_SUCCEEDED) {
            k4a_wait_result_t  queueCaptureResult = k4abt_tracker_enqueue_capture(tracker, sensorCapture,0);
            k4a_capture_release(sensorCapture);
            
            if (queueCaptureResult == K4A_WAIT_RESULT_FAILED) {
                std::cout << "Error! Add capture to tracker process queue failed!" << std::endl;
              //  break;
            }
        }
        else if (getCaptureResult != K4A_WAIT_RESULT_TIMEOUT) {
            std::cout << "Error! Add capture to tracker process queue failed!" << std::endl;
            break;
        }

       // getAndSendResult2();
        
    }

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
        k4a_wait_result_t popFrameResult = k4abt_tracker_pop_result(tracker, &bodyFrame, 0);
        if (popFrameResult == K4A_WAIT_RESULT_SUCCEEDED) {
            int num_bodies = k4abt_frame_get_num_bodies(bodyFrame);
            if (num_bodies > 0) {
                k4abt_skeleton_t skeleton;
                k4abt_frame_get_body_skeleton(bodyFrame, 0,&skeleton);
                char* dataSend = processResultAndSend(skeleton);
                udpClient.send_data(dataSend);
               // k4a_float3_t right_hand = skeleton.joints[K4ABT_JOINT_HAND_RIGHT].position;
               // std::cout << "[Thread " << id << "] x: " << right_hand.v[0] << " y: " << right_hand.v[1] <<"  z: " << right_hand.v[2] << std::endl;
            }
           
        }
    }
}

void getAndSendResult2() {

        k4abt_frame_t bodyFrame = nullptr;
        k4a_wait_result_t popFrameResult = k4abt_tracker_pop_result(tracker, &bodyFrame, 0);
        if (popFrameResult == K4A_WAIT_RESULT_SUCCEEDED) {
            int num_bodies = k4abt_frame_get_num_bodies(bodyFrame);
            if (num_bodies > 0) {
                k4abt_skeleton_t skeleton;
                k4abt_frame_get_body_skeleton(bodyFrame, 0, &skeleton);
          //      processResultAndSend(skeleton);
                // k4a_float3_t right_hand = skeleton.joints[K4ABT_JOINT_HAND_RIGHT].position;
                // std::cout << "[Thread " << id << "] x: " << right_hand.v[0] << " y: " << right_hand.v[1] <<"  z: " << right_hand.v[2] << std::endl;
            }

        }
   
}

char* processResultAndSend(k4abt_skeleton_t skeleton) {
    k4a_float3_t right_hand = skeleton.joints[K4ABT_JOINT_HAND_RIGHT].position;
    float right_hand_vector[3] = { -1 * right_hand.v[0]/1000, -1 * right_hand.v[1]/1000, -1 * right_hand.v[2]/1000 };
    k4abt_joint_confidence_level_t right_hand_level = skeleton.joints[K4ABT_JOINT_HAND_RIGHT].confidence_level;



    const char* json_struct = "{\"right\":{\"x\":%f,\"y\":%f,\"z\":%f},\"right_level\":%d,\"q_right\":{\"w\":%f,\"x\":%f,\"y\":%f,\"z\":%f}}";
    char* json_send = storePrintf(json_struct, right_hand_vector[0], right_hand_vector[1], right_hand_vector[2], right_hand_level, 0,0,0, 0);

    //udpClient.send_data(json_send);
    return json_send;
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



int main(int argc, char** argv)
{
    InputSettings inputSettings;
    if (ParseInputSettingsFromArg(argc, argv, inputSettings)) {
       // udpClient.start_client(inputSettings.serverIP.c_str(), 8000);
        GetDataFromDevice(inputSettings);
    }
    //std::cout << "Hello World!\n";
}

// Ejecutar programa: Ctrl + F5 o menú Depurar > Iniciar sin depurar
// Depurar programa: F5 o menú Depurar > Iniciar depuración

// Sugerencias para primeros pasos: 1. Use la ventana del Explorador de soluciones para agregar y administrar archivos
//   2. Use la ventana de Team Explorer para conectar con el control de código fuente
//   3. Use la ventana de salida para ver la salida de compilación y otros mensajes
//   4. Use la ventana Lista de errores para ver los errores
//   5. Vaya a Proyecto > Agregar nuevo elemento para crear nuevos archivos de código, o a Proyecto > Agregar elemento existente para agregar archivos de código existentes al proyecto
//   6. En el futuro, para volver a abrir este proyecto, vaya a Archivo > Abrir > Proyecto y seleccione el archivo .sln
