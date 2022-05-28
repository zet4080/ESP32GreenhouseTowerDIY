#ifndef RELAYS_HPP
#define RELAYS_HPP
#include <defines.hpp>

class Relays
{
public:
    Relays();
    virtual ~Relays();
    void SetupRelays();
    void RelayOnOff(int relay, bool on, long double delay = 0.1L);
    void SetupPID();
#if USE_SHT31_SENSOR
    void HumRelayOnOff();
#endif // USE_SHT31_SENSOR

private:
};
extern Relays Relay;
#endif