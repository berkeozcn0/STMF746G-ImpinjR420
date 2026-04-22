#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

/**
 * ModelListener, Model'den gelen verileri Presenter'a ileten arayüzdür.
 * C++ tarafındaki veri akışının merkezi köprüsüdür.
 */
class ModelListener
{
public:
    ModelListener() : model(0) {}

    virtual ~ModelListener() {}

    /**
     * Model.cpp'den gelen EPC verisini yakalamak için kullanılan ana fonksiyon.
     * Bu fonksiyon Presenter tarafından 'override' edilerek veri View'a aktarılır.
     */
    virtual void updateEpcText(char* epc) {}

    void bind(Model* m)
    {
        model = m;
    }
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
