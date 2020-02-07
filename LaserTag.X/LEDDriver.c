#include "LEDDriver.h"

#include "error.h"
#include "i2cMaster.h"

#include <stdbool.h>
#include <string.h>  // for memcpy

// 37 bytes: register address + max 36 data packets
// TODO investigate whether we need this to be a global variable, or if we can
// efficiently create function-local arrays
static uint8_t g_data[37];
static uint8_t g_address = 0b0111100;  // 7-bit I2C address of LED driver

const uint8_t REG_SHUTDOWN = 0x00;
const uint8_t REG_PWM = 0x01;
const uint8_t REG_UPDATE = 0x25;
const uint8_t REG_CONTROL = 0x26;
const uint8_t REG_GLOBAL_CONTROL = 0x4A;
const uint8_t REG_RESET = 0x4F;

static bool isValidRange(uint8_t start_index, uint8_t length)
{
    return start_index + length <= 36;
}

static void setRegisters(uint8_t register_address, uint8_t start_index, uint8_t* data, uint8_t data_len)
{
    if (!isValidRange(start_index, data_len))
        fatal(ERROR_LED_DRIVER_INVALID_REG_RANGE);

    g_data[0] = register_address + start_index;
    // Copy given data into our global data array
    memcpy(g_data + 1, data, data_len);

    i2cMaster_write(g_address, g_data, data_len + 1);
}

static void setRegister(uint8_t register_address, uint8_t data)
{
    uint8_t data_arr[1];
    data_arr[0] = data;
    setRegisters(register_address, 0, data_arr, 1);
}

void LEDDriver_setShutdown(bool shutdown)
{
    setRegister(REG_SHUTDOWN, shutdown ? 0 : 1);
}

void LEDDriver_setPWM(uint8_t start_index, uint8_t* pwm, uint8_t pwm_len)
{
    setRegisters(REG_PWM, start_index, pwm, pwm_len);
}

void LEDDriver_flushChanges()
{
    setRegister(REG_UPDATE, 0);
}
void LEDDriver_setControl(uint8_t start_index, uint8_t* control, uint8_t control_len)
{
    setRegisters(REG_CONTROL, start_index, control, control_len);
}

void LEDDriver_setGlobalEnable(bool enable)
{
    setRegister(REG_GLOBAL_CONTROL, enable ? 0 : 1);
}

void LEDDriver_reset()
{
    setRegister(REG_RESET, 0);
}
