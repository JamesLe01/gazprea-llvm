#include "LLVMGen.h"

namespace gazprea
{

    LLVMGen::LLVMGen(
        std::shared_ptr<SymbolTable> symtab,
        std::shared_ptr<TypePromote> tp,
        std::string &outfile)
        : symtab(symtab), globalCtx(), ir(globalCtx), mod("gazprea", globalCtx), outfile(outfile),
          llvmFunction(&globalCtx, &ir, &mod),
          llvmBranch(&globalCtx, &ir, &mod),
          numExprAncestors(0),
          numVariableDeclarationAncestors(0),
          numReturnStatementAncestors(0),
          tp(tp)
    {
        runtimeTypeTy = llvm::StructType::create(
            globalCtx,
            {
                ir.getInt32Ty(),
                ir.getInt8PtrTy() // llvm does not have void*, the equivalent is int8*
            },
            "RuntimeType");

        runtimeVariableTy = llvm::StructType::create(
            globalCtx,
            {
                runtimeTypeTy->getPointerTo(),
                ir.getInt8PtrTy(), // llvm does not have void*, the equivalent is int8*
                ir.getInt64Ty(),
                ir.getInt8PtrTy() // llvm does not have void*, the equivalent is int8*
            },
            "RuntimeVariable");
        
        runtimeStackItemTy = llvm::StructType::create(
            globalCtx, {
                ir.getInt32Ty(),
                ir.getInt8PtrTy()
            },
            "RuntimeStackItemTy"
        );

        runtimeStackTy = llvm::StructType::create(
            globalCtx, {
                ir.getInt64Ty(),
                ir.getInt64Ty(),
                runtimeStackItemTy->getPointerTo()
            },
            "RuntimeStackTy"
        );

        llvmFunction.runtimeTypeTy = runtimeTypeTy;
        llvmFunction.runtimeVariableTy = runtimeVariableTy;
        llvmFunction.runtimeStackTy = runtimeStackTy;
        llvmFunction.runtimeStackItemTy = runtimeStackItemTy;
        llvmFunction.declareAllFunctions();

        // Declare Global Variables
        for (auto variableSymbol : symtab->globals->globalVariableSymbols) {
            mod.getOrInsertGlobal(variableSymbol->name, runtimeVariableTy->getPointerTo() );
            auto globalVar = mod.getNamedGlobal(variableSymbol->name);
            globalVar->setLinkage(llvm::GlobalValue::InternalLinkage);
            globalVar->setInitializer(llvm::ConstantPointerNull::get(runtimeVariableTy->getPointerTo()));
            variableSymbol->llvmPointerToVariableObject = globalVar;
        }
        mod.getOrInsertGlobal("globalStack", runtimeStackTy->getPointerTo() );
        auto globalVar = mod.getNamedGlobal("globalStack");
        globalVar->setLinkage(llvm::GlobalValue::InternalLinkage);
        globalVar->setInitializer(llvm::ConstantPointerNull::get(runtimeStackTy->getPointerTo()));
        globalStack = globalVar;
    }

    LLVMGen::~LLVMGen() {
        Print();
    }

    void LLVMGen::visit(std::shared_ptr<AST> t) {
        if (t->isNil()) {
            visitChildren(t);
        } else {
            switch (t->getNodeType()) {
            case GazpreaParser::PROCEDURE:
            case GazpreaParser::FUNCTION:
                visitSubroutineDeclDef(t);
                break;
            case GazpreaParser::RETURN:
                numReturnStatementAncestors++;
                visitReturn(t);
                numReturnStatementAncestors--;
                break;
            case GazpreaParser::CONDITIONAL_STATEMENT_TOKEN:
                visitConditionalStatement(t);
                break;
            case GazpreaParser::INFINITE_LOOP_TOKEN:
                viistInfiniteLoop(t);
                break;
            case GazpreaParser::PRE_PREDICATE_LOOP_TOKEN:
                visitPrePredicatedLoop(t);
                break;
            case GazpreaParser::POST_PREDICATE_LOOP_TOKEN:
                visitPostPredicatedLoop(t);
                break;
            case GazpreaParser::ITERATOR_LOOP_TOKEN:
                visitIteratorLoop(t);
                break;
            case GazpreaParser::BooleanConstant:
                visitBooleanAtom(t);
                break;
            case GazpreaParser::CharacterConstant:
                visitCharacterAtom(t);
                break;
            case GazpreaParser::IntegerConstant:
                visitIntegerAtom(t);
                break;
            case GazpreaParser::REAL_CONSTANT_TOKEN:
                visitRealAtom(t);
                break;
            case GazpreaParser::IDENTITY:
                visitIdentityAtom(t);
                break;
            case GazpreaParser::NULL_LITERAL:
                visitNullAtom(t);
                break;
            case GazpreaParser::StringLiteral:
                visitStringLiteral(t);
                break;
            case GazpreaParser::GENERATOR_TOKEN:
                visitGenerator(t);
                break;
            case GazpreaParser::FILTER_TOKEN:
                visitFilter(t);
                break;
            case GazpreaParser::EXPRESSION_TOKEN:
                numExprAncestors++;
                visitExpression(t);
                numExprAncestors--;
                break;
            case GazpreaParser::CAST_TOKEN:
                visitCast(t);
                break;
            case GazpreaParser::TYPEDEF:
                visitTypedef(t);
                break;
            case GazpreaParser::INPUT_STREAM_TOKEN:
                visitInputStreamStatement(t);
                break;
            case GazpreaParser::OUTPUT_STREAM_TOKEN:
                visitOutputStreamStatement(t);
                break;
            case GazpreaParser::UNARY_TOKEN:
                visitUnaryOperation(t);
                break;
            case GazpreaParser::BINARY_OP_TOKEN:
                visitBinaryOperation(t);
                break;
            case GazpreaParser::INDEXING_TOKEN:
                visitIndexing(t);
                break;
            case GazpreaParser::INTERVAL:
                visitInterval(t);
                break;
            case GazpreaParser::CONCAT_TOKEN:
                visitConcatenation(t);
                break;
            case GazpreaParser::CALL_PROCEDURE_FUNCTION_IN_EXPRESSION:
                visitCallSubroutineInExpression(t);
                break;
            case GazpreaParser::CALL_PROCEDURE_STATEMENT_TOKEN:
                visitCallSubroutineStatement(t);
                break;
            case GazpreaParser::IDENTIFIER_TOKEN:
                visitIdentifier(t);
                break;

            case GazpreaParser::VAR_DECLARATION_TOKEN:
                numVariableDeclarationAncestors++;
                visitVarDeclarationStatement(t);
                numVariableDeclarationAncestors--;
                break;
            case GazpreaParser::ASSIGNMENT_TOKEN:
                visitAssignmentStatement(t);
                break;
            case GazpreaParser::UNQUALIFIED_TYPE_TOKEN:
                visitUnqualifiedType(t);
                break;
            case GazpreaParser::PARAMETER_ATOM_TOKEN:
                visitParameterAtom(t);
                break;
            case GazpreaParser::BREAK:
                visitBreak(t);
                break;
            case GazpreaParser::CONTINUE:
                visitContinue(t);
                break;
            case GazpreaParser::TUPLE_LITERAL_TOKEN:
                visitTupleLiteral(t);
                break;
            case GazpreaParser::TUPLE_ACCESS_TOKEN: 
                visitTupleAccess(t);
                break;
            case GazpreaParser::VECTOR_LITERAL_TOKEN:
                visitVectorMatrixLiteral(t);
                break;
            case GazpreaParser::BLOCK_TOKEN:
                visitBlock(t);
                break;
            default: // The other nodes we don't care about just have their children visited
                visitChildren(t);
            }
        }
    }

    void LLVMGen::visitChildren(std::shared_ptr<AST> t) {
        for (auto child : t->children) visit(child);
    }

    llvm::Value* LLVMGen::getStack() { 
        return ir.CreateLoad(runtimeStackTy->getPointerTo(), globalStack);
    }
    
