#include "common.h"

typedef void *vm_context;
typedef uint64_t object_id;

typedef enum : uint8_t {
	VM_NOP, // nop
	VM_CALL, // call function
	VM_METHOD, // call method
	VM_RET, // <nresults>
	VM_LIT, // <literal id>
	VM_CMP, // compare values
	VM_CMPT, // truth(x) == True
	VM_BR, // branch (possibly conditionally)
	VM_POP, // pop top of stack
	
	VM_STORE, // Store global named variable
	VM_LOAD, // Load global named variable
	VM_STL, // Store global (literal name)
	VM_LDL, // Load global (literal name)
	
	VM_COPY, // copy i-th element to top
	
	VM_NEG, // -a
	VM_TRUTH, // !!a
	VM_NOT, // !a
	VM_BNOT, // ~a
	VM_LEN, // len(a)
	
	VM_ADD, // b + a
	VM_SUB, // b - a
	VM_MUL, // b * a
	VM_DIV, // b / a
	VM_MOD, // b % a (b mod a)
	VM_POW, // b ** a
	VM_BOR, // b | a
	VM_BAND, // b & a
	VM_BXOR, // b ^ a
	VM_SHL, // b << a
	VM_SHR, // b >> a
	VM_ROTL, // b <<< a
	VM_ROTR, // b >>> a
	VM_OR, // b or a
	VM_AND, // b and a
	VM_XOR, // b xor a
	VM_ACS, // a.b
} VMBytecode;

/// PRIMITIVE TYPE IDS ///

// The first level of "un-indirection": common small immutable objects have
// their data stored in the object_id directly. This is called an "inline"
// object for want of a better term. They are technically also primitives.
// Their class IDs must fit in three bits and generally any class ID that can
// fit in 3 or less bits is reserved for them.
#define TID_OBJ    0b000
#define TID_SINT   0b001
#define TID_SSTR   0b010
#define TID_FLOAT  0b011
#define TID_BOOL   0b100
#define TID_TYPE   0b111

// Primitive object types that are allocated, possibly even mutable, get ids >= 8.
#define TID_ARRAY 0b1000
#define TID_DICT  0b1001
#define TID_STRING 0b1010
#define TID_FUNCTION 0b1011
#define TID_NATIVE_FUNCTION 0b1100

// Any other types don't have a type ID, they are objects with type set to
// MakeID(TID_TYPE, TID_TYPE) or an object with that type.

/// HELPERS THAT ARE ACTUALLY MACROS ///

// Get the type ID and value of an object_id
#define ClassOf(x) (x >> 61)
#define ValueOf(x) (x & 0x1fffffffffffffff)

// Make an object_id
#define MakeID(t, v) ((t << 61) | (v & 0x1fffffffffffffff))

// Sign extend object_id
#define SignExtId(x) ((int64_t)(((x >> 60) & 1) ? (0xe000000000000000 | x) : x))

// Get the size of a short string
#define ShortStringSize(x) ((x >> 56) & 0b11111)

// Convert between float object IDs and raw floats
#define RawCast(t, v) (*(t *)(&(v)))
#define FloatToId(x) MakeID(TID_FLOAT, RawCast(uint32_t, x))
#define IdToFloat(x) RawCast(float, x)

// Common object IDs
#define OID_NIL 0
#define OID_FALSE MakeID(TID_BOOL, 0)
#define OID_TRUE MakeID(TID_BOOL, 1)
#define OID_LONG_STRING_TYPE MakeID(TID_TYPE, TID_STRING) // Long string type

// Test if an object ID is falsey, which in the VM is defined to be any object
// which is nil, false or the integer 0.
#define IsFalsey(x) (x == OID_NIL || x == OID_FALSE || x == MakeID(OCLS_SINT, 0))

// Pointer related functions
#define IsPointer(x) (ClassOf(x) == TID_OBJ)
#define GetPointer(x) ((void *)(LE_OVALUE(x) << 3))
#define FromPointer(x) (MakeID(TID_OBJ, (uint64_t)(x >> 3)))

// Check the type of an object on the heap
#define CheckType(x, T) (IsPointer(x) && ((ObjectHeader)GetPointer(x))->type == T)

typedef struct {
	object_id type;
	size_t refs;
} ObjectHeader_;

typedef ObjectHeader_ *ObjectHeader;

typedef void *(*VMAlloc)(void *context, void *block, size_t size);
typedef void (*VMNativeFunction)(VM vm, VMArray args, VMArray rets);

typedef struct {
	ObjectHeader_ header;
	
	// Memory management information
	void *memory_context;
	VMAlloc memory_func;
	
	// Globals
	VMDict globals;
} VM_;

typedef VM_ *VM;

void *VMMemory(VM vm, void *block, size_t size);
void *New(size_t size);
object_id VMInc(object_id object);
object_id VMDec(VM vm, object_id object);
