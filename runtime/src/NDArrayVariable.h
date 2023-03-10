#pragma once

/**
 * This file defines operations on array variable/type with typeid = TYPEID_NDARRAY
 * mostly adding to the NDArray 1) some interfaces for the runtime variable/type and 2) to allow array variable index reference
 *
 * An array type is used for three types of variables
 * 1. A mixed type vector/matrix (but not a scalar) will be represented as a mixed array with the features
 * - isRef = false
 * - isSelfRef = false
 * - elementTypeID = ELEMENT_MIXED
 * An empty array is a mixed type array with 0 size and nDim=DIM_UNSPECIFIED
 *
 * 2. A concrete array is an array that is not a reference nor a literal
 * - isRef = false
 * - isSelfRef = false
 * - elementTypeID != ELEMENT_MIXED
 *
 * 3. A reference array to one (self-indexed), two(vector) or three(matrix) other array variables
 * - isRef = true
 * - isSelfRef can be true or false
 * - elementTypeID != ELEMENT_MIXED  // a reference can't be a literal or reference to a literal
 * More specifically, a reference array can only reference to concrete array type described in 2)
 * Note 2) and 3) can be string but an array literal is never a string
 */

#include <stdint.h>
#include "Bool.h"
#include "Enums.h"
#include "RuntimeVariables.h"
#include "RuntimeTypes.h"


///------------------------------Compound Type Info: ArrayType---------------------------------------------------------------

typedef struct struct_gazprea_array_type {
    ElementTypeID m_elementTypeID;  // type of the element
    int8_t m_nDim;                  // # of dimensions
    int64_t *m_dims;                // each int64_t specify the length of array in one dimension
    int32_t *m_refCount;              // the number of times m_data is pointed to; determines if we are able to free m_data in destructor
    bool m_isString;
    bool m_isRef;                     // if the array is index reference, default to false
    bool m_isSelfRef;                 // if the array is indexed by itself E.g. a[a], default to false
} ArrayType;

/// allocate
ArrayType *arrayTypeMalloc();
/// constructor
void arrayTypeInitFromVectorSize(ArrayType *this, ElementTypeID elementTypeID, int64_t vecLength, bool isString);
void arrayTypeInitFromMatrixSize(ArrayType *this, ElementTypeID elementTypeID, int64_t dim1, int64_t dim2);  // dim1 is #rows
void arrayTypeInitFromCopy(ArrayType *this, ArrayType *other);
void arrayTypeInitFromCopyByRef(ArrayType *this, ArrayType *other);
void arrayTypeInitFromDims(ArrayType *this, ElementTypeID elementTypeID, int8_t nDim, int64_t *dims,
                           bool isString, int32_t *refCount, bool isRef, bool isSelfRef);
/// destructor
void arrayTypeDestructor(ArrayType *this);

/// methods
int32_t arrayTypeGetReferenceCount(ArrayType *this);
void arrayTypeDecReferenceCount(ArrayType *this);
void arrayTypeIncReferenceCount(ArrayType *this);
VecToVecRHSSizeRestriction arrayTypeMinimumCompatibleRestriction(ArrayType *this, ArrayType *target);
bool arrayTypeHasUnknownSize(ArrayType *this);
int64_t arrayTypeElementSize(ArrayType *this);
int64_t arrayTypeGetTotalLength(ArrayType *this);


///------------------------------Type---------------------------------------------------------------

void typeInitFromVectorSizeSpecificationFromLiteral(Type *this, int64_t size, Type *baseType);  // for vector and string
void typeInitFromMatrixSizeSpecificationFromLiteral(Type *this, int64_t nRow, int64_t nCol, Type *baseType);
void typeInitFromArrayType(Type *this, bool isString, ElementTypeID eid, int8_t nDim, int64_t *dims);
void typeInitFromNDArray(Type *this, ElementTypeID eid, int8_t nDim, int64_t *dims,
                         bool isString, int32_t *refCount, bool isRef, bool isSelfRef);

bool typeIsSpecifiable(Type *this);  // basic types and string
bool typeIsScalar(Type *this);
bool typeIsScalarBasic(Type *this);
bool typeIsScalarNull(Type *this);
bool typeIsScalarIdentity(Type *this);
bool typeIsScalarInteger(Type *this);
bool typeIsArrayNull(Type *this);
bool typeIsArrayIdentity(Type *this);
bool typeIsArrayOrString(Type *this);
bool typeIsVectorOrString(Type *this);
bool typeIsMixedArray(Type *this);
bool typeIsEmptyArray(Type *this);
bool typeIsIntegerVector(Type *this);
bool typeIsArraySameTypeSameSize(Type *this, Type *other);  // return true only is both are array and both have same element type and size

int8_t typeGetNDArrayNDims(Type *this);  // return DIM_INVALID on non-ndarray variables, DIM_UNSPECIFIED on empty array
NDArrayTypeID typeGetNDArrayTypeID(Type *this);

///------------------------------Variable---------------------------------------------------------------

void variableInitFromArrayIndexingHelper(Variable *this, Variable *arr, Variable *rowIndex, Variable *colIndex, int64_t nIndex);
void variableInitFromNDArrayIndexRefToValue(Variable *this, Variable *ref);
void variableInitFromVectorIndexing(Variable *this, Variable *arr, Variable *index);                        /// INTERFACE
void variableInitFromMatrixIndexing(Variable *this, Variable *arr, Variable *rowIndex, Variable *colIndex); /// INTERFACE
void variableNDArrayDestructor(Variable *this);  // called in variableFree()

void variableInitFromNDArrayCopy(Variable *this, Variable *other);
void variableInitFromNDArrayCopyByRef(Variable *this, Variable *other);
Variable *variableNDArrayIndexRefGetRootVariable(Variable *indexRef);

void *variableNDArrayGet(Variable *this, int64_t pos);
void *variableNDArrayCopyGet(Variable *this, int64_t pos);
void variableNDArraySet(Variable *this, int64_t pos, void *val);
NDArrayIndexRefTypeID variableGetIndexRefTypeID(Variable *this);