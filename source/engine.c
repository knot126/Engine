#include "util/error.h"
#include "util/melon.h"
// #include "util/storage_filesystem.h"
#include "assets.h"
#include "asset_text.h"

#include "engine.h"

Engine *gEngine;

DgError EngineInit(Engine *this, DgArgs *args) {
	DgInitTime();
	
	AssetManagerInit(&this->assman);
	AssetManagerSetSource(&this->assman, ASSET_SOURCE_FOLDER, "assets");
	RegisterTextAssetTypeAndLoader(&this->assman);
	
	DgWindowInit(&this->window, "New Engine", (DgVec2I) {1280, 720});
	
	ZnContextCreateFromWindow(&this->roc, &this->window);
	
	DgTexture foxTexture;
	bool succ = DgTextureLoadQOI(&foxTexture, "assets/vulpes.qoi");
	
	if (!succ) {
		DgLog(DG_LOG_ERROR, "foxy fail 3:");
		return DG_ERROR_FAILED;
	}
	
	this->fox = ZnUploadTexture(&this->roc, &foxTexture, 0);
	
	if (!this->fox) {
		DgLog(DG_LOG_ERROR, "mega foxy fail 3:");
		return DG_ERROR_FAILED;
	}
	
	DgTextureFree(&foxTexture);
	
	this->frames = 0;
	
	return DG_ERROR_SUCCESS;
}

const char *gMainScriptPath = "main.script";

void EngineLoadMainScene(Engine *this) {
	Text mainScriptText = LoadText(&this->assman, gMainScriptPath);
	
	DgLog(DG_LOG_INFO, "Loaded main text asset: %s (size = %d)", gMainScriptPath, mainScriptText->size);
}

DgError EngineRun(Engine *this) {
	DgError err;
	
	EngineLoadMainScene(this);
	
	while (!DgWindowShouldClose(&this->window)) {
		double start = DgTime();
		
		DgVec2I pos = DgWindowGetMouseLocation(&this->window);
		DgVec2I wsize = DgWindowGetSize(&this->window);
		
		ZnDrawBegin(&this->roc);
		
		if ((err = ZnDrawRect(&this->roc, 
			(DgVec2){ 640.0f,  360.0f},
			(DgVec2){ 600.0f,  600.0f}, this->fox))) {
			DgLog(DG_LOG_ERROR, "Error while adding verts: %s.", DgErrorString(err));
		}
		
		if ((err = ZnDrawEnd(&this->roc))) {
			DgLog(DG_LOG_ERROR, "Error while finishing draw: %s.", DgErrorString(err));
		}
		
		bool success = DgWindowUpdate(&this->window);
		
		if (!success) {
			break;
		}
		
		this->frames++;
		
		double delta = (DgTime() - start);
		double sleeptime = (1.0/60.0) - delta;
		
		DgSleep(sleeptime);
	}
	
	return DG_ERROR_SUCCESS;
}

int EngineFree(Engine *this) {
	ZnContextDestroy(&this->roc);
	DgWindowFree(&this->window);
	
	return 0;
}

int main(int argc, const char *argv[]) {
	DgError err;
	DgArgs args;
	
	if ((err = DgArgParse(&args, argc, argv))) {
		return 0x01;
	}
	
	gEngine = DgMemoryAllocate(sizeof *gEngine);
	
	if ((err = EngineInit(gEngine, &args))) {
		DgMemoryFree(gEngine);
		return 0x10;
	}
	
	if ((err = EngineRun(gEngine))) {
		DgMemoryFree(gEngine);
		return 0x20;
	}
	
	int ret = EngineFree(gEngine);
	
	DgMemoryFree(gEngine);
	
	return ret;
}
