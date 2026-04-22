#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "cmsis_os.h"

// --- C VE C++ KÖPRÜSÜ ---
extern "C"
{
    extern osMessageQueueId_t epcQueueHandle;
}

// C tarafı ile tam uyumlu olması için 32 byte yapıldı
struct EpcData {
    char hexString[64];
};

Model::Model() : modelListener(0)
{
}

void Model::tick()
{
    EpcData receivedEpc;

    // Kuyruk kontrolü
    if (epcQueueHandle != NULL)
    {
        // Kuyruktan veri çek (0U: Beklemeden)
        if (osMessageQueueGet(epcQueueHandle, &receivedEpc, NULL, 0U) == osOK)
        {
            if (modelListener != 0)
            {
                // Presenter'a gönder
                modelListener->updateEpcText(receivedEpc.hexString);
            }
        }
    }
}
