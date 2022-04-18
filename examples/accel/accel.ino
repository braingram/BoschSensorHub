#include <Wire.h>

#include "bhy.h"
#include "firmware/Bosch_PCB_7183_di03_BMI160-7183_di03.2.1.11696_170103.h"

#define BHY_INT_PIN 12

BHYSensor bhi160;

volatile bool intrToggled = false;
bool newAccelerometerData = false;
float accel_x, accel_y, accel_z;
uint8_t status;

bool checkSensorStatus(void);

void bhyInterruptHandler(void)
{
    intrToggled = true;
}
void waitForBhyInterrupt(void)
{
    while (!intrToggled)
        ;
    intrToggled = false;
}

void accelerometerHandler(bhyVector data, bhyVirtualSensor type)
{
    accel_x = data.x;
    accel_y = data.y;
    accel_z = data.z;
    status = data.status;
    newAccelerometerData = true;
}

void setup()
{
    while (!Serial);
    Serial.begin(115200);
    Wire.setPins(6, 8);
    Wire.begin();

    if (Serial)
    {
        Serial.println("Serial working");
    }
    while(!Serial.available());
    Serial.println("setup...");

    attachInterrupt(BHY_INT_PIN, bhyInterruptHandler, RISING);

    bhi160.begin(BHY_I2C_ADDR2);

    // Check to see if something went wrong.
    if (!checkSensorStatus())
        return;

    Serial.println("Sensor found over I2C! Product ID: 0x" + String(bhi160.productId, HEX));

    Serial.println("Uploading Firmware.");
    bhi160.loadFirmware(bhy1_fw);

    if (!checkSensorStatus())
        return;

    intrToggled = false; /* Clear interrupt status received during firmware upload */
    waitForBhyInterrupt();  /* Wait for meta events from boot up */
    Serial.println("Firmware booted");

    /* Install a metaevent callback handler and a timestamp callback handler here if required before the first run */
    bhi160.run(); /* The first run processes all boot events */

    /* Link callbacks and configure desired virtual sensors here */
    if (bhi160.installSensorCallback(BHY_VS_ACCELEROMETER, true, accelerometerHandler))
    {
        Serial.println("Failed to install accelerometer callback");
        checkSensorStatus();

        return;
    }
    else
        Serial.println("Accelerometer callback installed");

    if (bhi160.configVirtualSensor(BHY_VS_ACCELEROMETER, true, BHY_FLUSH_ALL, 25, 0, 0, 0))
    {
        Serial.println("Failed to enable virtual sensor (" + bhi160.getSensorName(
                           BHY_VS_ACCELEROMETER) + "). Loaded firmware may not support requested sensor id.");
    }
    else
        Serial.println(bhi160.getSensorName(BHY_VS_ACCELEROMETER) + " virtual sensor enabled");


    if (checkSensorStatus())
        Serial.println("All ok");

}

void loop()
{
    if (intrToggled)
    {
        intrToggled = false;
        bhi160.run();
        checkSensorStatus();
        if (newAccelerometerData)
        {
            Serial.println(String(accel_x) + "," + String(accel_y) + "," + String(accel_z) + "," + String(status));
            newAccelerometerData = false;
        }
    }
}

bool checkSensorStatus(void)
{
    if (bhi160.status == BHY_OK)
        return true;

    if (bhi160.status < BHY_OK) /* All error codes are negative */
    {
        Serial.println("Error code: (" + String(bhi160.status) + "). " + bhi160.getErrorString(bhi160.status));

        return false; /* Something has gone wrong */
    }
    else /* All warning codes are positive */
    {
        Serial.println("Warning code: (" + String(bhi160.status) + ").");

        return true;
    }

    return true;
}
