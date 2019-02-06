#include "LEDs.h"

#include "LEDDriver.h"
#include "pins.h"
#include "system.h"

#include <xc.h>

void initializeLEDs()
{
    // Set all LEDs off
    // setBarDisplay1(0);
    // setBarDisplay2(0);
}

void flashMuzzleLight()
{
    LATCH_MUZZLE_LED = 0;
    NOP();
    LATCH_MUZZLE_LED = 1;
}

void flashHitLight()
{
    LATCH_HIT_LED = 0;
    NOP();
    LATCH_HIT_LED = 1;
}

void setBarDisplay1(uint16_t bits)
{
    uint8_t data[10];

    for (int i = 0; i < 10; i++)
    {
        data[i] = (bits >> i) & 1;
    }

    LEDDriver_setControl(0, data, 10);
    LEDDriver_flushChanges();
}

void setBarDisplay2(uint16_t bits)
{
    uint8_t data[10];

    for (int i = 0; i < 10; i++)
    {
        // Reverse, as this bar display is installed upside down
        data[10 - i - 1] = (bits >> i) & 1;
    }

    LEDDriver_setControl(10, data, 10);
    LEDDriver_flushChanges();
}

void setHealthDisplay(uint8_t value)
{
    // Shift in zeros from the right and invert
    setBarDisplay1(~(0b1111111111 << value));
}

void setAmmoDisplay(uint8_t value)
{
    // Shift in zeros from the right and invert
    setBarDisplay2(~(0b1111111111 << value));
}

void setRGB1(uint8_t r, uint8_t g, uint8_t b)
{
    // TODO
}

void setRGB2(uint8_t r, uint8_t g, uint8_t b)
{
    // TODO
}

void setRGB3(uint8_t r, uint8_t g, uint8_t b)
{
    // TODO
}

void setRGB4(uint8_t r, uint8_t g, uint8_t b)
{
    // TODO
}

void setRGB5(uint8_t r, uint8_t g, uint8_t b)
{
    // TODO
}
