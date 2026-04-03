#ifndef SENSOR_CONTROLLER_H
#define SENSOR_CONTROLLER_H

#include <Arduino.h>
#include <LSM6DSOXSensor.h>
#include <LIS3MDLSensor.h>

#include "main.h"
#include "pixelController.h"



enum GestureType {
  GESTURE_NONE,
  GESTURE_SOFT_TAP,
  GESTURE_HARD_TAP,
  GESTURE_VERTICAL_THRUST,
  GESTURE_HORIZONTAL_THRUST,
  GESTURE_SWIRL_CW,
  GESTURE_SWIRL_CCW
};

enum SequenceType {
    SEQ_NONE,
    SEQ_FIREBALL,
    SEQ_NECROBALL,
};
class SensorController {
    private:
        PixelController& controller;
        LSM6DSOXSensor lsm6dsox;
        LIS3MDLSensor lis3mdl;
        int32_t accel[3];
        int32_t linAcc[3];
        int32_t gyro[3];
        int32_t mag[3];
        uint32_t lastReadTime;

        // Gesture detection state
        GestureType currentGesture;
        bool newGestureFlag;
        
        SequenceType currentSequence;
        bool newSequenceFlag;

        // Circular buffer
        static const int BUFFER_SIZE = 300;
        struct SensorSample {
            uint32_t timestamp;
            int32_t accelX, accelY, accelZ;  // [mg]
            int32_t linX, linY, linZ;        // [mg]
            int32_t gyroX, gyroY, gyroZ;     // [mdps]
        };
        SensorSample buffer[BUFFER_SIZE];
        int bufferHead;
        int bufferCount;
        

        enum GestureState {
            STATE_IDLE,
            STATE_MOTION_DETECTED,
            STATE_ANALYZING,
            STATE_COOLDOWN
        };
        GestureState gState;
        uint32_t stateStartTime;
        uint32_t motionStartTime;
        
        static const int GESTURE_HISTORY_SIZE = 10;
        struct GestureEvent {
            GestureType type;
            uint32_t timestamp;
        };
        GestureEvent gestureHistory[GESTURE_HISTORY_SIZE];
        int gestureHistoryHead;
        int gestureHistoryCount;


        uint32_t sequenceWindowMs = 2000;  // Max time between gestures in a sequence
        uint32_t lastGestureTime;



        // Thresholds
        int32_t softTapThreshold = 400;       // mg
        int32_t hardTapThreshold = 1300;       // mg
        int32_t horizontalThrustThreshold = 300;        // mg
        int32_t verticalThrustThreshold = 1100;
        float tiltThreshold = 25.0;            // degrees/s
        float swirlRotationThreshold = 180.0f;   // degrees
        int32_t motionStartThreshold = 50;   // mg
        
        bool debugOutput = true;
    
        // Internal methods
        void addToBuffer(int32_t ax, int32_t ay, int32_t az,
                        int32_t lx, int32_t ly, int32_t lz,
                        int32_t gx, int32_t gy, int32_t gz);
        void analyzeGesture();
        GestureType detectTap();
        GestureType detectThrust();
        GestureType detectSwirl();
        bool isMotionActive();
        void resetGestureState();
        
        float calculateAccelMagnitude();
        float calculateGyroMagnitude();
        void addGestureToHistory(GestureType gesture);
        SequenceType detectSequence();
        bool matchPattern(const GestureType* pattern, int patternLength);
        

    public:
        SensorController(PixelController& pixController);

        void begin();
        void readSensor();

        // Gesture detection
        bool gestureDetected();
        GestureType getLastGesture();
        void clearGesture();
        

        void getAcceleration(int32_t &x, int32_t &y, int32_t &z);
        void getLinearAcceleration(int32_t &linX, int32_t &linY, int32_t &linZ);
        void getGyro(int32_t &x, int32_t &y, int32_t &z);
        void getMag(int32_t &x, int32_t &y, int32_t &z);
        float getTiltAngle();
        
   
        void enableDebugOutput(bool enable);
        bool sequenceDetected();
        SequenceType getLastSequence();
        void clearSequence();

        
};
#endif