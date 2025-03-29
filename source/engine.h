#pragma once

#include "common.h"
#include "assets.h"
#include "util/table.h"
#include "util/args.h"
#include "zinc/zinc.h"

typedef struct Engine {
	DgTable properties;
	
	DgWindow window;
	ZnContext roc;
	uint64_t fox;
	
	AssetManager assman;
	
	size_t frames;
} Engine;

extern Engine *gEngine;

DgError EngineInit(Engine *this, DgArgs *args);
DgError EngineRun(Engine *this);
int EngineFree(Engine *this);
