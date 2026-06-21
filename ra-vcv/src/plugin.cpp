#include "rack.hpp"

using namespace rack;

Plugin *pluginInstance;
extern Model *modelRaVca;
extern Model *modelRaGnawbz4x;
extern Model *modelRaGnawbz1x4;
extern Model *modelRaUlfo;
extern Model *modelRaScaler;
extern Model *modelRaEndless;
extern Model *modelRaShapes;
extern Model *modelRaTyche;
extern Model *modelRaAdd;
extern Model *modelRaSeer;
extern Model *modelRaBlank;
extern Model *modelRaAdsr;
extern Model *modelRaKlock;
extern Model *modelRaButtons;
extern Model *modelRaMagus;
extern Model *modelRaChromastretch;

void init(Plugin *p) {
    pluginInstance = p;
    p->addModel(modelRaVca);
    p->addModel(modelRaGnawbz4x);
    p->addModel(modelRaGnawbz1x4);
    p->addModel(modelRaUlfo);
    p->addModel(modelRaScaler);
    p->addModel(modelRaEndless);
    p->addModel(modelRaShapes);
    p->addModel(modelRaTyche);
    p->addModel(modelRaAdd);
    p->addModel(modelRaSeer);
    p->addModel(modelRaBlank);
    p->addModel(modelRaAdsr);
    p->addModel(modelRaKlock);
    p->addModel(modelRaButtons);
    p->addModel(modelRaMagus);
    p->addModel(modelRaChromastretch);
}