    void LLVMGen::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->symbol);
        std::vector<llvm::Type *> parameterTypes = std::vector<llvm::Type *>(
            subroutineSymbol->orderedArgs.size(), runtimeVariableTy->getPointerTo());

        llvm::Type *returnType = runtimeVariableTy->getPointerTo();
        if (t->children[2]->isNil()) {
            returnType = ir.getVoidTy();
        }
        else if (subroutineSymbol->name == "gazprea.subroutine.main") {
            returnType = ir.getInt32Ty();
        }

        llvm::FunctionType *subroutineTy = llvm::FunctionType::get(
            returnType,
            parameterTypes,
            false);
        auto subroutineLLVMName = subroutineSymbol->name;
        if (subroutineLLVMName == "gazprea.subroutine.main") {
            subroutineLLVMName = "main";
        }

        auto subroutine = llvm::cast<llvm::Function>(mod.getOrInsertFunction(subroutineLLVMName, subroutineTy).getCallee());
        subroutineSymbol->llvmFunction = subroutine;

        if (t->children[3]->getNodeType() == GazpreaParser::SUBROUTINE_EMPTY_BODY_TOKEN) {
            return;
        }

        currentSubroutine = subroutine;
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(globalCtx, "enterSubroutine", currentSubroutine);
        ir.SetInsertPoint(bb);
        if (subroutineSymbol->name == "gazprea.subroutine.main") {
            initializeGlobalVariables();
        }
        visit(t->children[1]);  // Populate Argument Symbol's llvmValue
        initializeSubroutineParameters(subroutineSymbol); 
        
        subroutineSymbol->stackPtr = llvmFunction.call("runtimeStackSave", {getStack()});

        if (t->children[3]->getNodeType() == GazpreaParser::SUBROUTINE_EXPRESSION_BODY_TOKEN) {
            // Subroutine with Expression Body
            if (t->children[2]->isNil()) {
                freeSubroutineParameters(subroutineSymbol);
                ir.CreateRetVoid();
                return;
            }

            visit(t->children[2]);  // Return Type
            llvmSubroutineReturnType = t->children[2]->llvmValue;
            visit(t->children[3]);  // Visit Body

            auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromDeclaration", { runtimeVariableObject, t->children[2]->llvmValue, t->children[3]->children[0]->llvmValue });
            llvmFunction.call("typeDestructThenFree", t->children[2]->llvmValue);
            
            freeExpressionIfNecessary(t->children[3]->children[0]);
            freeSubroutineParameters(subroutineSymbol);
            
            if (subroutineSymbol->name == "gazprea.subroutine.main") {
                auto returnIntegerValue = llvmFunction.call("variableGetIntegerValue", runtimeVariableObject);
                llvmFunction.call("variableDestructThenFree", runtimeVariableObject);
                freeGlobalVariables();
                llvmFunction.call("runtimeStackRestore", {getStack(), subroutineSymbol->stackPtr});
                ir.CreateRet(returnIntegerValue);
            } else {
                llvmFunction.call("runtimeStackRestore", {getStack(), subroutineSymbol->stackPtr});
                ir.CreateRet(runtimeVariableObject);
            }
            return;
        }
        visit(t->children[3]);  // Visit body
    }

    void LLVMGen::visitReturn(std::shared_ptr<AST> t) {
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->subroutineSymbol);
        llvmBranch.hitReturnStat = true;
        
        if (t->children[0]->isNil()) { 
            // Recursively free the current scope of return Statement and all of its enclosing scope (traverse up until subroutineSymbol)
            std::shared_ptr<Scope> temp = t->scope;
            freeSubroutineParameters(subroutineSymbol);
            while (temp->getEnclosingScope()->getScopeName() != "gazprea.scope.global") {
                // The enclosing scope of the subroutine symbol is global variable
                freeAllVariablesDeclaredInBlockScope(std::dynamic_pointer_cast<LocalScope>(temp));
                temp = temp->getEnclosingScope();
            }
            llvmFunction.call("runtimeStackRestore", {getStack(), subroutineSymbol->stackPtr});
            ir.CreateRetVoid();
            return;
        }

        // return expression; statement  
        visit(subroutineSymbol->declaration->children[2]);  // Visit Type
        llvmSubroutineReturnType = subroutineSymbol->declaration->children[2]->llvmValue;
        
        isExpressionToReplaceIdentityNull = t->isExpressionToReplaceIdentityNull;
        visitChildren(t);  // Visit Expression
        isExpressionToReplaceIdentityNull = false;

        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromDeclaration", { runtimeVariableObject, subroutineSymbol->declaration->children[2]->llvmValue, t->children[0]->llvmValue });
        llvmFunction.call("typeDestructThenFree", subroutineSymbol->declaration->children[2]->llvmValue);
        
        freeExpressionIfNecessary(t->children[0]);
        freeSubroutineParameters(subroutineSymbol);
        
        if (subroutineSymbol->name == "gazprea.subroutine.main") {
            auto returnIntegerValue = llvmFunction.call("variableGetIntegerValue", { runtimeVariableObject });

            std::shared_ptr<Scope> temp = t->scope;
            while (temp->getEnclosingScope()->getScopeName() != "gazprea.scope.global") {
                freeAllVariablesDeclaredInBlockScope(std::dynamic_pointer_cast<LocalScope>(temp));
                temp = temp->getEnclosingScope();
            }
            
            llvmFunction.call("variableDestructThenFree", runtimeVariableObject);
            llvmFunction.call("runtimeStackRestore", {getStack(), subroutineSymbol->stackPtr}); 
            freeGlobalVariables();
            
            ir.CreateRet(returnIntegerValue);
        } else {
            // Non-main subroutine

            // Recursively free the current scope of return Statement and all of its enclosing scope (traverse up until subroutineSymbol)
            std::shared_ptr<Scope> temp = t->scope;
            while (temp->getEnclosingScope()->getScopeName() != "gazprea.scope.global") {
                // The enclosing scope of the subroutine symbol is global variable
                freeAllVariablesDeclaredInBlockScope(std::dynamic_pointer_cast<LocalScope>(temp));
                temp = temp->getEnclosingScope();
            }
            llvmFunction.call("runtimeStackRestore", {getStack(), subroutineSymbol->stackPtr});
            ir.CreateRet(runtimeVariableObject);
        }
    }

    void LLVMGen::visitVarDeclarationStatement(std::shared_ptr<AST> t) {
        auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
        if (variableSymbol->isGlobalVariable) {
            return;
        }

        // Handle identity/null in expression in vector/matrix declaration
        auto runtimeIntegerType = llvmFunction.call("typeMalloc", {});
        llvmFunction.call("typeInitFromIntegerScalar", { runtimeIntegerType });
        llvmVarDeclarationLHSType = runtimeIntegerType;
        visit(t->children[0]);
        llvmFunction.call("typeDestructThenFree", runtimeIntegerType);
        llvmVarDeclarationLHSType = nullptr;

        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        if (t->children[2]->isNil()) {
            // No expression => Initialize to null
            auto runtimeTypeObject = t->children[0]->children[1]->llvmValue;
            auto rhs = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromNullScalar", { rhs });
            llvmFunction.call("variableInitFromDeclaration", { runtimeVariableObject, runtimeTypeObject, rhs });
            llvmFunction.call("typeDestructThenFree", runtimeTypeObject);
            llvmFunction.call("variableDestructThenFree", rhs);
        } else if (t->children[0]->getNodeType() == GazpreaParser::INFERRED_TYPE_TOKEN) {
            llvmVarDeclarationLHSType = nullptr;
            visit(t->children[2]);
            auto runtimeTypeObject = llvmFunction.call("typeMalloc", {});
            llvmFunction.call("typeInitFromUnknownType", { runtimeTypeObject });
            llvmFunction.call("variableInitFromDeclaration", {runtimeVariableObject, runtimeTypeObject, t->children[2]->llvmValue});
            variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;
            
            freeExpressionIfNecessary(t->children[2]);
            llvmFunction.call("typeDestructThenFree", runtimeTypeObject);
        } else {
            auto runtimeTypeObject = t->children[0]->children[1]->llvmValue;
            llvmVarDeclarationLHSType = runtimeTypeObject;

            isExpressionToReplaceIdentityNull = t->isExpressionToReplaceIdentityNull;
            visit(t->children[2]);
            isExpressionToReplaceIdentityNull = false;

            llvmFunction.call("variableInitFromDeclaration", {runtimeVariableObject, runtimeTypeObject, t->children[2]->llvmValue});
            variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;

            freeExpressionIfNecessary(t->children[2]);
            llvmFunction.call("typeDestructThenFree", runtimeTypeObject);
        }
        
        t->llvmValue = runtimeVariableObject;
        variableSymbol->llvmPointerToVariableObject = runtimeVariableObject;
    }

    void LLVMGen::visitAssignmentStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto numLHSExpressions = t->children[0]->children.size();
        if (numLHSExpressions == 1) {
            llvmFunction.call("variableAssignment", {t->children[0]->children[0]->llvmValue, t->children[1]->llvmValue});
            freeExpressionIfNecessary(t->children[1]);
            freeExpressionIfNecessary(t->children[0]->children[0]);
            return;
        }
        for (size_t i = 0; i < numLHSExpressions; i++) {
            auto LHSExpressionAtomAST = t->children[0]->children[i];
            auto tupleFieldValue = llvmFunction.call("variableGetTupleField", { t->children[1]->llvmValue, ir.getInt64(i + 1) });
            llvmFunction.call("variableAssignment", { LHSExpressionAtomAST->llvmValue, tupleFieldValue });
            freeExpressionIfNecessary(LHSExpressionAtomAST);
        }
        freeExpressionIfNecessary(t->children[1]);
    }

    void LLVMGen::visitUnqualifiedType(std::shared_ptr<AST> t) {
        auto runtimeTypeObject = llvmFunction.call("typeMalloc", {});

        std::shared_ptr<MatrixType> matrixType;
        std::shared_ptr<TypedefTypeSymbol> typedefTypeSymbol;
        std::shared_ptr<TupleType> tupleType;
        llvm::Value *baseType;
        llvm::Value *dimension1Expression = nullptr;
        llvm::Value *dimension2Expression = nullptr;

        switch(t->type->getTypeId()) {
            case Type::BOOLEAN:
                llvmFunction.call("typeInitFromBooleanScalar", {runtimeTypeObject});
                break;
            case Type::CHARACTER:
                llvmFunction.call("typeInitFromCharacterScalar", {runtimeTypeObject});
                break;
            case Type::INTEGER:
                llvmFunction.call("typeInitFromIntegerScalar", {runtimeTypeObject});
                break;
            case Type::REAL:
                llvmFunction.call("typeInitFromRealScalar", {runtimeTypeObject});
                break;
            case Type::INTEGER_INTERVAL:
                llvmFunction.call("typeInitFromIntegerInterval", {runtimeTypeObject});
                break;
            case Type::STRING:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    if (typedefTypeSymbol->type->isMatrixType()) {
                        matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            visit(matrixType->def->children[1]->children[0]);
                            dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                        } else {
                            dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                        }
                        baseType = llvmFunction.call("typeMalloc", {});
                        llvmFunction.call("typeInitFromUnspecifiedString", {baseType});
                        llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                        llvmFunction.call("typeDestructThenFree", baseType);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                        }
                    } else {
                        llvmFunction.call("typeInitFromUnspecifiedString", runtimeTypeObject);
                    }
                } else {
                    if (t->type->isMatrixType()) {
                        matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            visit(matrixType->def->children[1]->children[0]);
                            dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                        } else {
                            dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                        }
                        baseType = llvmFunction.call("typeMalloc", {});
                        llvmFunction.call("typeInitFromUnspecifiedString", {baseType});
                        llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                        llvmFunction.call("typeDestructThenFree", baseType);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                        }
                    } else {
                        llvmFunction.call("typeInitFromUnspecifiedString", runtimeTypeObject);
                    }
                }
                break;
            case Type::BOOLEAN_1:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromBooleanScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            
            case Type::CHARACTER_1:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromCharacterScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            
            case Type::INTEGER_1:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromIntegerScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);

                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            
            case Type::REAL_1:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromRealScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            
            case Type::BOOLEAN_2:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromBooleanScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            case Type::CHARACTER_2:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromCharacterScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;

            case Type::INTEGER_2:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromIntegerScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            
            case Type::REAL_2:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromRealScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            
            case Type::TUPLE:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    tupleType = std::dynamic_pointer_cast<TupleType>(typedefTypeSymbol->type);
                } else {
                    tupleType = std::dynamic_pointer_cast<TupleType>(t->type);
                }
                auto typeArray = llvmFunction.call("typeArrayMalloc", {ir.getInt64(tupleType->orderedArgs.size())} );
                auto stridArray = llvmFunction.call("stridArrayMalloc", {ir.getInt64(tupleType->orderedArgs.size())} );
                std::shared_ptr<VariableSymbol> argumentSymbol;
                for (size_t i = 0; i < tupleType->orderedArgs.size(); i++) {
                    argumentSymbol = std::dynamic_pointer_cast<VariableSymbol>(tupleType->orderedArgs[i]);
                    visit(argumentSymbol->def);
                    llvmFunction.call("typeArraySet", { typeArray, ir.getInt64(i), argumentSymbol->llvmPointerToTypeObject });
                    if (argumentSymbol->name == "") {
                        llvmFunction.call("stridArraySet", { stridArray, ir.getInt64(i), ir.getInt64(-1) });
                    } else {
                        llvmFunction.call("stridArraySet", { stridArray, ir.getInt64(i), ir.getInt64(symtab->tupleIdentifierAccess.at(argumentSymbol->name)) });
                    }
                }
                llvmFunction.call("typeInitFromTupleType", { runtimeTypeObject, ir.getInt64(tupleType->orderedArgs.size()), typeArray, stridArray });
                
                llvmFunction.call("stridArrayFree", { stridArray });
                for (size_t i = 0; i < tupleType->orderedArgs.size(); i++) {
                    argumentSymbol = std::dynamic_pointer_cast<VariableSymbol>(tupleType->orderedArgs[i]);
                    llvmFunction.call("typeDestructThenFree", argumentSymbol->llvmPointerToTypeObject);
                }
                llvmFunction.call("typeArrayFree", { typeArray });
                break;
        }

        t->llvmValue = runtimeTypeObject;
    }

    void LLVMGen::visitConditionalStatement(std::shared_ptr<AST> t) {
        //Setup 
        auto *ctx = dynamic_cast<GazpreaParser::ConditionalStatementContext*>(t->parseTree);   
        int numChildren = t->children.size();
        //Parent Function 
        llvm::Function* parentFunc = ir.GetInsertBlock()->getParent(); 
        //Initialize If Header and Body
        llvm::BasicBlock* IfHeaderBB = llvm::BasicBlock::Create(globalCtx, "IfHeaderBlock", parentFunc);
        llvm::BasicBlock* IfBodyBB = llvm::BasicBlock::Create(globalCtx, "IfBodyBlock", parentFunc);
        llvm::BasicBlock* ElseIfHeader = llvm::BasicBlock::Create(globalCtx, "ElseIfHeader"); //only use if needed
        llvm::BasicBlock* ElseBlock = llvm::BasicBlock::Create(globalCtx, "ElseBlock"); // only use if needed
        llvm::BasicBlock* Merge = llvm::BasicBlock::Create(globalCtx, "Merge"); //used at end
        // Start inserting at If Header 
        ir.CreateBr(IfHeaderBB);
        ir.SetInsertPoint(IfHeaderBB);
        visit(t->children[0]); 
        // setup branch condition
        auto exprValue = llvmFunction.call("variableGetBooleanValue", {t->children[0]->llvmValue}); //HERE

        freeExpressionIfNecessary(t->children[0]);

        llvm::Value* ifCondition = ir.CreateICmpNE(exprValue, ir.getInt32(0));
        if ((!ctx->elseStatement() && numChildren > 2) || (ctx->elseStatement() && numChildren > 3)) {
            ir.CreateCondBr(ifCondition, IfBodyBB, ElseIfHeader); 
        } else if(ctx->elseStatement()) {
            ir.CreateCondBr(ifCondition, IfBodyBB, ElseBlock); 
        } else {
            ir.CreateCondBr(ifCondition, IfBodyBB, Merge); 
        }
        //If Body
        ir.SetInsertPoint(IfBodyBB);
        llvmBranch.hitReturnStat = false;
        visit(t->children[1]);
        if (!llvmBranch.hitReturnStat){
            ir.CreateBr(Merge);
        }
        llvmBranch.hitReturnStat = false;
        // Handle Arbitrary Number of Else If
        int elseIfIdx = 2; // 0 is if expr  1 is if body 2 starts else if
        llvm::BasicBlock* residualElseIfHeader = nullptr; //we declare next else if header in previous itteration because we need it for the conditional branch
        for (auto elseIfStatement : ctx->elseIfStatement()) {
            (void)elseIfStatement;
            auto elifNode = t->children[elseIfIdx];            
            if(residualElseIfHeader != nullptr) { //first else if 
                ir.SetInsertPoint(residualElseIfHeader);
            }else {
                parentFunc->getBasicBlockList().push_back(ElseIfHeader);
                ir.SetInsertPoint(ElseIfHeader);
            }
            //Fill header
            visit(elifNode->children[0]); 
            auto elseIfExprValue = llvmFunction.call("variableGetBooleanValue", {elifNode->children[0]->llvmValue});
            
            freeExpressionIfNecessary(elifNode->children[0]);
            
            llvm::Value* elseIfCondition = ir.CreateICmpNE(elseIfExprValue, ir.getInt32(0));
            llvm::BasicBlock* elseIfBodyBlock = llvm::BasicBlock::Create(globalCtx, "ElseIfBody", parentFunc);
            // Conditional Branch Out (3 Cases)
            if (!ctx->elseStatement() && elseIfIdx == (numChildren -1)) {           // 1) last else if no else
                ir.CreateCondBr(elseIfCondition, elseIfBodyBlock, Merge); 
            } else if (ctx->elseStatement() && elseIfIdx == (numChildren -2)) {     // 2) last else if w/ else
                ir.CreateCondBr(elseIfCondition, elseIfBodyBlock, ElseBlock);  
            } else {                                                                // 3) another else if
                llvm::BasicBlock* nextElseIfHeaderBlock = llvm::BasicBlock::Create(globalCtx, "nextElseIfHeader", parentFunc);
                ir.CreateCondBr(elseIfCondition, elseIfBodyBlock, nextElseIfHeaderBlock); 
                residualElseIfHeader = nextElseIfHeaderBlock;
            } 
            //Fill Body
            ir.SetInsertPoint(elseIfBodyBlock);
            llvmBranch.hitReturnStat = false;
            visit(elifNode->children[1]);
            if (!llvmBranch.hitReturnStat){
                ir.CreateBr(Merge);
            }
            llvmBranch.hitReturnStat = false;
            elseIfIdx++; //Elif Counter
        } 
        // Else Caluse (Optional) 
        if (ctx->elseStatement()) {
            parentFunc->getBasicBlockList().push_back(ElseBlock);
            int elseIdx = t->children.size()-1;
            auto elseNode = t->children[elseIdx]->children[0]; 
            ir.SetInsertPoint(ElseBlock);
            llvmBranch.hitReturnStat = false;
            visit(elseNode);
            if (!llvmBranch.hitReturnStat) {
                ir.CreateBr(Merge);
            }
            llvmBranch.hitReturnStat = false;
        }
        // Merge
        parentFunc->getBasicBlockList().push_back(Merge); 
        ir.SetInsertPoint(Merge);         
    }
    
    void LLVMGen::visitBreak(std::shared_ptr<AST> t) {
        std::shared_ptr<Scope> temp = t->scope;
        while (true) {
            // The enclosing scope of the subroutine symbol is global variable
            auto localScope = std::dynamic_pointer_cast<LocalScope>(temp);
            freeAllVariablesDeclaredInBlockScope(localScope);
            if (localScope->parentIsLoop) {
                break;
            }
            temp = temp->getEnclosingScope();
        }
        
        int stackSize = llvmBranch.blockStack.size();
        llvm::Function *parentFunc = ir.GetInsertBlock()->getParent();
        llvm::BasicBlock* mergeBB = llvmBranch.blockStack[stackSize -1]; 
        llvm::BasicBlock* breakBlock = llvm::BasicBlock::Create(globalCtx, "BreakBlock", parentFunc);
        llvm::BasicBlock* resumeLoopBody = llvm::BasicBlock::Create(globalCtx, "ResumeLoop", parentFunc);

        ir.CreateBr(breakBlock);
        ir.SetInsertPoint(breakBlock);
        ir.CreateBr(mergeBB);
        ir.SetInsertPoint(resumeLoopBody);
    }

    void LLVMGen::visitContinue(std::shared_ptr<AST> t) {
        std::shared_ptr<Scope> temp = t->scope;
        while (true) {
            // The enclosing scope of the subroutine symbol is global variable
            auto localScope = std::dynamic_pointer_cast<LocalScope>(temp);
            freeAllVariablesDeclaredInBlockScope(localScope);
            if (localScope->parentIsLoop) {
                break;
            }
            temp = temp->getEnclosingScope();
        }

        int stackSize = llvmBranch.blockStack.size();
        llvm::Function *parentFunc = ir.GetInsertBlock()->getParent();
        llvm::BasicBlock* loopHeader = llvmBranch.blockStack[stackSize -3]; 
        llvm::BasicBlock* continueBlock = llvm::BasicBlock::Create(globalCtx, "ContinueBlock", parentFunc);
        llvm::BasicBlock* resumeLoopBody = llvm::BasicBlock::Create(globalCtx, "ResumeLoop", parentFunc);

        ir.CreateBr(continueBlock);
        ir.SetInsertPoint(continueBlock);
        ir.CreateBr(loopHeader);
        ir.SetInsertPoint(resumeLoopBody);
    }
    
    void LLVMGen::viistInfiniteLoop(std::shared_ptr<AST> t) {
        // setup
        llvm::Function *parentFunc = ir.GetInsertBlock()->getParent();
        llvm::BasicBlock *InfiniteBodyBB = llvm::BasicBlock::Create(globalCtx, "InfiniteBody", parentFunc);
        llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(globalCtx, "MergeFromInfinteLoop");
        llvmBranch.blockStack.push_back(InfiniteBodyBB);
        llvmBranch.blockStack.push_back(nullptr); // maintain index offset to other loop methods
        llvmBranch.blockStack.push_back(MergeBB);
        // create infinite loop
        ir.CreateBr(InfiniteBodyBB);
        ir.SetInsertPoint(InfiniteBodyBB);
        llvmBranch.hitReturnStat = false;
        visitChildren(t);
        if (!llvmBranch.hitReturnStat){
            ir.CreateBr(InfiniteBodyBB);
        }
        llvmBranch.hitReturnStat = false;
        //construct merge 
        parentFunc->getBasicBlockList().push_back(MergeBB);
        ir.SetInsertPoint(MergeBB);
        // keep stack organized
        llvmBranch.blockStack.pop_back();
        llvmBranch.blockStack.pop_back();
        llvmBranch.blockStack.pop_back();
    }

    void LLVMGen::visitPrePredicatedLoop(std::shared_ptr<AST> t) {
        llvmBranch.createPrePredConditionalBB("PrePredLoop");
        visit(t->children[0]);      // Conditional Expr
        auto exprValue = llvmFunction.call("variableGetBooleanValue", {t->children[0]->llvmValue});
        freeExpressionIfNecessary(t->children[0]);
        
        llvm::Value* condition = ir.CreateICmpNE(exprValue, ir.getInt32(0)); 
        llvmBranch.createPrePredBodyBB(condition);
        llvmBranch.hitReturnStat = false;
        visit(t->children[1]);      // Visit body
        llvmBranch.createPrePredMergeBB();
    }

    void LLVMGen::visitPostPredicatedLoop(std::shared_ptr<AST> t) {
        llvmBranch.createPostPredBodyBB(); 
        llvmBranch.hitReturnStat = false;
        visit(t->children[0]);      //visit Body  
        llvmBranch.createPostPredConditionalBB(); 
        visit(t->children[1]);      //grab value from post predicate
        auto exprValue = llvmFunction.call("variableGetBooleanValue", {t->children[1]->llvmValue});

        freeExpressionIfNecessary(t->children[1]);
        
        llvm::Value *condition = ir.CreateICmpNE(exprValue, ir.getInt32(0));
        llvmBranch.createPostPredMergeBB(condition);
    }
 
    void LLVMGen::visitIteratorLoop(std::shared_ptr<AST> t) { 
        // stave stack

        llvm::Function *parentFunc = ir.GetInsertBlock()->getParent();
        llvm::BasicBlock *preHeader = llvm::BasicBlock::Create(globalCtx, "IteratorLoopPreHeader", parentFunc);
        ir.CreateBr(preHeader);
        ir.SetInsertPoint(preHeader);

        auto sp = llvmFunction.call("runtimeStackSave", {getStack()});
        
        std::vector<llvm::Value*> domainIndexVars;
        std::vector<llvm::Value*> domainExprs;
        std::vector<llvm::Value*> domainExprSizes;
        std::vector<llvm::Value*> domainVars;

        // Constants for all loops
        auto indexVariableType = llvmFunction.call("typeStackAllocate", {getStack()});
        llvmFunction.call("typeInitFromIntegerScalar", {indexVariableType});

        auto constOne = llvmFunction.call("variableStackAllocate", {getStack()});
        llvmFunction.call("variableInitFromIntegerScalar", {constOne, ir.getInt32(1)});
 
        // auto constZero = llvmFunction.call("variableStackAllocate", {getStack()});
        // llvmFunction.call("variableInitFromIntegerScalar", {constZero, ir.getInt32(0)});

        auto constNegativeOne = llvmFunction.call("variableStackAllocate", {getStack()});
        llvmFunction.call("variableInitFromIntegerScalar", {constNegativeOne, ir.getInt32(-1)});
 
        // Create Preheader and necessary vectors
        for (size_t i = 0; i < t->children.size()-1; i++) {
            // create index variable & set to 0
            auto indexInitialization = llvmFunction.call("variableStackAllocate", {getStack()}); 
            auto indexVariable = llvmFunction.call("variableStackAllocate", {getStack()});
            llvmFunction.call("variableInitFromIntegerScalar", {indexInitialization, ir.getInt32(-1)});
            llvmFunction.call("variableInitFromDeclaration", {indexVariable, indexVariableType, indexInitialization}); 
            domainIndexVars.push_back(indexVariable);

            // Initialize domain expressions & push to vector
            visit(t->children[i]);
            auto domainExpr = t->children[i]->children[1];
            auto runtimeDomainArray = llvmFunction.call("variableStackAllocate", {getStack()});
            llvmFunction.call("variableInitFromDomainExpression", {runtimeDomainArray, domainExpr->llvmValue});
            if (domainExpr->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) { //free is not id
                freeExpressionIfNecessary(domainExpr); 
            } 
            domainExprs.push_back(runtimeDomainArray);

            // Calculate size of each domain array and store in vector 
            llvm::Value *length = llvmFunction.call("variableGetLength", {runtimeDomainArray});
            llvm::Value *truncLength = ir.CreateIntCast(length, ir.getInt32Ty(), true);
            auto lengthVariable = llvmFunction.call("variableStackAllocate", {getStack()});
            llvmFunction.call("variableInitFromIntegerScalar", {lengthVariable, truncLength}); 
            domainExprSizes.push_back(lengthVariable);

            //speculative domain variable declaration to satisfy LLVM dominator constraint
            auto domainVar = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromIntegerScalar", {domainVar, ir.getInt32(0)});
            domainVars.push_back(domainVar);
        } 
        // Initialize Basic Blocks 
        for (size_t i = 0; i < t->children.size()-1; i++) { 
            //make basic blocks and set up inserts 
            auto str_i = std::to_string(i);
            llvm::BasicBlock *header = llvm::BasicBlock::Create(globalCtx, "IteratorLoopHeader" + str_i, parentFunc);
            llvm::BasicBlock* body = llvm::BasicBlock::Create(globalCtx, "IteratorLoopBody" + str_i);
            llvm::BasicBlock* merge = llvm::BasicBlock::Create(globalCtx, "IteratorLoopMerge" + str_i);
            llvmBranch.blockStack.push_back(header);
            llvmBranch.blockStack.push_back(body);
            llvmBranch.blockStack.push_back(merge); 
        }
        // Create Header Blocks
        for (size_t i = 0; i < t->children.size()-1; i++) {
            // Headers will cascade from { header_0, header_1 .. header_i } until the final header: header_n  
            size_t bsSize = llvmBranch.blockStack.size();
            llvm::BasicBlock* branchTrue;
            llvm::BasicBlock* branchFalse;
            int offset = (3*(t->children.size()-1-i));
            if (i == 0) {
                ir.CreateBr(llvmBranch.blockStack[bsSize - offset]);
            }
            if (i == (t->children.size() -2)) {
                //header cond br has true: body n (visit t->children[-1] here or false: merge n)
                auto header_n = llvmBranch.blockStack[bsSize - 3];
                auto body_n   = llvmBranch.blockStack[bsSize - 2];
                auto merge_n  = llvmBranch.blockStack[bsSize - 1];
                ir.SetInsertPoint(header_n);
                branchTrue = body_n;
                branchFalse = merge_n;
            } else {
                //header cond br has true: next header (i + 1) or merge i
                auto header_i    = llvmBranch.blockStack[bsSize - offset];
                auto merge_i     = llvmBranch.blockStack[bsSize - offset + 2];
                auto next_header = llvmBranch.blockStack[bsSize - offset + 3];

                ir.SetInsertPoint(header_i);
                //reset the next header array index and domain variable to initial values 
                auto next_loop_index = domainIndexVars[i+1];
                llvmFunction.call("variableReplace", {next_loop_index, constNegativeOne});
                branchTrue = next_header;
                branchFalse = merge_i;
            }

            //create comparisson between index variable and length of domain vector
            auto indexVariable = domainIndexVars[i];
            auto lengthVariable = domainExprSizes[i];
            incrementIndex(indexVariable, 1); 
            llvm::Value* branchCond = createBranchCondition(indexVariable, lengthVariable);
            ir.CreateCondBr(branchCond, branchTrue, branchFalse);
        }
        // Create Body and Merge Blocks
        for (int i = t->children.size()-2; i >=0; i--) {
            size_t bsSize = llvmBranch.blockStack.size();
            int offset = (3*(t->children.size()-1-i));
            auto header_i   = llvmBranch.blockStack[bsSize - offset];
            auto body_i     = llvmBranch.blockStack[bsSize - offset + 1];
            auto merge_i    = llvmBranch.blockStack[bsSize - offset + 2];
            llvm::BasicBlock* next_body; 
            if (i != 0 ) {
                next_body  = llvmBranch.blockStack[bsSize - offset - 2];
            }
            parentFunc->getBasicBlockList().push_back(body_i);
            ir.SetInsertPoint(body_i);

            // Only fill the body on the inner most loop 
            int numChildren = t->children.size();
            if (i == numChildren-2) {
                for (size_t j = 0; j < t->children.size()-1; j++ ) {
                    auto indexVariable = domainIndexVars[j];
                    auto runtimeDomainArray = domainExprs[j];
                    auto runtimeDomainVar = domainVars[j];
                    llvm::Value* index_i32 = llvmFunction.call("variableGetIntegerValue", {indexVariable});
                    llvm::Value* index_i64 = ir.CreateIntCast(index_i32, ir.getInt64Ty(), false); 
                    
                    //init domain variable & variable symbol
                    auto variableAST = t->children[j]->children[0];
                    initializeDomainVariable(runtimeDomainVar, runtimeDomainArray, index_i64); 
                    initializeVariableSymbol(variableAST, runtimeDomainVar); 
                }
                llvmBranch.hitReturnStat = false;
                visit(t->children[numChildren-1]);
            }
            // close loop if no return statement
            if (!llvmBranch.hitReturnStat){
                ir.CreateBr(header_i);
            }
            llvmBranch.hitReturnStat = false;
            parentFunc->getBasicBlockList().push_back(merge_i);
            ir.SetInsertPoint(merge_i);
            if (i != 0) {
                ir.CreateBr(next_body);
            }
        }
        // clear the block stack
        for (size_t i = 0; i < 3; i++) {  
            llvmBranch.blockStack.pop_back(); 
        }
        for(size_t i = 0; i < t->children.size()-1; i++) {
            llvmFunction.call("variableDestructThenFree", {domainVars[i]});
        }

        llvmFunction.call("runtimeStackRestore", {getStack(), sp});
    }

    void LLVMGen::visitBooleanAtom(std::shared_ptr<AST> t) {
        bool booleanValue = 0;
        if (t->parseTree->getText() == "true") {
            booleanValue = 1;
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBooleanScalar", {runtimeVariableObject, ir.getInt32(booleanValue)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitCharacterAtom(std::shared_ptr<AST> t) {
        char characterValue;
        if (t->parseTree->getText().length() == 4) {
            switch (t->parseTree->getText()[2]) {
                case 'a':
                    characterValue = '\a';
                    break;
                case 'b':
                    characterValue = '\b';
                    break;
                case 'n':
                    characterValue = '\n';
                    break;
                case 'r':
                    characterValue = '\r';
                    break;
                case 't':
                    characterValue = '\t';
                    break;
                case '\"':
                    characterValue = '\"';
                    break;
                case '\'':
                    characterValue = '\'';
                    break;
                case '\\':
                    characterValue = '\\';
                    break;
            }
        } else {
            characterValue = t->parseTree->getText()[1];
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromCharacterScalar", {runtimeVariableObject, ir.getInt8(characterValue)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIntegerAtom(std::shared_ptr<AST> t) {
        auto integerValue = std::stoi(t->parseTree->getText());
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIntegerScalar", {runtimeVariableObject, ir.getInt32(integerValue)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitRealAtom(std::shared_ptr<AST> t) {
        auto realValue = std::stof(t->parseTree->getText());
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromRealScalar", {runtimeVariableObject, llvm::ConstantFP::get(ir.getFloatTy(), realValue)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIdentityAtom(std::shared_ptr<AST> t) {
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        if (numVariableDeclarationAncestors > 0) {
            if (llvmVarDeclarationLHSType != nullptr && isExpressionToReplaceIdentityNull) {
                llvmFunction.call("variableInitFromIdentity", { runtimeVariableObject, llvmVarDeclarationLHSType });
            } else {
                llvmFunction.call("variableInitFromIdentityScalar", {runtimeVariableObject});
            }
        } else if (numReturnStatementAncestors > 0) {
            if (isExpressionToReplaceIdentityNull) {
                llvmFunction.call("variableInitFromIdentity", { runtimeVariableObject, llvmSubroutineReturnType });
            } else {
                llvmFunction.call("variableInitFromIdentityScalar", {runtimeVariableObject});
            }
            
        } else {
            llvmFunction.call("variableInitFromIdentityScalar", {runtimeVariableObject});
        }
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitNullAtom(std::shared_ptr<AST> t) {
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        if (numVariableDeclarationAncestors > 0) {
            if (llvmVarDeclarationLHSType != nullptr && isExpressionToReplaceIdentityNull) {
                llvmFunction.call("variableInitFromNull", { runtimeVariableObject, llvmVarDeclarationLHSType });
            } else {
                llvmFunction.call("variableInitFromNullScalar", {runtimeVariableObject});
            }
        } else if (numReturnStatementAncestors > 0) {
            if (isExpressionToReplaceIdentityNull) {
                llvmFunction.call("variableInitFromNull", { runtimeVariableObject, llvmSubroutineReturnType });
            } else {
                llvmFunction.call("variableInitFromNullScalar", {runtimeVariableObject});
            }
        } else {
            llvmFunction.call("variableInitFromNullScalar", {runtimeVariableObject});
        }
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitStringLiteral(std::shared_ptr<AST> t) {
        visitChildren(t);
        std::string stringChars = unescapeString(t->parseTree->getText().substr(1, t->parseTree->getText().length() - 2));
        auto stringLength = stringChars.length();
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvm::StringRef string = llvm::StringRef(stringChars.c_str());
        llvm::Value* myStr = ir.CreateGlobalStringPtr(string);
        llvmFunction.call("variableInitFromString", { runtimeVariableObject, ir.getInt64(stringLength), myStr});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIdentifier(std::shared_ptr<AST> t) {
        visitChildren(t);
        if (numExprAncestors > 0) {
            auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
            if (variableSymbol != nullptr && variableSymbol->isGlobalVariable) {
                auto globalVar = mod.getNamedGlobal(variableSymbol->name);
                t->llvmValue = ir.CreateLoad(runtimeVariableTy->getPointerTo(), globalVar);
            } else {
                t->llvmValue = t->symbol->llvmPointerToVariableObject;
            }
        }
    }

    void LLVMGen::visitGenerator(std::shared_ptr<AST> t) {  
        if (t->children[0]->children.size() ==  1) { 
            // create basic blocks            
            llvm::Function* parentFunc = ir.GetInsertBlock()->getParent();
            llvm::BasicBlock* preHeader = llvm::BasicBlock::Create(globalCtx, "generatorPreHeader", parentFunc);
            llvm::BasicBlock* header = llvm::BasicBlock::Create(globalCtx, "generatorHeader", parentFunc);
            llvm::BasicBlock* body = llvm::BasicBlock::Create(globalCtx, "generatorBody", parentFunc);
            llvm::BasicBlock* merge = llvm::BasicBlock::Create(globalCtx, "generatorMerge", parentFunc);
            
            // enter the preheader
            ir.CreateBr(preHeader);
            ir.SetInsertPoint(preHeader); 
            
            auto indexVariableType = llvmFunction.call("typeMalloc", {});
            auto indexInitialization = llvmFunction.call("variableMalloc", {});
            auto indexVariable = llvmFunction.call("variableMalloc", {});            
            llvmFunction.call("typeInitFromIntegerScalar", {indexVariableType});
            llvmFunction.call("variableInitFromIntegerScalar", {indexInitialization, ir.getInt32(0)});
            llvmFunction.call("variableInitFromDeclaration", {indexVariable, indexVariableType, indexInitialization}); 
            llvmFunction.call("variableDestructThenFree", {indexInitialization}); 
            
            // get vector size
            visit(t->children[0]); 
            auto domainArray = t->children[0]->children[0]->children[1];
            auto runtimeDomainArray = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromDomainExpression", {runtimeDomainArray, domainArray->llvmValue});
            if (domainArray->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) { //free is not id
                freeExpressionIfNecessary(domainArray); 
            } 
            //init length 
            llvm::Value *length = llvmFunction.call("variableGetLength", {runtimeDomainArray});
            llvm::Value *truncLength = ir.CreateIntCast(length, ir.getInt32Ty(), true);
            auto lengthVariable = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromIntegerScalar", {lengthVariable, truncLength});
            // create the result vector
            auto generatorArray = llvmFunction.call("variableArrayMalloc", {length}); //result vector i
            // move onto header  
            ir.CreateBr(header);
            ir.SetInsertPoint(header);

            // compare current index with length of domain vector    
            llvm::Value *branchCond = createBranchCondition(indexVariable, lengthVariable);
            ir.CreateCondBr(branchCond, body, merge); 
            ir.SetInsertPoint(body);
            // get integer values of current index because some runtime functions require i64 type 
            llvm::Value* index_i32 = llvmFunction.call("variableGetIntegerValue", {indexVariable});
            llvm::Value* index_i64 = ir.CreateIntCast(index_i32, ir.getInt64Ty(), false);

            auto runtimeDomainVar = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromIntegerScalar", {runtimeDomainVar, ir.getInt32(0)});
            initializeDomainVariable(runtimeDomainVar, runtimeDomainArray, index_i64); 
            
            //initialize variable symbol to from variable at current index in domain array
            auto variableAST = t->children[0]->children[0]->children[0]; 
            initializeVariableSymbol(variableAST, runtimeDomainVar);  
            visit(t->children[1]); //evaluate RHS expression with current domain variable value 

            auto exprVar = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromMemcpy", {exprVar, t->children[1]->llvmValue});
            llvmFunction.call("variableArraySet", {generatorArray, index_i64, exprVar}); 
            // free what we can
            llvmFunction.call("variableDestructThenFree", {runtimeDomainVar});
            freeExpressionIfNecessary(t->children[1]);

            //increment the index variable
            incrementIndex(indexVariable, 1); 
            ir.CreateBr(header);
            ir.SetInsertPoint(merge);

            // assign result array to AST
            auto generatorArrayVar = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromVectorLiteral", { generatorArrayVar, length, generatorArray }); 
            t->llvmValue = generatorArrayVar;
            //free mallocs
            llvmFunction.call("variableDestructThenFree", {indexVariable});
            llvmFunction.call("variableDestructThenFree", {lengthVariable});
            llvmFunction.call("variableDestructThenFree", {runtimeDomainArray});
            llvmFunction.call("freeArrayContents", {generatorArray, length});
            llvmFunction.call("variableArrayFree", {generatorArray});
            llvmFunction.call("typeDestructThenFree", {indexVariableType});
 
        } else if (t->children[0]->children.size() == 2) { 
            
            //matrix generator
            llvm::Function* parentFunc = ir.GetInsertBlock()->getParent();
            llvm::BasicBlock* preHeader = llvm::BasicBlock::Create(globalCtx, "generatorPreHeader", parentFunc);
            llvm::BasicBlock* outerHeader = llvm::BasicBlock::Create(globalCtx, "generatorMatrixOuterHeader", parentFunc);
            llvm::BasicBlock* innerPreHeader = llvm::BasicBlock::Create(globalCtx, "generatorInnerPreHeader", parentFunc);
            llvm::BasicBlock* innerHeader = llvm::BasicBlock::Create(globalCtx, "generatorMatrixInnerHeader", parentFunc);
            llvm::BasicBlock* innerBody = llvm::BasicBlock::Create(globalCtx, "generatorMatrixInnerBody", parentFunc);
            llvm::BasicBlock* innerMerge = llvm::BasicBlock::Create(globalCtx, "generatorMatrixInnerMerge", parentFunc);
            llvm::BasicBlock* outerBody = llvm::BasicBlock::Create(globalCtx, "generatorMatrixOuterBody", parentFunc);
            llvm::BasicBlock* outerMerge = llvm::BasicBlock::Create(globalCtx, "generatorMatrixOuterMerge", parentFunc);

            ir.CreateBr(preHeader);
            ir.SetInsertPoint(preHeader);           
            
            visit(t->children[0]); 
            auto outerDomainArray = t->children[0]->children[0]->children[1];
            auto innerDomainArray = t->children[0]->children[1]->children[1];
            auto outerRuntimeDomainArray = llvmFunction.call("variableMalloc", {});
            auto innerRuntimeDomainArray = llvmFunction.call("variableMalloc", {});
            auto constZero = llvmFunction.call("variableMalloc", {});

            llvmFunction.call("variableInitFromIntegerScalar", {constZero, ir.getInt32(0)}); 
            llvmFunction.call("variableInitFromDomainExpression", {outerRuntimeDomainArray, outerDomainArray->llvmValue});
            llvmFunction.call("variableInitFromDomainExpression", {innerRuntimeDomainArray, innerDomainArray->llvmValue});
            if (outerDomainArray->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) { 
                freeExpressionIfNecessary(outerDomainArray); 
            }    
            if (innerDomainArray->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) { 
                freeExpressionIfNecessary(innerDomainArray); 
            }    

            auto indexVariableType = llvmFunction.call("typeMalloc", {});
            auto outerDomainLengthVar = llvmFunction.call("variableMalloc", {}); 
            auto innerDomainLengthVar = llvmFunction.call("variableMalloc", {});
            auto outerIndex = llvmFunction.call("variableMalloc", {});
            auto innerIndex = llvmFunction.call("variableMalloc", {});
            auto indexInitialization = llvmFunction.call("variableMalloc", {});
            auto runtimeOuterDomainVar = llvmFunction.call("variableMalloc", {});
            auto runtimeInnerDomainVar = llvmFunction.call("variableMalloc", {});
            
            llvmFunction.call("variableInitFromIntegerScalar", {runtimeOuterDomainVar, ir.getInt32(0)});
            llvmFunction.call("variableInitFromIntegerScalar", {runtimeInnerDomainVar, ir.getInt32(0)}); 
            llvmFunction.call("typeInitFromIntegerScalar", {indexVariableType}); 
            llvmFunction.call("variableInitFromIntegerScalar", {indexInitialization, ir.getInt32(0)});
            llvmFunction.call("variableInitFromDeclaration", {outerIndex, indexVariableType, indexInitialization}); 
            llvmFunction.call("variableInitFromDeclaration", {innerIndex, indexVariableType, indexInitialization}); 
            llvmFunction.call("variableDestructThenFree", {indexInitialization});

            //length variables for comparisson 
            llvm::Value *outerDomainLength = llvmFunction.call("variableGetLength", {outerRuntimeDomainArray});
            llvm::Value *truncOuterDomainLength= ir.CreateIntCast(outerDomainLength, ir.getInt32Ty(), false);
            llvm::Value *innerDomainLength = llvmFunction.call("variableGetLength", {innerRuntimeDomainArray});
            llvm::Value *truncInnerDomainLength= ir.CreateIntCast(innerDomainLength, ir.getInt32Ty(), false);
            llvmFunction.call("variableInitFromIntegerScalar", {outerDomainLengthVar, truncOuterDomainLength});
            llvmFunction.call("variableInitFromIntegerScalar", {innerDomainLengthVar, truncInnerDomainLength});

            //result matrix 
            auto generatorMatrix = llvmFunction.call("variableArrayMalloc", {outerDomainLength}); //result vector i 
            ir.CreateBr(outerHeader);
            ir.SetInsertPoint(outerHeader); 
            llvm::Value *branchCond = createBranchCondition(outerIndex, outerDomainLengthVar);
            ir.CreateCondBr(branchCond, innerPreHeader, outerMerge); 
            
            ir.SetInsertPoint(innerPreHeader);
            auto matrixRow = llvmFunction.call("variableArrayMalloc", {innerDomainLength});
            llvmFunction.call("variableReplace", {innerIndex, constZero});
            ir.CreateBr(innerHeader);
            ir.SetInsertPoint(innerHeader); 
            llvm::Value *innerBranchCond = createBranchCondition(innerIndex, innerDomainLengthVar);

            //hoist index values 
            llvm::Value* outerIndex_i32 = llvmFunction.call("variableGetIntegerValue", {outerIndex});
            llvm::Value* outerIndex_i64 = ir.CreateIntCast(outerIndex_i32, ir.getInt64Ty(), false); 
            llvm::Value* innerIndex_i32 = llvmFunction.call("variableGetIntegerValue", {innerIndex});
            llvm::Value* innerIndex_i64 = ir.CreateIntCast(innerIndex_i32, ir.getInt64Ty(), false);
            
            ir.CreateCondBr(innerBranchCond, innerBody, innerMerge); 
            ir.SetInsertPoint(innerBody);

            //load the two domain variables at respective index             
            initializeDomainVariable(runtimeOuterDomainVar, outerRuntimeDomainArray, outerIndex_i64);
            initializeDomainVariable(runtimeInnerDomainVar, innerRuntimeDomainArray, innerIndex_i64); 
            t->children[0]->children[0]->llvmValue = runtimeOuterDomainVar;
            t->children[0]->children[1]->llvmValue = runtimeInnerDomainVar;

            //initialize variable symbol to from variable at current index in domain array
            auto outerVarAST = t->children[0]->children[0];
            auto innerVarAST = t->children[0]->children[1];
            initializeVariableSymbol(outerVarAST, runtimeOuterDomainVar); 
            initializeVariableSymbol(innerVarAST, runtimeInnerDomainVar);  

            visit(t->children[1]);
            
            //set row to computed value
            auto exprVar = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromMemcpy", {exprVar, t->children[1]->llvmValue});
            llvmFunction.call("variableArraySet", {matrixRow, innerIndex_i64, exprVar});
            freeExpressionIfNecessary(t->children[1]);

            incrementIndex(innerIndex, 1); // increment the inner index
            ir.CreateBr(innerHeader);
            ir.SetInsertPoint(innerMerge);
            
            // variable init from vector literal & set into generator matrix [outer index] 
            auto matrixRowVariable = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromVectorLiteral", { matrixRowVariable, innerDomainLength, matrixRow });
            llvmFunction.call("variableArraySet", {generatorMatrix, outerIndex_i64, matrixRowVariable});
            llvmFunction.call("freeArrayContents", {matrixRow, innerDomainLength});
            llvmFunction.call("variableArrayFree", {matrixRow});

            ir.CreateBr(outerBody);
            ir.SetInsertPoint(outerBody);
            incrementIndex(outerIndex, 1); // increment the outer index

            ir.CreateBr(outerHeader);
            ir.SetInsertPoint(outerMerge);
 
            auto generatorMatrixVariable = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromVectorLiteral", { generatorMatrixVariable , outerDomainLength, generatorMatrix });
            t->llvmValue = generatorMatrixVariable;

            // low hanging fruits
            llvmFunction.call("typeDestructThenFree", {indexVariableType});
            llvmFunction.call("variableDestructThenFree", {constZero});
            llvmFunction.call("variableDestructThenFree", {outerIndex});
            llvmFunction.call("variableDestructThenFree", {innerIndex});
            llvmFunction.call("variableDestructThenFree", {outerDomainLengthVar}); 
            llvmFunction.call("variableDestructThenFree", {innerDomainLengthVar});
            llvmFunction.call("variableDestructThenFree", {outerRuntimeDomainArray});
            llvmFunction.call("variableDestructThenFree", {innerRuntimeDomainArray});
            llvmFunction.call("freeArrayContents", {generatorMatrix, outerDomainLength});
            llvmFunction.call("variableArrayFree", {generatorMatrix});
            llvmFunction.call("variableDestructThenFree", {t->children[0]->children[0]->llvmValue});
            llvmFunction.call("variableDestructThenFree", {t->children[0]->children[1]->llvmValue});
        }  
    }

    // creates boolean value that represents the comparisson currenIndex < domainLength
    llvm::Value* LLVMGen::createBranchCondition(llvm::Value* currentIndex, llvm::Value* domainLength) {
        auto comparissonVariable = llvmFunction.call("variableMalloc", {}); 
        llvmFunction.call("variableInitFromBinaryOp", {comparissonVariable, currentIndex, domainLength, ir.getInt32(10)}); //10 = < 
        llvm::Value *boolCond = llvmFunction.call("variableGetBooleanValue", {comparissonVariable});
        llvmFunction.call("variableDestructThenFree", {comparissonVariable});
        return ir.CreateICmpNE(boolCond, ir.getInt32(0));
    }

    // after domain variable is initialized, tie it to the vairable symbol so it may be used in generator expression
    void LLVMGen::initializeVariableSymbol(std::shared_ptr<AST> t, llvm::Value* domainVariable) {
        auto vs = std::dynamic_pointer_cast<VariableSymbol>(t->symbol); 
        t->llvmValue = domainVariable;
        vs->llvmPointerToVariableObject = domainVariable;
    }

    // for current index i, initialize the domain variable at array[i]
    void LLVMGen::initializeDomainVariable(llvm::Value* domainVariable, llvm::Value* domainArray, llvm::Value* index) {
        auto tempDomainVariable = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromArrayElementAtIndex", {tempDomainVariable, domainArray, index});
        llvmFunction.call("variableReplace", {domainVariable, tempDomainVariable});
        llvmFunction.call("variableDestructThenFree", {tempDomainVariable});
    }

    // increment and index variable by constant one 
    void LLVMGen::incrementIndex(llvm::Value* index, unsigned int increment) {
        auto constIncrement = llvmFunction.call("variableMalloc", {});
        auto newIndex = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIntegerScalar", {constIncrement, ir.getInt32(increment)}); 
        llvmFunction.call("variableInitFromBinaryOp", {newIndex, index, constIncrement, ir.getInt32(7)});
        llvmFunction.call("variableReplace", {index, newIndex});
        llvmFunction.call("variableDestructThenFree", {newIndex});
        llvmFunction.call("variableDestructThenFree", {constIncrement});
    }

    void LLVMGen::visitFilter(std::shared_ptr<AST> t) {

        llvm::Function* parentFunc = ir.GetInsertBlock()->getParent();
        llvm::BasicBlock* filterSetup = llvm::BasicBlock::Create(globalCtx, "filterSetup", parentFunc);
        ir.CreateBr(filterSetup);
        ir.SetInsertPoint(filterSetup);

        auto constZero = llvmFunction.call("variableMalloc", {});
        auto indexVarType = llvmFunction.call("typeMalloc", {});
        auto domainIndexVar = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("typeInitFromIntegerScalar", {indexVarType}); 
        llvmFunction.call("variableInitFromIntegerScalar", {constZero, ir.getInt32(0)}); 
        llvmFunction.call("variableInitFromDeclaration", {domainIndexVar, indexVarType, constZero});
 
        visit(t->children[0]); // domain expression
        auto domainArray = t->children[0]->children[1]; //expr pass up domain var 
        auto domainArrayVar = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromDomainExpression", {domainArrayVar, domainArray->llvmValue}); 
        if (domainArray->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) { //free is not id
            freeExpressionIfNecessary(domainArray); 
        }

        llvm::Value *domainArrayLength_i64 = llvmFunction.call("variableGetLength", {domainArrayVar});
        llvm::Value *domainArrayLength_i32 = ir.CreateIntCast(domainArrayLength_i64, ir.getInt32Ty(), false); 
        size_t numFilters = t->children[1]->children.size();

        auto domainArrayLengthVar = llvmFunction.call("variableMalloc", {});
        auto numFiltersVar = llvmFunction.call("variableMalloc", {});
        auto numFiltersIndexVar = llvmFunction.call("variableMalloc", {}); 

        llvmFunction.call("variableInitFromIntegerScalar", {domainArrayLengthVar, domainArrayLength_i32});
        llvmFunction.call("variableInitFromIntegerScalar", {numFiltersVar, ir.getInt32(numFilters)});
        llvmFunction.call("variableInitFromDeclaration", {numFiltersIndexVar, indexVarType, constZero});

        auto acceptMatrix = llvmFunction.call("acceptMatrixMalloc", {ir.getInt64(numFilters), domainArrayLength_i64}); 
        
        for (size_t i = 0; i < numFilters; i++) {
            //create basic blocks
            llvm::BasicBlock* preHeader = llvm::BasicBlock::Create(globalCtx, "filterPreHeader", parentFunc);
            llvm::BasicBlock* header = llvm::BasicBlock::Create(globalCtx, "fitlerHeader", parentFunc);
            llvm::BasicBlock* body = llvm::BasicBlock::Create(globalCtx, "fitlerBody", parentFunc);
            llvm::BasicBlock* merge = llvm::BasicBlock::Create(globalCtx, "filterMerge", parentFunc);

            //start inserting 
            ir.CreateBr(preHeader);
            ir.SetInsertPoint(preHeader);
            llvmFunction.call("variableAssignment", {domainIndexVar, constZero});
            
            //set in header
            ir.CreateBr(header);
            ir.SetInsertPoint(header);
            
            // create condition when domain variable index < domainSize 
            llvm::Value* condition = createBranchCondition(domainIndexVar, domainArrayLengthVar);
            ir.CreateCondBr(condition, body, merge);
            ir.SetInsertPoint(body);          
            
            //initialize the domain variable
            auto domainVar = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromIntegerScalar", {domainVar, ir.getInt32(0)}); //need to do this or else crash
            llvm::Value* index_i32 = llvmFunction.call("variableGetIntegerValue", {domainIndexVar});
            llvm::Value* index_i64 = ir.CreateIntCast(index_i32, ir.getInt64Ty(), false); 
            initializeDomainVariable(domainVar, domainArrayVar, index_i64);
            
            //initialize variable symbol with durrent domain variable
            auto variableAST = t->children[0]->children[0];
            initializeVariableSymbol(variableAST, domainVar);
            visit(t->children[1]->children[i]);
            
            //get boolean value from ast
            auto exprVar = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromMemcpy", {exprVar, t->children[1]->children[i]->llvmValue});
            llvm::Value *boolValue = llvmFunction.call("variableGetBooleanValue", {exprVar});
            auto filterIdx = ir.getInt64(i);
            auto domainIdx = ir.CreateIntCast(llvmFunction.call("variableGetIntegerValue", {domainIndexVar}), ir.getInt64Ty(),false);
            freeExpressionIfNecessary(t->children[1]->children[i]);
            llvmFunction.call("variableDestructThenFree", {exprVar});

            //set the accept matrix
            llvmFunction.call("acceptArraySet", {acceptMatrix, domainArrayLength_i64, filterIdx, domainIdx, boolValue});
            incrementIndex(domainIndexVar, 1);

            //free domain var 
            llvmFunction.call("variableDestructThenFree", {domainVar});
            ir.CreateBr(header);    
            ir.SetInsertPoint(merge);
        }
        auto resultTuple = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromFilterArray", {resultTuple, ir.getInt64(numFilters), domainArrayVar, acceptMatrix}); 
        t->llvmValue = resultTuple;

        llvmFunction.call("typeDestructThenFree", {indexVarType});
        llvmFunction.call("variableDestructThenFree", {constZero});
        llvmFunction.call("variableDestructThenFree", {domainIndexVar});
        llvmFunction.call("variableDestructThenFree", {domainArrayVar});
        llvmFunction.call("variableDestructThenFree", {domainArrayLengthVar});
        llvmFunction.call("variableDestructThenFree", {numFiltersVar});
        llvmFunction.call("variableDestructThenFree", {numFiltersIndexVar});
        llvmFunction.call("acceptMatrixFree", {acceptMatrix});
    }

    void LLVMGen::visitExpression(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->llvmValue = t->children[0]->llvmValue;
    }

    void LLVMGen::visitCast(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromCast", { runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue });
        t->llvmValue = runtimeVariableObject;
        llvmFunction.call("typeDestructThenFree", t->children[0]->llvmValue);
        
        freeExpressionIfNecessary(t->children[1]);
    }

    void LLVMGen::visitTypedef(std::shared_ptr<AST> t) {
        return;
    }

    void LLVMGen::visitInputStreamStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        llvmFunction.call("variableReadFromStdin", { t->children[0]->llvmValue });
    }

    void LLVMGen::visitOutputStreamStatement(std::shared_ptr<AST> t)
    {
        visitChildren(t);
        llvmFunction.call("variablePrintToStdout", {t->children[0]->llvmValue});
        freeExpressionIfNecessary(t->children[0]);
    }

    void LLVMGen::visitBinaryOperation(std::shared_ptr<AST> t) {
        visitChildren(t);
        int opCode;
        switch (t->children[2]->getNodeType()) {
            case GazpreaParser::CARET: // Character '*'
                opCode = 2;
                break;
            case GazpreaParser::ASTERISK:
                opCode = 3;
                break;
            case GazpreaParser::DIV:
                opCode = 4;
                break;
            case GazpreaParser::MODULO:
                opCode = 5;
                break;
            case GazpreaParser::DOTPRODUCT:
                opCode = 6;
                break;
            case GazpreaParser::PLUS:
                opCode = 7;
                break;
            case GazpreaParser::MINUS:
                opCode = 8;
                break;
            case GazpreaParser::BY:
                opCode = 9;
                break;
            case GazpreaParser::LESSTHAN:
                opCode = 10;
                break;
            case GazpreaParser::GREATERTHAN:
                opCode = 11;
                break;
            case GazpreaParser::LESSTHANOREQUAL:
                opCode = 12;
                break;
            case GazpreaParser::GREATERTHANOREQUAL:
                opCode = 13;
                break;
            case GazpreaParser::ISEQUAL:
                opCode = 14;
                break;
            case GazpreaParser::ISNOTEQUAL:
                opCode = 15;
                break;
            case GazpreaParser::AND:
                opCode = 16;
                break;
            case GazpreaParser::OR:
                opCode = 17;
                break;
            case GazpreaParser::XOR:
                opCode = 18;
                break;
            default:
                opCode = -1;
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", {runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(opCode)});
        t->llvmValue = runtimeVariableObject;

        freeExprAtomIfNecessary(t->children[0]);
        freeExprAtomIfNecessary(t->children[1]);
    }

    void LLVMGen::visitUnaryOperation(std::shared_ptr<AST> t) {
        if (t->children[0]->getNodeType() == GazpreaParser::MINUS 
        && t->children[1]->getNodeType() == GazpreaParser::IntegerConstant
        && t->children[1]->parseTree->getText() == "2147483648") {
            // Handle the edge case: integer x = -2147483648;
            auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromIntegerScalar", {runtimeVariableObject, ir.getInt32(-2147483648)});
            t->llvmValue = runtimeVariableObject;
            return;
        }
        visitChildren(t);
        int opCode;
        switch (t->children[0]->getNodeType()) {
        case GazpreaParser::PLUS:
            opCode = 0;
            break;
        case GazpreaParser::MINUS:
            opCode = 1;
            break;
        default:
            // "not" operator
            opCode = 2;
            break;
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromUnaryOp", {runtimeVariableObject, t->children[1]->llvmValue, ir.getInt32(opCode)});
        t->llvmValue = runtimeVariableObject;

        freeExprAtomIfNecessary(t->children[1]);
    }

    void LLVMGen::visitIndexing(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        auto numRHSExpressions = t->children[1]->children.size();
        if (numRHSExpressions == 1) {
            llvmFunction.call(
                "variableInitFromVectorIndexing", 
                { 
                    runtimeVariableObject, 
                    t->children[0]->llvmValue, 
                    t->children[1]->children[0]->llvmValue 
                }
            );
            freeExpressionIfNecessary(t->children[1]->children[0]);
        } else {
            llvmFunction.call(
                "variableInitFromMatrixIndexing", 
                { 
                    runtimeVariableObject, 
                    t->children[0]->llvmValue, 
                    t->children[1]->children[0]->llvmValue, 
                    t->children[1]->children[1]->llvmValue 
                }
            );
            freeExpressionIfNecessary(t->children[1]->children[0]);
            freeExpressionIfNecessary(t->children[1]->children[1]);
        }
        t->llvmValue = runtimeVariableObject;
        freeExprAtomIfNecessary(t->children[0]);
    }

    void LLVMGen::visitInterval(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", {runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(1)});
        t->llvmValue = runtimeVariableObject;

        freeExprAtomIfNecessary(t->children[0]);
        freeExprAtomIfNecessary(t->children[1]);
    }

    void LLVMGen::visitConcatenation(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", {runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(19)});
        t->llvmValue = runtimeVariableObject;

        freeExprAtomIfNecessary(t->children[0]);
        freeExprAtomIfNecessary(t->children[1]);
    }

    void LLVMGen::visitCallSubroutineInExpression(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->children[0]->symbol);
        auto *ctx = dynamic_cast<GazpreaParser::CallProcedureFunctionInExpressionContext*>(t->parseTree);

        if (subroutineSymbol->isBuiltIn) {
            // Built-in function
            int numArgsRecieved = t->children[1]->children.size(); 
            if (numArgsRecieved != 1) {
                throw InvalidArgumentError(subroutineSymbol->name, t->getText(),
                    ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()
                );
            }  
            std::vector<llvm::Value *> arguments = std::vector<llvm::Value *>();
            if (!t->children[1]->isNil()) {
                for (auto expressionAST : t->children[1]->children){
                    arguments.push_back(expressionAST->llvmValue);
                }
            }
            if (subroutineSymbol->name == "gazprea.subroutine.stream_state") {
                t->llvmValue = llvmFunction.call("BuiltInStreamState", {});
            } else if (subroutineSymbol->name == "gazprea.subroutine.length") {
                t->llvmValue = llvmFunction.call("BuiltInLength", arguments);
            } else if (subroutineSymbol->name == "gazprea.subroutine.reverse") {
                t->llvmValue = llvmFunction.call("BuiltInReverse", arguments);
            } else if (subroutineSymbol->name == "gazprea.subroutine.rows") {
                t->llvmValue = llvmFunction.call("BuiltInRows", arguments);
            } else if (subroutineSymbol->name == "gazprea.subroutine.columns") {
                t->llvmValue = llvmFunction.call("BuiltInColumns", arguments);
            }
            if (!t->children[1]->isNil()) {
                for (auto expressionAST : t->children[1]->children) {
                    freeExpressionIfNecessary(expressionAST);
                }
            }
            return;
        }
        
        //Exception for misaligned argument pass  
        int numArgsExpected = subroutineSymbol->declaration->children[1]->children.size(); 
        int numArgsRecieved = t->children[1]->children.size(); 
        if (numArgsExpected != numArgsRecieved) {
            throw InvalidArgumentError(subroutineSymbol->declaration->children[0]->getText(), t->getText(),
                ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()
            );
        }       
        std::vector<llvm::Value *> arguments = std::vector<llvm::Value *>();
        if (!t->children[1]->isNil()) {
            int i = 0;
            for (auto expressionAST : t->children[1]->children) {
                if (expressionAST->children[0]->getNodeType() == GazpreaParser::IDENTIFIER_TOKEN) {
                    auto variableArgumentSymbol = std::dynamic_pointer_cast<VariableSymbol>(expressionAST->children[0]->symbol);
                    auto variableParameterSymbol = std::dynamic_pointer_cast<VariableSymbol>(subroutineSymbol->orderedArgs[i]);

                    if (variableArgumentSymbol->typeQualifier == "const" && variableParameterSymbol->typeQualifier == "var") {
                        throw InvalidArgumentError(subroutineSymbol->declaration->children[0]->getText(), t->getText(),
                            ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()
                        );
                    }
                }
                arguments.push_back(expressionAST->llvmValue);
                i++;
            }
        }
        t->llvmValue = ir.CreateCall(subroutineSymbol->llvmFunction, arguments);
        if (!t->children[1]->isNil()) {
            for (auto expressionAST : t->children[1]->children) {
                freeExpressionIfNecessary(expressionAST);
            }
        }
    }

    void LLVMGen::visitCallSubroutineStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->children[0]->symbol);
        auto *ctx = dynamic_cast<GazpreaParser::CallProcedureContext*>(t->parseTree);
        
        if (subroutineSymbol->isBuiltIn) {
            // Built-in function
            int numArgsRecieved = t->children[1]->children.size(); 
            if (numArgsRecieved != 1) {
                throw InvalidArgumentError(subroutineSymbol->name, t->getText(),
                    ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()
                );
            }  
            std::vector<llvm::Value *> arguments = std::vector<llvm::Value *>();
            if (!t->children[1]->isNil()) {
                for (auto expressionAST : t->children[1]->children){
                    arguments.push_back(expressionAST->llvmValue);
                }
            }
            llvm::Value *returnValue = nullptr;
            if (subroutineSymbol->name == "gazprea.subroutine.stream_state") {
                returnValue = llvmFunction.call("BuiltInStreamState", {});
            } else if (subroutineSymbol->name == "gazprea.subroutine.length") {
                returnValue = llvmFunction.call("BuiltInLength", arguments);
            } else if (subroutineSymbol->name == "gazprea.subroutine.reverse") {
                returnValue = llvmFunction.call("BuiltInReverse", arguments);
            } else if (subroutineSymbol->name == "gazprea.subroutine.rows") {
                returnValue = llvmFunction.call("BuiltInRows", arguments);
            } else if (subroutineSymbol->name == "gazprea.subroutine.columns") {
                returnValue = llvmFunction.call("BuiltInColumns", arguments);
            }
            
            if (returnValue != nullptr) {
                llvmFunction.call("variableDestructThenFree", returnValue);
            }

            if (!t->children[1]->isNil()) {
                for (auto expressionAST : t->children[1]->children) {
                    freeExpressionIfNecessary(expressionAST);
                }
            }
            return;
        }
        
        //Throw exception for invalid arguments 
        int numArgsExpected = subroutineSymbol->declaration->children[1]->children.size(); 
        int numArgsRecieved = t->children[1]->children.size(); 
        if (numArgsExpected != numArgsRecieved) {
            throw InvalidArgumentError(subroutineSymbol->declaration->children[0]->getText(), t->getText(),
                ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()
            );
        }  
        std::vector<llvm::Value *> arguments = std::vector<llvm::Value *>();
        if (!t->children[1]->isNil()) {
            int i = 0;
            for (auto expressionAST : t->children[1]->children) {
                if (expressionAST->children[0]->getNodeType() == GazpreaParser::IDENTIFIER_TOKEN) {
                    auto variableArgumentSymbol = std::dynamic_pointer_cast<VariableSymbol>(expressionAST->children[0]->symbol);
                    auto variableParameterSymbol = std::dynamic_pointer_cast<VariableSymbol>(subroutineSymbol->orderedArgs[i]);

                    if (variableArgumentSymbol->typeQualifier == "const" && variableParameterSymbol->typeQualifier == "var") {
                        throw InvalidArgumentError(subroutineSymbol->declaration->children[0]->getText(), t->getText(),
                            ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()
                        );
                    }
                }
                arguments.push_back(expressionAST->llvmValue);
                i++;
            }
        }
        if (subroutineSymbol->type == nullptr) {
            ir.CreateCall(subroutineSymbol->llvmFunction, arguments);
        } else {
            auto returnValue = ir.CreateCall(subroutineSymbol->llvmFunction, arguments);
            llvmFunction.call("variableDestructThenFree", returnValue);
        }

        if (!t->children[1]->isNil()) {
            for (auto expressionAST : t->children[1]->children) {
                freeExpressionIfNecessary(expressionAST);
            }
        }
    }

    void LLVMGen::visitParameterAtom(std::shared_ptr<AST> t) {
        auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
        auto runtimeTypeObject = llvmFunction.call("typeMalloc", {}); 
        std::shared_ptr<MatrixType> matrixType;
        std::shared_ptr<TupleType> tupleType;
        std::shared_ptr<TypedefTypeSymbol> typedefTypeSymbol;
        llvm::Value *baseType;
        llvm::Value *dimension1Expression = nullptr;
        llvm::Value *dimension2Expression = nullptr;

        if (variableSymbol->type == nullptr) {
            // inferred type qualifier
            llvmFunction.call("typeInitFromUnknownType", { runtimeTypeObject });
            variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;
            return;
        }
        switch(variableSymbol->type->getTypeId()) {
            case Type::BOOLEAN:
                llvmFunction.call("typeInitFromBooleanScalar", { runtimeTypeObject });
                break;
            case Type::CHARACTER:
                llvmFunction.call("typeInitFromCharacterScalar", { runtimeTypeObject });
                break;
            case Type::INTEGER:
                llvmFunction.call("typeInitFromIntegerScalar", { runtimeTypeObject });
                break;
            case Type::REAL:
                llvmFunction.call("typeInitFromRealScalar", { runtimeTypeObject });
                break;
            case Type::INTEGER_INTERVAL: 
                llvmFunction.call("typeInitFromIntegerInterval", { runtimeTypeObject });
                break;
            case Type::STRING:
                if (t->symbol->type->isTypedefType()) {
                    auto typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    if (typedefTypeSymbol->type->isMatrixType()) {
                        matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            visit(matrixType->def->children[1]->children[0]);
                            dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                        } else { 
                            dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                        }
                        baseType = llvmFunction.call("typeMalloc", {});
                        llvmFunction.call("typeInitFromUnspecifiedString", {baseType});
                        llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                        llvmFunction.call("typeDestructThenFree", baseType);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                        }
                    } else {
                        llvmFunction.call("typeInitFromUnspecifiedString", runtimeTypeObject);
                    }
                } else {
                    if (t->symbol->type->isMatrixType()) {
                        matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            visit(matrixType->def->children[1]->children[0]);
                            dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                        } else { 
                            dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                        }
                        baseType = llvmFunction.call("typeMalloc", {});
                        llvmFunction.call("typeInitFromUnspecifiedString", {baseType});
                        llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                        llvmFunction.call("typeDestructThenFree", baseType);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                        }
                    } else {
                        llvmFunction.call("typeInitFromUnspecifiedString", runtimeTypeObject);
                    }
                }
                break;
            case Type::BOOLEAN_1: {
                if (t->symbol->type->isTypedefType()) {
                    auto typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                }    
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromBooleanScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            }
            case Type::CHARACTER_1: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                }    
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                        visit(matrixType->def->children[1]->children[0]);
                        dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                        dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromCharacterScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            }
            case Type::INTEGER_1: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                }    
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                        visit(matrixType->def->children[1]->children[0]);
                        dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                        dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromIntegerScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            }
            case Type::REAL_1: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                }    
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                        visit(matrixType->def->children[1]->children[0]);
                        dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                        dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromRealScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            }
            case Type::BOOLEAN_2: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                } if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                        visit(matrixType->def->children[1]->children[0]);
                        dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                        dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                } if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                        visit(matrixType->def->children[1]->children[1]);
                        dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                        dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromBooleanScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", {runtimeTypeObject, dimension1Expression, dimension2Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            }
            case Type::CHARACTER_2: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                } if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                        dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                } if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromCharacterScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", {runtimeTypeObject, dimension1Expression, dimension2Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            }
            case Type::INTEGER_2: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                } if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                } if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromIntegerScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", {runtimeTypeObject, dimension1Expression, dimension2Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            }
            case Type::REAL_2: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                }  if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else { 
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                } if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromRealScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", {runtimeTypeObject, dimension1Expression, dimension2Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            }
            case Type::TUPLE: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    tupleType = std::dynamic_pointer_cast<TupleType>(typedefTypeSymbol->type);
                } else {
                    tupleType = std::dynamic_pointer_cast<TupleType>(t->symbol->type);
                }
                auto typeArray = llvmFunction.call("typeArrayMalloc", {ir.getInt64(tupleType->orderedArgs.size())} );
                auto stridArray = llvmFunction.call("stridArrayMalloc", {ir.getInt64(tupleType->orderedArgs.size())} );
                std::shared_ptr<VariableSymbol> argumentSymbol;
                for (size_t i = 0; i < tupleType->orderedArgs.size(); i++) {
                    argumentSymbol = std::dynamic_pointer_cast<VariableSymbol>(tupleType->orderedArgs[i]);
                    visit(argumentSymbol->def);
                    llvmFunction.call("typeArraySet", { typeArray, ir.getInt64(i), argumentSymbol->llvmPointerToTypeObject });
                    if (argumentSymbol->name == "") {
                        llvmFunction.call("stridArraySet", { stridArray, ir.getInt64(i), ir.getInt64(-1) });
                    } else {
                        llvmFunction.call("stridArraySet", { stridArray, ir.getInt64(i), ir.getInt64(symtab->tupleIdentifierAccess.at(argumentSymbol->name)) });
                    }
                }
                llvmFunction.call("typeInitFromTupleType", { runtimeTypeObject, ir.getInt64(tupleType->orderedArgs.size()), typeArray, stridArray });
                llvmFunction.call("stridArrayFree", { stridArray });

                for (size_t i = 0; i < tupleType->orderedArgs.size(); i++) {
                    argumentSymbol = std::dynamic_pointer_cast<VariableSymbol>(tupleType->orderedArgs[i]);
                    llvmFunction.call("typeDestructThenFree", argumentSymbol->llvmPointerToTypeObject);
                }
                llvmFunction.call("typeArrayFree", { typeArray });
            }
        }
        variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;
    }

    void LLVMGen::visitTupleLiteral(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto numExpressions = t->children[0]->children.size();
        auto runtimeVariableArray = llvmFunction.call("variableArrayMalloc", { ir.getInt64(numExpressions) });
        for (size_t i = 0; i < numExpressions; i++) {
            llvmFunction.call("variableArraySet", { runtimeVariableArray, ir.getInt64(i), t->children[0]->children[i]->llvmValue });
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromTupleLiteral", { runtimeVariableObject, ir.getInt64(numExpressions), runtimeVariableArray });
        t->llvmValue = runtimeVariableObject;
        llvmFunction.call("variableArrayFree", { runtimeVariableArray });

        // Free all unused variables
        for (size_t i = 0; i < numExpressions; i++) {
            freeExpressionIfNecessary(t->children[0]->children[i]);
        }
    }

    void LLVMGen::visitVectorMatrixLiteral(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto numExpressions = t->children[0]->children.size();
        auto runtimeVariableArray = llvmFunction.call("variableArrayMalloc", { ir.getInt64(numExpressions) });
        for (size_t i = 0; i < numExpressions; i++) {
            llvmFunction.call("variableArraySet", { runtimeVariableArray, ir.getInt64(i), t->children[0]->children[i]->llvmValue });
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromVectorLiteral", { runtimeVariableObject, ir.getInt64(numExpressions), runtimeVariableArray });
        
        t->llvmValue = runtimeVariableObject;
        llvmFunction.call("variableArrayFree", { runtimeVariableArray });

        // Free all unused variables
        for (size_t i = 0; i < numExpressions; i++) {
            freeExpressionIfNecessary(t->children[0]->children[i]);
        }
    }

    void LLVMGen::visitTupleAccess(std::shared_ptr<AST> t) {
        visit(t->children[0]);
        if (t->children[1]->getNodeType() == GazpreaParser::IDENTIFIER_TOKEN) {
            auto tupleType = std::dynamic_pointer_cast<TupleType>(t->children[0]->evalType);
            auto identifierName = t->children[1]->parseTree->getText();
            if (tupleType != nullptr) {
                size_t i;
                for (i = 0; i < tupleType->orderedArgs.size(); i++) {
                    if (tupleType->orderedArgs[i]->name == identifierName) {
                        break;
                    }
                }
                t->llvmValue = llvmFunction.call("variableGetTupleField", { t->children[0]->llvmValue, ir.getInt64(i + 1) });
            } else {
                t->llvmValue = llvmFunction.call("variableGetTupleFieldFromID", { t->children[0]->llvmValue, ir.getInt64(symtab->tupleIdentifierAccess.at(identifierName)) });
            }
        } else {
            auto index = std::stoi(t->children[1]->parseTree->getText());
            t->llvmValue = llvmFunction.call("variableGetTupleField", { t->children[0]->llvmValue, ir.getInt64(index) }); 
        }
    }

    void LLVMGen::visitBlock(std::shared_ptr<AST> t) {
        llvmBranch.hitReturnStat = false; 
        auto parentFunc = ir.GetInsertBlock()->getParent();

        //visit the body
        visitChildren(t);
        
        auto localScope = std::dynamic_pointer_cast<LocalScope>(t->scope);
        //handle returns in empty blocks 
        if (!localScope->parentIsSubroutineSymbol && !localScope->parentIsLoop && !localScope->parentIsConditional) {
            if (llvmBranch.hitReturnStat) {
                llvm::BasicBlock* exitBlock = llvm::BasicBlock::Create(globalCtx, "exitBlock", parentFunc);
                ir.SetInsertPoint(exitBlock);
                llvmBranch.hitReturnStat = false;
            }
        } 
        if (!localScope->containReturn) {
            freeAllVariablesDeclaredInBlockScope(localScope);
            if (localScope->parentIsSubroutineSymbol) {
                auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->scope->getEnclosingScope());
                freeSubroutineParameters(subroutineSymbol);
                if (subroutineSymbol->type == nullptr) {
                    ir.CreateRetVoid();
                } else {
                    ir.CreateRet(llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()));
                }
            }
        }
    }

    void LLVMGen::initializeGlobalVariables() {
        // Initialize global variables (should only call in the beginning of main())
        auto globalStackObj = llvmFunction.call("runtimeStackMallocThenInit", {});
        ir.CreateStore(globalStackObj, globalStack);

        for (auto variableSymbol : symtab->globals->globalVariableSymbols) {
            auto globalVar = mod.getNamedGlobal(variableSymbol->name);
            visit(variableSymbol->def->children[2]);
            ir.CreateStore(variableSymbol->def->children[2]->llvmValue, globalVar);
        }

    }

    void LLVMGen::freeGlobalVariables() {
        // Free all global variables
        for (auto variableSymbol : symtab->globals->globalVariableSymbols) {
            auto globalVarAddress = mod.getNamedGlobal(variableSymbol->name);
            auto globalVar = ir.CreateLoad(runtimeVariableTy->getPointerTo(), globalVarAddress);
            llvmFunction.call("variableDestructThenFree", globalVar);
        }
        llvmFunction.call("runtimeStackDestructThenFree", {getStack()});
    }

    void LLVMGen::freeAllVariablesDeclaredInBlockScope(std::shared_ptr<LocalScope> scope) {
        // Free all VariableSymbol in the Scope
        for (auto const& [key, val] : scope->symbols) {
            auto vs = std::dynamic_pointer_cast<VariableSymbol>(val);
            if (vs != nullptr && vs->llvmPointerToVariableObject != nullptr) {
                llvmFunction.call("variableDestructThenFree", vs->llvmPointerToVariableObject);
            }
        }
    }

    void LLVMGen::initializeSubroutineParameters(std::shared_ptr<SubroutineSymbol> subroutineSymbol) {
        for (size_t i = 0; i < subroutineSymbol->orderedArgs.size(); i++) {
            auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(subroutineSymbol->orderedArgs[i]);
            if (variableSymbol->typeQualifier == "const") {
                auto runtimeVariableParameterObject = llvmFunction.call("variableMalloc", {});
                llvmFunction.call(
                    "variableInitFromParameter", 
                    {
                        runtimeVariableParameterObject, 
                        variableSymbol->llvmPointerToTypeObject, 
                        subroutineSymbol->llvmFunction->getArg(i) 
                    }
                );
                variableSymbol->llvmPointerToVariableObject = runtimeVariableParameterObject;
                llvmFunction.call("typeDestructThenFree", variableSymbol->llvmPointerToTypeObject);
            } else {
                variableSymbol->llvmPointerToVariableObject = subroutineSymbol->llvmFunction->getArg(i);
                if (variableSymbol->type != nullptr && variableSymbol->type->getTypeId() == Type::TUPLE) {
                    // Only swap/redefine type if the parameter type is tuple
                    auto oldType = llvmFunction.call(
                        "variableSwapType", 
                        { variableSymbol->llvmPointerToVariableObject, variableSymbol->llvmPointerToTypeObject }
                    );
                    subroutineSymbol->oldParameterTypes.push_back(oldType);
                } else {
                    subroutineSymbol->oldParameterTypes.push_back(nullptr);
                    llvmFunction.call("typeDestructThenFree", variableSymbol->llvmPointerToTypeObject);
                }
            }
        }
    }

    void LLVMGen::freeSubroutineParameters(std::shared_ptr<SubroutineSymbol> subroutineSymbol) {
        // Free all variables from variableInitFromParameter()
        for (size_t i = 0; i < subroutineSymbol->orderedArgs.size(); i++) {
            auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(subroutineSymbol->orderedArgs[i]);
            if (variableSymbol->typeQualifier == "const") {
                llvmFunction.call("variableDestructThenFree", variableSymbol->llvmPointerToVariableObject);
            } else {
                if (subroutineSymbol->oldParameterTypes[i] != nullptr) {
                    auto subroutineParameterType = llvmFunction.call(
                        "variableSwapType", 
                        { variableSymbol->llvmPointerToVariableObject, subroutineSymbol->oldParameterTypes[i] }
                    );
                    llvmFunction.call("typeDestructThenFree", subroutineParameterType);
                }
            }
        }
    }

    void LLVMGen::freeExpressionIfNecessary(std::shared_ptr<AST> t) {
        if (t->children[0]->getNodeType() != GazpreaParser::IDENTIFIER_TOKEN
        && t->children[0]->getNodeType() != GazpreaParser::TUPLE_ACCESS_TOKEN) {
            llvmFunction.call("variableDestructThenFree", t->llvmValue);
        }
    }

    void LLVMGen::freeExprAtomIfNecessary(std::shared_ptr<AST> t) {
        if (t->getNodeType() != GazpreaParser::IDENTIFIER_TOKEN
        && t->getNodeType() != GazpreaParser::TUPLE_ACCESS_TOKEN) {
            llvmFunction.call("variableDestructThenFree", t->llvmValue);
        }
    }

    std::string LLVMGen::unescapeString(const std::string &s) {
        std::string res = "";
        std::string::const_iterator it = s.begin();
        while (it != s.end())
        {
            char c = *it++;
            if (c == '\\' && it != s.end()) {
                switch (*it++) {
                    case 'a':
                        c = '\a';
                        break;
                    case 'b':
                        c = '\b';
                        break;
                    case 'n':
                        c = '\n';
                        break;
                    case 'r':
                        c = '\r';
                        break;
                    case 't':
                        c = '\t';
                        break;
                    case '\"':
                        c = '\"';
                        break;
                    case '\'':
                        c = '\'';
                        break;
                    case '\\':
                        c = '\\';
                        break;
                    default: 
                        // invalid escape sequence
                        continue;
                }
            }
            res += c;
        }
        return res;
    }

    void LLVMGen::Print() {
        // write module as .ll file
        std::string compiledLLFile;
        llvm::raw_string_ostream out(compiledLLFile);

        llvm::raw_os_ostream llOut(std::cout);
        llvm::raw_os_ostream llErr(std::cerr);
        llvm::verifyModule(mod, &llErr);

        mod.print(out, nullptr);
        out.flush();
        std::ofstream outFile(outfile);
        outFile << compiledLLFile;
    }
} // namespace gazprea