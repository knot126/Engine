#include "array.h"

object_id NewArray(VM vm, size_t initialCap) {
	/**
	 * Create a new array
	 */
	
	VMArray array = New(MakeID(LE_OT_TYPE, LE_OT_ARRAY), sizeof(Array));
	
	if (initialCap) {
		DgAlloc(initialCap * sizeof(object_id));
	}
	
	return MakeID(LE_OT_ID, (size_t)array);
}

static bool VMArrayExtend(VM vm, VMArray array, size_t minSize) {
	/**
	 * Prepare an array to hold at least minSize elements
	 */
	
	size_t newSize = array->cap;
	
	while (newSize < minSize) {
		newSize = 1 + 2 * newSize;
	}
	
	object_id *newData = DgAlloc(newSize * sizeof(object_id));
	
	if (!newData) {
		return false;
	}
	
	if (array->data) {
		memcpy(newData, array->data, array->length * sizeof(object_id));
		DgFree(array->data);
	}
	
	array->cap = newSize;
	array->data = newData;
	
	return true;
}

#define PRE if (!CheckType(this, LE_OT_ARRAY)) { \
		return false; \
	} \
	Array arr = GetPointer(this);

bool VMArrayAppend(VM vm, object_id this, object_id object) {
	/**
	 * Append an object to the array
	 */
	
	PRE
	
	if (arr->cap < arr->length + 1) {
		bool success = ArrayExtend(arr, arr->length + 1);
		
		if (!success) {
			return false;
		}
	}
	
	arr->data[arr->length++] = VMInc(object);
	
	return true;
}

bool VMArrayPushInt(VM vm, object_id this, int64_t value) {
	return VMArrayAppend(vm, this, MakeID(TID_SINT, value));
}

object_id VMArrayGet(VM vm, object_id this, size_t index) {
	PRE
	
	if (index < arr->length) {
		return VMInc(arr->data[index]);
	}
	else {
		return OID_NIL;
	}
}


bool VMArrayDelete(VM vm, object_id this, size_t starting_index, size_t count) {
	PRE
	
	if (starting_index + count > arr->length) {
		return false;
	}
	
	// Decrement the refcounts
	for (size_t i = starting_index; i < starting_index + count; i++) {
		VMDec(vm, arr->data[i]);
	}
	
	// Copy data
	memmove(&arr->data[starting_index], &arr->data[starting_index + count], (arr->length - count) * sizeof(object_id));
	
	return true;
}

#undef PRE
