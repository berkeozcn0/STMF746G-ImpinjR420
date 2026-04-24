#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "cmsis_os.h"

extern "C" {
    extern osMessageQueueId_t epcQueueHandle;
    extern osMessageQueueId_t statusQueueHandle;
    extern osMessageQueueId_t cmdQueueHandle;
}

/* Must match C-side EpcData exactly (64 bytes) */
struct EpcData {
    char    hexString[60];
    uint8_t antennaId;
    uint8_t _pad[3];
};

/* Must match C-side StatusMsg exactly (64 bytes) */
struct StatusMsg {
    char text[64];
};

Model::Model() : modelListener(0) {}

void Model::tick()
{
    EpcData epc;
    if (epcQueueHandle != NULL)
    {
        while (osMessageQueueGet(epcQueueHandle, &epc, NULL, 0U) == osOK)
        {
            if (modelListener != 0)
            {
                if (epc.antennaId == 2)
                    modelListener->updateAntenna2Epc(epc.hexString);
                else
                    modelListener->updateAntenna1Epc(epc.hexString);
            }
        }
    }

    StatusMsg st;
    if (statusQueueHandle != NULL)
    {
        if (osMessageQueueGet(statusQueueHandle, &st, NULL, 0U) == osOK)
        {
            if (modelListener != 0)
                modelListener->updateStatusText(st.text);
        }
    }
}

void Model::sendCommand(unsigned char cmd)
{
    if (cmdQueueHandle != NULL)
        osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
}
