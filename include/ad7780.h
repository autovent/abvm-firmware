#include <SPI.h>

class AD7780
{
private:
    static constexpr uint8_t kGainMask = 0x04;
    static constexpr uint32_t kOffsetBinaryCodeZero = (1 << 23); // 2^23 is the halfway point

public:
    AD7780() : powerdown_pin(0), use_powerdown(false), m(1), offset(0), b(0) {}
    AD7780(uint32_t powerdown_pin, float m = 1, float offset = 0, float b = 0) : powerdown_pin(powerdown_pin), use_powerdown(true), vref(3.3), m(m), offset(offset), b(b) {}

    void init()
    {
        if (use_powerdown)
        {
            pinMode(powerdown_pin, OUTPUT);
        }

        set_pins_to_idle();
    }

    float read_volts()
    {
        return volts;
    }

    float read()
    {
        return m * (volts + offset) + b;
    }

    /**
     * Check if a measurement is ready, if it is get the value.
     */
    bool update()
    {

        if (is_ready())
        {
            SPI.begin();
            SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2));

            uint32_t value = 0;
            value |= (SPI.transfer(0) & 0xFF) << 16;
            value |= (SPI.transfer(0) & 0xFF) << 8;
            value |= (SPI.transfer(0) & 0xFF) << 0;

            uint8_t code = SPI.transfer(0);

            SPI.endTransaction();
            SPI.end();
            set_pins_to_idle();

            volts = convert_to_volts(value, get_gain(code), vref);
            return true;
        }
        else
        {
            return false;
        }
    }

    void measure()
    {
        digitalWrite(powerdown_pin, HIGH);
        is_measuring = true;
    }

    uint32_t powerdown_pin;
    bool use_powerdown;
    bool is_measuring;
    uint8_t code;

    float volts;
    float vref;

    float m;
    float offset;
    float b;

private:
    bool is_ready()
    {
        return !digitalRead(PIN_SPI_MISO);
    }

    void set_pins_to_idle()
    {
        pinMode(PIN_SPI_MISO, INPUT);
        pinMode(PIN_SPI_SCK, OUTPUT);
        digitalWrite(PIN_SPI_SCK, HIGH);
        if (use_powerdown)
        {
            digitalWrite(powerdown_pin, LOW);
            is_measuring = false;
        }
        else
        {
            is_measuring = true;
        }
    }

    static constexpr float get_gain(uint8_t code)
    {
        return (code & kGainMask) ? 1 : 128;
    }

    // See datasheet page 12
    static constexpr float convert_to_volts(uint32_t x, float gain, float vref)
    {
        return ((static_cast<float>(x) / kOffsetBinaryCodeZero) - 1) * vref / gain;
    }
};