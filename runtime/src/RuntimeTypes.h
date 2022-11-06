#pragma once

// forward declaration
typedef struct struct_gazprea_variable Variable;

///------------------------------TYPE---------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "Enums.h"

typedef struct struct_gazprea_type {
    TypeID m_typeId;
    void *m_compoundTypeInfo;  // for compound type only
} Type;

Type *typeMalloc();

void typeInitFromCopy(Type *this, Type *other);
void typeInitFromAssign(Type *this, Type *lhs, Variable *rhs);
void typeInitFromDeclaration(Type *this, Type *lhs, Variable *rhs);
void typeInitFromCast(Type *this, Type *target, Variable *source);
void typeInitFromPromotion(Type *this, Type *target, Variable *source);
void typeInitFromUnaryOp(Type *this, Variable *operand, UnaryOpCode opcode);
void typeInitFromBinaryOp(Type *this, Variable *op1, Variable *op2, BinOpCode opcode);
void typeInitFromTwoSingleTerms(Type *this, Type *first, Type *second);
void typeInitFromParameter(Type *this, Type *parameterType, Variable *invokeValue);  // same as fromDeclaration but allow vec/matrix references
void typeInitFromVectorSizeSpecification(Type *this, int64_t size, Type *baseType);  // for vector and string
void typeInitFromMatrixSizeSpecification(Type *this, int64_t nRow, int64_t nCol, Type *baseType);


void typeDestructor(Type *this);

bool typeIsUnknown(Type *this);
bool typeIsBasicType(Type *this);
bool typeIsLValueType(Type *this);
bool typeIsVectorOrString(Type *this);  // does not include ref type
bool typeIsMatrix(Type *this);  // does not include ref type
bool typeIsIdentical(Type *this, Type *other);  // checks if the two types are describing the same types

int64_t typeGetMemorySize(Type *this);

// debug print to stdout
void typeDebugPrint(Type *this);

///------------------------------COMPOUND TYPE INFO---------------------------------------------------------------
// ArrayType---------------------------------------------------------------------------------------------
typedef struct struct_gazprea_array_or_string_type {
    ElementTypeID m_elementTypeID;  // type of the element
    int8_t m_nDim;                  // # of dimensions
    int64_t *m_dims;                // each int64_t specify the length of array in one dimension
} ArrayType;

/// allocate
ArrayType *arrayTypeMalloc();
/// constructor
void arrayTypeInitFromVectorSize(ArrayType *this, TypeID elementTypeID, int64_t vecLength);
void arrayTypeInitFromMatrixSize(ArrayType *this, TypeID elementTypeID, int64_t dim1, int64_t dim2);  // dim1 is #rows
void arrayTypeInitFromCopy(ArrayType *this, ArrayType *other);
/// destructor
void arrayTypeDestructor(ArrayType *this);
/// methods
int64_t arrayTypeElementSize(ArrayType *this);
int64_t arrayTypeGetTotalLength(ArrayType *this);
bool arrayTypeCanBeLValue(ArrayType *this);

// note given a VectorType object, we can't distinguish between
// 1. vector
// 2. vector reference (vec[vec], mat[vec, scalar] or mat[scalar, vec])
// we need to look at Type.m_typeid to determine which one is the actual type of this variable


// TupleType---------------------------------------------------------------------------------------------

typedef struct struct_gazprea_tuple_type {
    Type *m_fieldTypeArray;
    int64_t *m_strIdToField;  // maps identifier access to position of field in the tuple; of size 2 * m_nField
    int64_t m_nField;
} TupleType;

TupleType *tupleTypeMalloc();

void tupleTypeInitFromTypeAndId(TupleType *this, Type *typeArray, int64_t *idArray);

void tupleTypeDestructor(TupleType *this);