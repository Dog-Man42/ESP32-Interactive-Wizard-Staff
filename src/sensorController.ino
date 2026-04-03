#include "sensorController.h"
#include "main.h"


SensorController::SensorController(PixelController& pixController) :
    controller(pixController),
    lsm6dsox(&I2C_WIRE, LSM6DSOX_I2C_ADD_L),
    lis3mdl(&I2C_WIRE, LIS3MDL_MAG_I2C_ADDRESS_LOW){
    gestureHistoryHead = 0;
    gestureHistoryCount = 0;
    lastGestureTime = 0;
}

void SensorController::begin(){
    
    //accel/gyro
    lsm6dsox.begin();
    lsm6dsox.Enable_X();
    lsm6dsox.Enable_G();

    //config 
    lsm6dsox.Set_X_ODR(417.0f);     //hzsampling
    lsm6dsox.Set_X_ODR(8);         //g scale
    lsm6dsox.Set_G_ODR(417.0f);     //hz sampling
    lsm6dsox.Set_G_FS(500);         //dps scale
    
    //magnetometer
    lis3mdl.begin();
    lis3mdl.Enable();
    lis3mdl.SetODR(100.0f);         //hz sampling
    lis3mdl.SetFS(4.0f);            //gauss scale
    

}
bool printedReadings = false;
void SensorController::readSensor(){
    lsm6dsox.Get_X_Axes(accel);
    lsm6dsox.Get_G_Axes(gyro);
    lis3mdl.GetAxes(mag);



    getLinearAcceleration(linAcc[0],linAcc[1],linAcc[2]);

    lastReadTime = millis();

    addToBuffer(accel[0], accel[1], accel[2],
                linAcc[0], linAcc[1], linAcc[2],
                gyro[0], gyro[1], gyro[2]);

    
    switch (gState) {
        case STATE_IDLE:
            if (isMotionActive()) {
                gState = STATE_MOTION_DETECTED;
                motionStartTime = millis();
                stateStartTime = millis();
            }
            break;
        case STATE_MOTION_DETECTED:
            if (millis() - motionStartTime > 300) {
                //Long gestures
                gState = STATE_ANALYZING;
                stateStartTime = millis();
            } else if (!isMotionActive() && millis() - motionStartTime > 100) {
                // Quick gestures
                gState = STATE_ANALYZING;
                stateStartTime = millis();
            }
            break;
        case STATE_ANALYZING:
            analyzeGesture();
            gState = STATE_COOLDOWN;
            stateStartTime = millis();
            break;
        case STATE_COOLDOWN:
            if (millis() - stateStartTime > 300) {
                resetGestureState();
                gState = STATE_IDLE;
            }
            break;
    }
}

void SensorController::addToBuffer(int32_t ax, int32_t ay, int32_t az,
                                    int32_t lx, int32_t ly, int32_t lz, 
                                    int32_t gx, int32_t gy, int32_t gz) {
    buffer[bufferHead].timestamp = millis();
    buffer[bufferHead].accelX = ax;
    buffer[bufferHead].accelY = ay;
    buffer[bufferHead].accelZ = az;
    buffer[bufferHead].linX = lx;
    buffer[bufferHead].linY = ly;
    buffer[bufferHead].linZ = lz;
    buffer[bufferHead].gyroX = gx;
    buffer[bufferHead].gyroY = gy;
    buffer[bufferHead].gyroZ = gz;
    
    bufferHead = (bufferHead + 1) % BUFFER_SIZE;

    if (bufferCount < BUFFER_SIZE) {
        bufferCount++;
    }
}

bool SensorController::isMotionActive() {
    float accelMag = calculateAccelMagnitude();
    float gyroMag = calculateGyroMagnitude();

    return (abs(accelMag - 1000.0f) > motionStartThreshold) || (gyroMag / 1000 > 50.0f);
}

float SensorController::calculateAccelMagnitude() {
    int64_t ax = accel[0];
    int64_t ay = accel[1];
    int64_t az = accel[2];
    return sqrt(ax*ax + ay*ay + az*az);
}

float SensorController::calculateGyroMagnitude() {
    int64_t gx = gyro[0];
    int64_t gy = gyro[1];
    int64_t gz = gyro[2];
    return sqrt(gx*gx + gy*gy + gz*gz); 
}

void SensorController::analyzeGesture() {
    if (bufferCount < 10) {
        if (debugOutput) Serial.println("!  Insufficient data");
        return;
    }
    
    GestureType detected = GESTURE_NONE;
    
    detected = detectTap();
    if (currentGesture != GESTURE_NONE) {
        addGestureToHistory(currentGesture);
    }
    if (detected != GESTURE_NONE) {
        
        currentGesture = detected;
        newGestureFlag = true;
        return;
    }
    
    detected = detectThrust();
    if (detected != GESTURE_NONE) {
        currentGesture = detected;
        newGestureFlag = true;
        return;
    }
    
}

