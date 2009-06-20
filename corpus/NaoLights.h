#ifndef NaoLights_h_DEFINED
#define NaoLights_h_DEFINED

#include "Lights.h"
#include "dcmproxy.h"
#include "ALLedNames.h"
/**
 *  This class implements LED capability on the Nao robot using the DCM in Naoqi
 *  @author Johannes Strom
 *  @date June 2009
 */

class NaoLights : public Lights{
public:
    NaoLights(AL::ALPtr<AL::ALBroker> broker);
    virtual ~NaoLights();

public:
    void setRGB(std::string led_id, int rdbHex);

    void sendLights();
private:
    void initDCMAliases();
    void initDCMCommands();
    const float getColor(const ALNames::LedColor c, const int rgbHex);
    void updateLightCommand(ALValue &command, const int rgbHex,
                     const unsigned int numRGBLeds);
private:
    AL::ALPtr<AL::DCMProxy> dcmProxy;
    AL::ALValue leftFaceLedCommand;

};

#endif
