#pragma once

#include "vm.h"

typedef struct {
	ObjectHeader_ header;
	size_t length;
	size_t cap;
	object_id *data;
} Array_;

typedef struct Array_ *Array;