GestureType SensorController::detectTap() {
    int32_t maxPos = 0;
    int32_t maxNeg = 0;
    int64_t totalGyro = 0;
    int64_t avgZ = 0;

    int startIndex = (bufferHead - bufferCount + BUFFER_SIZE) % BUFFER_SIZE;
    for (int i = 0; i < bufferCount; i++) {
        int index = (startIndex + i) % BUFFER_SIZE;
        int nextIndex = (index + 1) % BUFFER_SIZE;

        if (buffer[index].linY < maxNeg){
            maxNeg = buffer[index].linY;
        }

        if(buffer[nextIndex].linY > maxPos) {
            maxPos = buffer[nextIndex].linY;
        }
        avgZ += abs(buffer[nextIndex].accelZ - buffer[nextIndex].linZ);
        int64_t gx = buffer[index].gyroX;
        int64_t gy = buffer[index].gyroY;
        int64_t gz = buffer[index].gyroZ;
        totalGyro += sqrt(gx*gx + gy*gy + gz*gz);
    }
    avgZ /= bufferCount;
    
    // Calculate average gyro - taps should have minimal rotation
    float avgGyro = totalGyro / bufferCount;
    
    // Taps should be brief
    bool isTapLike = (avgZ < 700) && (avgGyro / 10.0 < 5000);
    if (!isTapLike) {
        return GESTURE_NONE; 
    }

    if (maxPos - maxNeg >= hardTapThreshold) {
        controller.sparkIdle(255,40,0,80,255,false,16,EASE_OUT_QUADRATIC);
        if (debugOutput) Serial.println("Detected: HARD TAP");
        return GESTURE_HARD_TAP;
    } else if (maxPos - maxNeg >= softTapThreshold) {
        controller.sparkIdle(0,40,255,20,255,false,8,EASE_OUT_QUADRATIC);
        if (debugOutput) Serial.println("Detected: SOFT TAP");
        return GESTURE_SOFT_TAP;
    }

    return GESTURE_NONE;
}

GestureType SensorController::detectThrust() {
    int startIndex = (bufferHead - bufferCount + BUFFER_SIZE) % BUFFER_SIZE;
    
    int midIndex = (startIndex + bufferCount / 2) % BUFFER_SIZE;
    float ay = (buffer[midIndex].accelY - buffer[midIndex].linZ) / 1000.0f; // g
    float az = (buffer[midIndex].accelZ - buffer[midIndex].linZ) / 1000.0f; // g
    
    // Calculate angle from vertical pos
    float tiltAngle = atan2(abs(az), abs(ay)) * (180.0f / PI);
    
    bool isUpright = tiltAngle < 45.0f; 
    
    if (isUpright) {
        // VERTICAL THRUST: Staff upright, pushed forward
        // Look for Z-axis linear acceleration pulse
        float maxZAccel = 0;
        bool hasZPulse = false;
        
        for(int i = 0; i < bufferCount - 1; i++){
            int index = (startIndex + i) % BUFFER_SIZE;
            int nextIndex = (index + 1) % BUFFER_SIZE;
            
            int32_t linZ = buffer[index].linZ;
            int32_t nextLinZ = buffer[nextIndex].linZ;
            
            if (abs(linZ) > abs(maxZAccel)) {
                maxZAccel = linZ;
            }
            
            if (abs(linZ) > verticalThrustThreshold && 
                abs(nextLinZ) < abs(linZ)) {
                hasZPulse = true;
            }
        }
        
        if (hasZPulse) {
            if (debugOutput) Serial.println("Detected: VERTICAL THRUST (upright, forward)");
            controller.sparkIdle(255,255,255,255,255,false,16,EASE_EXPONENTIAL);
            return GESTURE_VERTICAL_THRUST;
        }
        
    } else {

        float maxThrust = 0;
        bool hasPulse = false;
        
        for(int i = 0; i < bufferCount - 1; i++){
            int index = (startIndex + i) % BUFFER_SIZE;
            int nextIndex = (index + 1) % BUFFER_SIZE;

            float thrustMag = sqrt(float(buffer[index].linY) * buffer[index].linY + 
                                  float(buffer[index].linZ) * buffer[index].linZ);
            float nextThrustMag = sqrt(float(buffer[nextIndex].linY) * buffer[nextIndex].linY + 
                                       float(buffer[nextIndex].linZ) * buffer[nextIndex].linZ);
            
            if (thrustMag > maxThrust) {
                maxThrust = thrustMag;
            }
            
            // Detect thrust pulse
            if (thrustMag > horizontalThrustThreshold && nextThrustMag < thrustMag) {
                hasPulse = true;
            }
        }
        
        if (hasPulse) {
            if (debugOutput) Serial.println("Detected: HORIZONTAL THRUST (horizontal, forward)");
            controller.sparkIdle(0,255,0,80,255,false,8,EASE_EXPONENTIAL);
            return GESTURE_HORIZONTAL_THRUST;
        }
    }
    
    return GESTURE_NONE;
}



