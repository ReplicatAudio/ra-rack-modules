#include "rack.hpp"

using namespace rack;

Plugin *pluginInstance;
extern Model *modelRaVca;
extern Model *modelRaGnawbz4x;
extern Model *modelRaGnawbz1x4;
extern Model *modelRaUlfo;
extern Model *modelRaScaler;
extern Model *modelRaEndless;
extern Model *modelRaYscope;

void init(Plugin *p) {
    pluginInstance = p;
    p->addModel(modelRaVca);
    p->addModel(modelRaGnawbz4x);
    p->addModel(modelRaGnawbz1x4);
    p->addModel(modelRaUlfo);
    p->addModel(modelRaScaler);
    p->addModel(modelRaEndless);
    p->addModel(modelRaYscope);
}
