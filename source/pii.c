#include "common.h"
#include "pii.h"

#define MAX_STYPES 2
struct {
	PString suffix;
	PWrappers *wrappers;
} gScriptTypes[MAX_STYPES];

void PRegisterScriptType(PString suffix, PWrappers *wrappers) {
	/**
	 * Register a scripting language along with its suffix
	 */
	
	for (size_t i = 0; i < MAX_STYPES; i++) {
		if (gScriptTypes[i].suffix == NULL) {
			gScriptTypes[i].suffix = suffix;
			gScriptTypes[i].wrappers = wrappers;
			break;
		}
	}
}
