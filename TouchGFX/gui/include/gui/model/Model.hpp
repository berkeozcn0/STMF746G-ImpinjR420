#ifndef MODEL_HPP
#define MODEL_HPP

class ModelListener;

#include <stdint.h>

#define MAX_EPC_COUNT 20

struct Ant1EpcRecord {
    char epc[60];
    int8_t maxRssi;
};

class Model
{
public:
    Model();
    void bind(ModelListener* listener) { modelListener = listener; }
    void tick();
    void sendCommand(unsigned char cmd);

    const Ant1EpcRecord* getAnt1EpcList() const { return ant1List; }
    uint8_t getAnt1EpcCount() const { return ant1Count; }

    const Ant1EpcRecord* getAnt2EpcList() const { return ant2List; }
    uint8_t getAnt2EpcCount() const { return ant2Count; }

protected:
    ModelListener* modelListener;
    Ant1EpcRecord ant1List[MAX_EPC_COUNT];
    uint8_t ant1Count;
    Ant1EpcRecord ant2List[MAX_EPC_COUNT];
    uint8_t ant2Count;
};

#endif // MODEL_HPP
