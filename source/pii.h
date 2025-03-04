/**
 * Trestle Engine Copyright (C) 2025 Knot126
 * -----------------------------------------
 * 
 * The Trestle Plugable Interpreter Interface (PII) provides a method of
 * abstracting away scripting language details so I can make a fucking game
 * using Lua while developing my own custom language. Its not optimal, but I
 * think this might be a good way to solve the problem of "I'd like to make a
 * game" and just wanting to use Lua but also wanting to make my own scripting
 * langauge.
 */

#pragma once
#include "common.h"

// Basic types
typedef bool PBool;
typedef int64_t PInt;
typedef float PFloat;
typedef const char *PString;

typedef enum {
	P_NIL,
	P_BOOL,
	P_INT,
	P_FLOAT,
	P_STRING,
} PType;

typedef struct {
	union {
		PBool asBool;
		PInt asInt;
		PFloat asFloat;
		PString asString;
	};
	PType type;
} PValue;

#define PSTACK_MAX 32

typedef struct {
	PValue data[PSTACK_MAX];
	uint16_t top;
} PStack;

typedef struct PScript PScript;

// Native function types
typedef void (*PNativeFunction)(PScript *script, PStack *args, PStack *rets);

// Function wrappers which types of scripting languages will implement
typedef void *(*PCreateScriptInstance)(void);
typedef void (*PDeleteScriptInstance)(void *instance);
typedef PString (*PRunCode)(PScript *script, PString code);
typedef PValue (*PGetGlobalValue)(PScript *script, PString name);
typedef bool (*PSetGlobalValue)(PScript *script, PString name, PValue value);
typedef bool (*PRegisterFunction)(PScript *script, PString name, PNativeFunction func);

typedef struct {
	PCreateScriptInstance create;
	PDeleteScriptInstance delete;
	PRunCode run;
	PGetGlobalValue getGlobal;
	PSetGlobalValue setGlobal;
	PRegisterFunction registerFunction;
} PWrappers;

// Generic script interface struct
typedef struct PScript {
	void *context;
	PWrappers *wrappers;
} PScript;

void PRegisterScriptType(PString suffix, PWrappers *wrappers);
