#include "LEDDriverI2CInterface.h"

#include "error.h"
#include "i2cMaster.h"

#include <stdbool.h>
#include <string.h> // for memcpy

// 37 bytes: register address + max 36 data packets
static uint8_t g_data[37];
static uint8_t g_address = 0b01111000; // I2C address of LED driver

const uint8_t REG_SHUTDOWN          = 0x00;
const uint8_t REG_PWM               = 0x01;
const uint8_t REG_UPDATE            = 0x25;
const uint8_t REG_CONTROL           = 0x26;
const uint8_t REG_GLOBAL_CONTROL    = 0x4A;
const uint8_t REG_RESET             = 0x4F;

static bool isValidRange(uint8_t startIndex, uint8_t length)
{
    return startIndex + length <= 36;
}

static bool setRegisters(uint8_t registerAddress, uint8_t startIndex, uint8_t* data, uint8_t dataLen)
{
    if (!isValidRange(startIndex, dataLen))
        fatal(ERROR_LED_DRIVER_INVALID_REG_RANGE);

    if (isTransmissionInProgress())
        return false;

    g_data[0] = registerAddress + startIndex;
    // Copy given data into our global data array
    memcpy(g_data + 1, data, dataLen);

    bool conflict = !transmitI2CMaster(g_address, g_data, dataLen + 1);

    // We checked if a transmission was in progress above - if a transmission
    // started between then and now, it must have been started by an ISR, which
    // is not allowed
    if (conflict)
        fatal(ERROR_LED_DRIVER_TRANSMISSION_CONFLICT);

    return true;
}

static bool setRegister(uint8_t registerAddress, uint8_t data)
{
    uint8_t dataArr[1];
    dataArr[0] = data;
    return setRegisters(registerAddress, 0, dataArr, 1);
}

bool LEDDriver_setShutdown(bool shutdown)
{
    return setRegister(REG_SHUTDOWN, shutdown ? 0 : 1);
}

bool LEDDriver_setPWM(uint8_t startIndex, uint8_t* pwm, uint8_t pwmLen)
{
    return setRegisters(REG_PWM, startIndex, pwm, pwmLen);
}

bool LEDDriver_flushChanges()
{
    return setRegister(REG_UPDATE, 0);
}

bool LEDDriver_setControl(uint8_t startIndex, uint8_t* control, uint8_t controlLen)
{
    return setRegisters(REG_CONTROL, startIndex, control, controlLen);
}

bool LEDDriver_setGlobalEnable(bool enable)
{
    return setRegister(REG_GLOBAL_CONTROL, enable ? 0 : 1);
}

bool LEDDriver_reset()
{
    return setRegister(REG_RESET, 0);
}
