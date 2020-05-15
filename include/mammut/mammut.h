/**
 * (Partial) C interface for Mammut.
 **/
#ifndef MAMMUT_MAMMUT_H_
#define MAMMUT_MAMMUT_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C"{
#endif

struct MammutHandle;
typedef struct MammutHandle MammutHandle;

struct MammutEnergyHandle;
typedef struct MammutEnergyHandle MammutEnergyHandle;

typedef double MammutEnergyJoules;

struct MammutEnergyCounter;
typedef struct MammutEnergyCounter MammutEnergyCounter;

struct MammutEnergyCounterCpus;
typedef struct MammutEnergyCounterCpus MammutEnergyCounterCpus;

struct MammutTopologyHandle;
typedef struct MammutTopologyHandle MammutTopologyHandle;

struct MammutCpuFreqHandle;
typedef struct MammutCpuFreqHandle MammutCpuFreqHandle;


struct MammutCpuHandle ;
typedef struct MammutCpuHandle MammutCpuHandle ;


//TODO Copy-pasted from energy.hpp. Ugly solution, but I do not know how to do it better atm
typedef enum{
    COUNTER_CPUS = 0,// Power measured at CPU level
    COUNTER_PLUG, // Power measured at the plug
}MammutEnergyCounterType;

/** Class Mammut */
MammutHandle* createMammut();
void destroyMammut(MammutHandle* m);
MammutEnergyHandle* getInstanceEnergy(MammutHandle* m);
MammutCpuFreqHandle* getInstanceCpuFreq(MammutHandle* m);
MammutTopologyHandle* getInstanceTopology(MammutHandle* m);

/** Class Energy */
MammutEnergyCounter* getCounter(MammutEnergyHandle* e);
MammutEnergyCounter* getCounterByType(MammutEnergyHandle* e, MammutEnergyCounterType ct);
void getCountersTypes(MammutEnergyHandle* e, MammutEnergyCounterType** cts, size_t* ctsSize);

/** Class Counter */
MammutEnergyJoules getJoules(MammutEnergyCounter* c);
void reset(MammutEnergyCounter* c);
MammutEnergyCounterType getType(MammutEnergyCounter*c);

/** Class CounterCpus */
MammutEnergyJoules getJoulesCpuAll(MammutEnergyCounterCpus* c);
MammutEnergyJoules getJoulesCpu(MammutEnergyCounterCpus* c, MammutCpuHandle* cpu);
MammutEnergyJoules getJoulesCoresAll(MammutEnergyCounterCpus* c);
MammutEnergyJoules getJoulesCores(MammutEnergyCounterCpus* c, MammutCpuHandle* cpu);
int hasJoulesGraphic(MammutEnergyCounterCpus* c);
MammutEnergyJoules getJoulesGraphicAll(MammutEnergyCounterCpus* c);
MammutEnergyJoules getJoulesGraphic(MammutEnergyCounterCpus* c, MammutCpuHandle* cpu);
int hasJoulesDram(MammutEnergyCounterCpus* c);
MammutEnergyJoules getJoulesDramAll(MammutEnergyCounterCpus* c);
MammutEnergyJoules getJoulesDram(MammutEnergyCounterCpus* c, MammutCpuHandle* cpu);

/** Class Topology */
void getCpus(MammutTopologyHandle* e, MammutCpuHandle*** cts, size_t* ctsSize);

/** Clas CPU */
const char* getFamily(MammutCpuHandle*) ;
const char* getModel(MammutCpuHandle*) ;
int getCpuId(MammutCpuHandle*) ;


/** Class CpuFreq */
void enableBoosting(MammutCpuFreqHandle* cf);
void disableBoosting(MammutCpuFreqHandle* cf);
int isBoostingEnabled(MammutCpuFreqHandle* cf);
int isBoostingSupported(MammutCpuFreqHandle* cf);

#ifdef __cplusplus
}
#endif

#endif
