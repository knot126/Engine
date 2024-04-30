#pragma once

/**
 * In the engine, an entity is really just a script. Attributes are stored in
 * the script as script-local variables, and behaviour is implemented in the
 * script as functions.
 */
typedef struct {
	TrScript script;
} TrEntity;

/**
 * A scene contains a collection of entities. It should also probably contain
 * some kind of resource manager for *.script resources, honestly.
 */
typedef struct {
	TrEntity entities;
	size_t entity_count;
} TrScene;
