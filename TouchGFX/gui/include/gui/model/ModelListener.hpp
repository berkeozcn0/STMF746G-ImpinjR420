#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

class ModelListener
{
public:
    ModelListener() : model(0) {}
    virtual ~ModelListener() {}

    virtual void updateAntenna1Epc(char* epc) {}
    virtual void updateAntenna2Epc(char* epc) {}
    virtual void updateStatusText(char* status) {}

    void bind(Model* m) { model = m; }
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
