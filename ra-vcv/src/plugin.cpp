#include "rack.hpp"

using namespace rack;

Plugin *pluginInstance;
extern Model *modelRaVca;
extern Model *modelRaGnawbz;
extern Model *modelRaMacro;
extern Model *modelRaUlfo;
extern Model *modelRaScaler;
extern Model *modelRaEndless;
extern Model *modelRaGlitch;
extern Model *modelRaShapes;
extern Model *modelRaTyche;
extern Model *modelRaAdd;
extern Model *modelRaSeer;
extern Model *modelRaBlank;
extern Model *modelRaAdsr;
extern Model *modelRaKlock;
extern Model *modelRaButtons;
extern Model *modelRaM2;
extern Model *modelRaMagus;
extern Model *modelRaNtet;
extern Model *modelRaThink;
extern Model *modelRaAccumulator;
extern Model *modelRaZeno;
extern Model *modelRaDscope;
extern Model *modelRaChord;
extern Model *modelRaTracker;
extern Model *modelRaJust;
extern Model *modelRaQuant;

void init(Plugin *p) {
    pluginInstance = p;
    p->addModel(modelRaVca);
    p->addModel(modelRaGnawbz);
    p->addModel(modelRaMacro);
    p->addModel(modelRaUlfo);
    p->addModel(modelRaScaler);
    p->addModel(modelRaEndless);
    p->addModel(modelRaGlitch);
    p->addModel(modelRaShapes);
    p->addModel(modelRaTyche);
    p->addModel(modelRaAdd);
    p->addModel(modelRaSeer);
    p->addModel(modelRaBlank);
    p->addModel(modelRaAdsr);
    p->addModel(modelRaKlock);
    p->addModel(modelRaButtons);
    p->addModel(modelRaM2);
    p->addModel(modelRaMagus);
	p->addModel(modelRaNtet);
	p->addModel(modelRaThink);
	p->addModel(modelRaAccumulator);
	p->addModel(modelRaZeno);
	p->addModel(modelRaDscope);
	p->addModel(modelRaChord);
	p->addModel(modelRaTracker);
	p->addModel(modelRaJust);
	p->addModel(modelRaQuant);
}
