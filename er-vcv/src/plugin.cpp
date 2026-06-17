#include "rack.hpp"

using namespace rack;

Plugin *pluginInstance;
extern Model *modelRaVca;
extern Model *modelRaGnawbz4x;
extern Model *modelRaGnawbz1x4;

void init(Plugin *p) {
    pluginInstance = p;
    p->addModel(modelRaVca);
    p->addModel(modelRaGnawbz4x);
    p->addModel(modelRaGnawbz1x4);
}