void SensorController::resetGestureState() {
    bufferCount = 0;
    bufferHead = 0;
}

bool SensorController::gestureDetected() {
    return newGestureFlag;
}

GestureType SensorController::getLastGesture() {
    return currentGesture;
}

void SensorController::clearGesture() {
    newGestureFlag = false;
    currentGesture = GESTURE_NONE;
}

void SensorController::getAcceleration(int32_t &x, int32_t &y, int32_t &z) {
    x = accel[0];
    y = accel[1];
    z = accel[2];
}

float prevAccelX = 0, prevAccelY = 0, prevAccelZ = 0;
float prevLinearX = 0, prevLinearY = 0, prevLinearZ = 0;
const float HP_ALPHA = 0.95;  

void SensorController::getLinearAcceleration(int32_t &linX, int32_t &linY, int32_t &linZ) {
    
    // High-pass filter
    linX = HP_ALPHA * (prevLinearX + accel[0] - prevAccelX);
    linY = HP_ALPHA * (prevLinearY + accel[1] - prevAccelY);
    linZ = HP_ALPHA * (prevLinearZ + accel[2] - prevAccelZ);

    prevAccelX = accel[0];
    prevAccelY = accel[1];
    prevAccelZ = accel[2];
    prevLinearX = linX;
    prevLinearY = linY;
    prevLinearZ = linZ;
}

void SensorController::getGyro(int32_t &x, int32_t &y, int32_t &z) {
    x = gyro[0];
    y = gyro[1];
    z = gyro[2];
}

void SensorController::getMag(int32_t &x, int32_t &y, int32_t &z) {
    x = mag[0];
    y = mag[1];
    z = mag[2];
}

float SensorController::getTiltAngle() {
    float ay = accel[1] / 1000.0f;
    float az = accel[2] / 1000.0f;
    return atan2(az, ay) * 180.0 / PI;
}

void SensorController::enableDebugOutput(bool enable) {
    debugOutput = enable;
}

bool SensorController::sequenceDetected() {
    return newSequenceFlag;
}

SequenceType SensorController::getLastSequence() {
    return currentSequence;
}

void SensorController::clearSequence() {
    newSequenceFlag = false;
    currentSequence = SEQ_NONE;
}


void SensorController::addGestureToHistory(GestureType gesture) {
    if (gesture == GESTURE_NONE) return;
    
    uint32_t now = millis();
    

    if (gestureHistoryCount > 0 && (now - lastGestureTime) > sequenceWindowMs) {

        gestureHistoryCount = 0;
        gestureHistoryHead = 0;
    }
    
    gestureHistory[gestureHistoryHead].type = gesture;
    gestureHistory[gestureHistoryHead].timestamp = now;
    
    gestureHistoryHead = (gestureHistoryHead + 1) % GESTURE_HISTORY_SIZE;
    if (gestureHistoryCount < GESTURE_HISTORY_SIZE) {
        gestureHistoryCount++;
    }
    
    lastGestureTime = now;
    
    currentSequence = detectSequence();
    if (currentSequence != SEQ_NONE) {
        newSequenceFlag = true;
        if (debugOutput) {
            Serial.print("Sequence detected: ");
            Serial.println(currentSequence);
        }
    }
}

SequenceType SensorController::detectSequence() {
    if (gestureHistoryCount < 2) return SEQ_NONE;
    

    GestureType fireball[] = {GESTURE_SOFT_TAP, GESTURE_SOFT_TAP, GESTURE_HARD_TAP};
    GestureType necroball[] = {GESTURE_SOFT_TAP, GESTURE_SOFT_TAP, GESTURE_HORIZONTAL_THRUST};

    
    if (matchPattern(fireball, 3)) return SEQ_FIREBALL;
    if (matchPattern(necroball, 3)) return SEQ_NECROBALL;
    
    return SEQ_NONE;
}

bool SensorController::matchPattern(const GestureType* pattern, int patternLength) {
    if (gestureHistoryCount < patternLength) return false;
    
    // Get the most recent gestures
    for (int i = 0; i < patternLength; i++) {
        int bufferIndex = (gestureHistoryHead - patternLength + i + GESTURE_HISTORY_SIZE) % GESTURE_HISTORY_SIZE;
        if (gestureHistory[bufferIndex].type != pattern[i]) {
            return false;
        }
    }
    
    // all gestures should be within the sequence window
    int firstIndex = (gestureHistoryHead - patternLength + GESTURE_HISTORY_SIZE) % GESTURE_HISTORY_SIZE;
    int lastIndex = (gestureHistoryHead - 1 + GESTURE_HISTORY_SIZE) % GESTURE_HISTORY_SIZE;
    
    uint32_t timeDiff = gestureHistory[lastIndex].timestamp - gestureHistory[firstIndex].timestamp;
    if (timeDiff > sequenceWindowMs) {
        return false;
    }
    
    return true;
}