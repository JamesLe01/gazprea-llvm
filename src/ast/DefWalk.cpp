#include "DefWalk.h"

namespace gazprea {

    DefWalk::DefWalk(std::shared_ptr<SymbolTable> symtab) : symtab(symtab), currentScope(symtab->globals), numLoopAncestors(0), numSubroutineAncestors(0) {
        // Define built-in subroutine symbols
        bool isBuiltIn = true;
        symtab->globals->define(std::make_shared<SubroutineSymbol>("gazprea.subroutine.length", symtab->getType(Type::INTEGER), symtab->globals, false, isBuiltIn));
        symtab->globals->define(std::make_shared<SubroutineSymbol>("gazprea.subroutine.rows", symtab->getType(Type::INTEGER), symtab->globals, false, isBuiltIn));
        symtab->globals->define(std::make_shared<SubroutineSymbol>("gazprea.subroutine.columns", symtab->getType(Type::INTEGER), symtab->globals, false, isBuiltIn));
        symtab->globals->define(std::make_shared<SubroutineSymbol>("gazprea.subroutine.reverse", nullptr, symtab->globals, false, isBuiltIn));
        symtab->globals->define(std::make_shared<SubroutineSymbol>("gazprea.subroutine.stream_state", symtab->getType(Type::INTEGER), symtab->globals, true, isBuiltIn));
        symtab->globals->define(std::make_shared<VariableSymbol>("std_input", nullptr));
        symtab->globals->define(std::make_shared<VariableSymbol>("std_output", nullptr));
    }
    DefWalk::~DefWalk() {}

    void DefWalk::visit(std::shared_ptr<AST> t) {
        if(!t->isNil()){
            switch(t->getNodeType()){
                //Statements & Intermediate Tokens
                case GazpreaParser::PROCEDURE:
                case GazpreaParser::FUNCTION:
                    numSubroutineAncestors++;
                    visitSubroutineDeclDef(t);
                    numSubroutineAncestors--;
                    break;
                case GazpreaParser::TYPEDEF:
                    visitTypedefStatement(t);
                    break;
                case GazpreaParser::BLOCK_TOKEN: 
                    visitBlock(t);
                    break;
                case GazpreaParser::IDENTIFIER_TOKEN:
                    visitIdentifier(t);
                    break;
                case GazpreaParser::VAR_DECLARATION_TOKEN:
                    visitVariableDeclaration(t);
                    break;
                case GazpreaParser::PARAMETER_ATOM_TOKEN:
                    visitParameterAtom(t);
                    break;
                case GazpreaParser::TUPLE_TYPE_TOKEN:
                    visitTupleType(t);
                    break;
                case GazpreaParser::BREAK:
                    visitBreak(t);
                    break;
                case GazpreaParser::CONTINUE:
                    visitContinue(t);
                    break;
                case GazpreaParser::RETURN:
                    visitReturn(t);
                    break;
                case GazpreaParser::TUPLE_ACCESS_TOKEN:
                    visitTupleAccess(t);
                    break;
                case GazpreaParser::INFINITE_LOOP_TOKEN:
                    numLoopAncestors++;
                    vistInfiniteLoop(t);
                    numLoopAncestors--;
                    break;
                case GazpreaParser::PRE_PREDICATE_LOOP_TOKEN:
                    numLoopAncestors++;
                    visitPrePredicatedLoop(t);
                    numLoopAncestors--;
                    break;
                case GazpreaParser::POST_PREDICATE_LOOP_TOKEN:
                    numLoopAncestors++;
                    visitPostPredicatedLoop(t);
                    numLoopAncestors--;
                    break;
                case GazpreaParser::ITERATOR_LOOP_TOKEN:
                    numLoopAncestors++;
                    visitIteratorLoop(t);
                    numLoopAncestors--;
                    break;
                case GazpreaParser::CONDITIONAL_STATEMENT_TOKEN:
                    visitConditionalStatement(t);
                    break;
                case GazpreaParser::GENERATOR_TOKEN:
                    visitGenerator(t);
                    break;
                case GazpreaParser::FILTER_TOKEN:
                    visitFilter(t);
                    break;
                case GazpreaParser::DOMAIN_EXPRESSION_TOKEN:
                    visitDomainExpression(t);
                    break;
                default:
                    visitChildren(t);
            };
        }
        else {
            visitChildren(t); // t must be the null root node 
        }
    }

    void DefWalk::visitChildren(std::shared_ptr<AST> t) { 
        for(auto stat : t->children){
            visit(stat);
        }
    }

    void DefWalk::visitVariableDeclaration(std::shared_ptr<AST> t) {
        auto identifierAST = t->children[1];
        auto vs = std::make_shared<VariableSymbol>(identifierAST->parseTree->getText(), nullptr);
        vs->def = t;  // track AST location of def's ID (i.e., where in AST does this symbol defined)
        t->symbol = vs;  // track in AST
        currentScope->define(vs);
        if (vs->isDoubleDefined) { 
            auto *ctx = dynamic_cast<GazpreaParser::VarDeclarationStatementContext*>(t->parseTree);
            throw RedefineIdError(t->children[1]->getText(), ctx->getText(), ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine());
        }
 
        visitChildren(t);
        if (currentScope->getScopeName() == "gazprea.scope.global") {
            vs->isGlobalVariable = true;
            symtab->globals->globalVariableSymbols.push_back(vs);
        } else {
            vs->isGlobalVariable = false;
        }
    }

    void DefWalk::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
        bool isProcedure;
        bool isBuiltIn = false;
        auto *ctx = dynamic_cast<GazpreaParser::SubroutineDeclDefContext*>(t->parseTree);
        std::string sbrtName = t->children[0]->parseTree->getText(); 
        for (auto reservedName : this->reservedSbrtNames) {
            if (sbrtName == reservedName) {
                throw RedefineIdError(sbrtName, t->children[0]->getText() + "(...", ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine());
            }
        }
        if (t->getNodeType() == GazpreaParser::PROCEDURE) {
            isProcedure = true;
        } else {
            isProcedure = false;
        }
        auto identiferAST = t->children[0];
        if (identiferAST->parseTree->getText() == "main" && isProcedure) {
            //track if we encounter a procedure named main
            this->hasMainProcedure = true;
        }
        std::shared_ptr<SubroutineSymbol> subroutineSymbol;
        auto declarationSubroutineSymbol = symtab->globals->resolveSubroutineSymbol(
            "gazprea.subroutine." + identiferAST->parseTree->getText()
        );
        
        if (declarationSubroutineSymbol == nullptr) {
            subroutineSymbol = std::make_shared<SubroutineSymbol>(
                "gazprea.subroutine." + identiferAST->parseTree->getText(), 
                nullptr, 
                symtab->globals, 
                isProcedure, 
                isBuiltIn
            );
            subroutineSymbol->declaration = t;
            symtab->globals->define(subroutineSymbol); // def subroutine in globals
            subroutineSymbol->numTimesDeclare++;
        } else {
            subroutineSymbol = declarationSubroutineSymbol;
            subroutineSymbol->definition = t;
            subroutineSymbol->numTimesDeclare++;
        }
        if (!t->children[2]->isNil()) {
            subroutineSymbol->hasReturn = true;
        }
        if ( t->children[3]->getNodeType() == GazpreaParser::SUBROUTINE_BLOCK_BODY_TOKEN 
            || t->children[3]->getNodeType() == GazpreaParser::SUBROUTINE_EXPRESSION_BODY_TOKEN )  {
                subroutineSymbol->numTimesDefined++;
        }
        if (subroutineSymbol->numTimesDeclare > 2) {
            std::string descr = "Subroutine has been defined/declared more than two times: ";
            throw GazpreaError(descr, t->children[0]->getText() + "(...", t->children[0]->getText(),  ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()); 
        } 

        t->symbol = subroutineSymbol;  // track in AST
        currentSubroutineScope = subroutineSymbol;  // Track Subroutine Scope for visitReturn()
        currentScope = subroutineSymbol;        // set current scope to subroutine scope
        visitChildren(t);
        currentScope = currentScope->getEnclosingScope(); // pop subroutine scope

        t->children[0]->scope = currentScope;  // Manually set the scope of the identifier token of this AST node (override visitIdentifier(t))
    }

    void DefWalk::visitTypedefStatement(std::shared_ptr<AST> t) {
        auto identiferAST = t->children[1];
        auto typeDefTypeSymbol = std::make_shared<TypedefTypeSymbol>(identiferAST->parseTree->getText());
        typeDefTypeSymbol->def = t;  // track AST location of def's ID (i.e., where in AST does this symbol defined)
        t->symbol = typeDefTypeSymbol;  // track in AST
        symtab->globals->defineTypeSymbol(typeDefTypeSymbol);
        
        if (typeDefTypeSymbol->isDoubleDefined) {
            auto *ctx = dynamic_cast<GazpreaParser::TypedefStatementContext*>(t->parseTree);
            throw RedefineIdError(identiferAST->getText(), ctx->getText(), ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine());
        }
        visitChildren(t);
    } 
    void DefWalk::visitBlock(std::shared_ptr<AST> t) {
        auto blockScope = std::make_shared<LocalScope>(currentScope);
        t->scope = blockScope;
        currentScope = blockScope; // push scope

        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(currentScope->getEnclosingScope());
        if (subroutineSymbol != nullptr) {
            // If the parent scope is subroutine symbol
            blockScope->parentIsSubroutineSymbol = true;
            subroutineSymbol->subroutineDirectChildScope = blockScope;
        } else {
            blockScope->parentIsSubroutineSymbol = false;
        }
        visitChildren(t);
        currentScope = currentScope->getEnclosingScope(); // pop scope
    }

    void DefWalk::visitIdentifier(std::shared_ptr<AST> t) {
        t->scope = currentScope;
    }

    void DefWalk::visitTupleType(std::shared_ptr<AST> t) {
        size_t tupleSize = t->children[0]->children.size();
        auto tupleType = std::make_shared<TupleType>(currentScope, t, tupleSize);
        t->type = tupleType;
        currentScope = tupleType;        // set current scope to tuple scope
        visitChildren(t);
        currentScope = currentScope->getEnclosingScope(); // pop tuple scope
    }

    void DefWalk::visitParameterAtom(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto variableSymbol = std::make_shared<VariableSymbol>("", nullptr);
        currentScope->define(variableSymbol);
        t->symbol = variableSymbol;
        variableSymbol->def = t;
    }

    void DefWalk::visitBreak(std::shared_ptr<AST> t) {
        if (numLoopAncestors == 0) {
            // TODO: Use different type of exception
            auto *ctx = dynamic_cast<GazpreaParser::BreakStatementContext*>(t->parseTree);
            std::string msg = "Return statement must be inside a subroutine (function or proceedure) ";
            throw GazpreaError(msg, t->getText(), t->getText(), ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine());
        }
        t->scope = currentScope;
    }

    void DefWalk::visitContinue(std::shared_ptr<AST> t) {
        if (numLoopAncestors == 0) {
            // TODO: Use different type of exception
            auto *ctx = dynamic_cast<GazpreaParser::ContinueStatementContext*>(t->parseTree);
            std::string msg = "Return statement must be inside a subroutine (function or proceedure) ";
            throw GazpreaError(msg, t->getText(), t->getText(), ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()); 
        }
        t->scope = currentScope;
    }

    void DefWalk::visitReturn(std::shared_ptr<AST> t) {
        if (numSubroutineAncestors == 0) {
            auto *ctx = dynamic_cast<GazpreaParser::ReturnStatementContext*>(t->parseTree);
            std::string msg = "Return statement must be inside a subroutine (function or proceedure) ";
            throw GazpreaError(msg, t->getText(), t->getText(), ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine());
        }
        t->scope = currentScope;
        t->scope->containReturn = true;
        t->subroutineSymbol = currentSubroutineScope;
        visitChildren(t);
    }

    void DefWalk::visitTupleAccess(std::shared_ptr<AST> t) {
        visitChildren(t);
        if (t->children[1]->getNodeType() == GazpreaParser::IDENTIFIER_TOKEN) {
            auto identifierName = t->children[1]->parseTree->getText();
            auto status = symtab->tupleIdentifierAccess.emplace(
                identifierName, symtab->numTupleIdentifierAccess
            );
            if (status.second) {
                symtab->numTupleIdentifierAccess++;
            }
        }
    }

    void DefWalk::vistInfiniteLoop(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto bodyScope = std::dynamic_pointer_cast<LocalScope>(t->children[0]->scope);
        bodyScope->parentIsLoop = true;
    }

    void DefWalk::visitPrePredicatedLoop(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto bodyScope = std::dynamic_pointer_cast<LocalScope>(t->children[1]->scope);
        bodyScope->parentIsLoop = true;
    }

    void DefWalk::visitPostPredicatedLoop(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto bodyScope = std::dynamic_pointer_cast<LocalScope>(t->children[0]->scope);
        bodyScope->parentIsLoop = true;
    }

    //Change the order of visit children so that domain expressions are defined in the block scope
    void DefWalk::visitIteratorLoop(std::shared_ptr<AST> t) {
        //create scope for domain expr and loop body
        auto blockScope = std::make_shared<LocalScope>(currentScope);
        t->children[t->children.size()-1]->scope = blockScope;
        currentScope = blockScope; // push scope
        blockScope->parentIsSubroutineSymbol = false;
        //then visit domain expressions after scope defined
        for (size_t i = 0; i < t->children.size()-1; i++) {
            visit(t->children[i]);
        }  
        visit(t->children[t->children.size()-1]); //visit the block statements
        auto bodyScope = std::dynamic_pointer_cast<LocalScope>(t->children[t->children.size()-1]->scope);
        bodyScope->parentIsLoop = true;
        currentScope = currentScope->getEnclosingScope(); // pop scope 
        return;
    }

    void DefWalk::visitConditionalStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        for (auto child: t->children ) {
            if (child->getNodeType() == GazpreaParser::BLOCK_TOKEN) {
                auto blockScope = std::dynamic_pointer_cast<LocalScope>(child->scope);
                blockScope->parentIsConditional = true;
            } 
            else if (child->getNodeType() == GazpreaParser::ELSEIF_TOKEN) {
                auto blockScope = std::dynamic_pointer_cast<LocalScope>(child->children[1]->scope);
                blockScope->parentIsConditional = true;
            } 
            else if (child->getNodeType() == GazpreaParser::ELSE_TOKEN) {
                auto blockScope = std::dynamic_pointer_cast<LocalScope>(child->children[0]->scope);
                blockScope->parentIsConditional = true;
            }
        }  
        return;
    }

    void DefWalk::visitGenerator(std::shared_ptr<AST> t) {  
        auto generatorScope = std::make_shared<LocalScope>(currentScope);
        currentScope = generatorScope; // push scope 
        t->scope = generatorScope;
        for (size_t i = 0; i < t->children[0]->children.size(); i++) { 
            visit(t->children[0]->children[i]); //visit each domain expression  
        }
        visit(t->children[t->children.size()-1]); //visit expression
        currentScope = currentScope->getEnclosingScope(); // pop scope  
        return;
    }

    void DefWalk::visitFilter(std::shared_ptr<AST> t) {
        auto filterScope = std::make_shared<LocalScope>(currentScope);
        currentScope = filterScope;
        t->scope = filterScope; 
        visitChildren(t);  
        currentScope = currentScope->getEnclosingScope();
        return;
    }

    void DefWalk::visitDomainExpression(std::shared_ptr<AST> t) {
        visit(t->children[0]);
        std::shared_ptr<AST> identifierAST = t->children[0];
        auto vs = std::make_shared<VariableSymbol>(identifierAST->parseTree->getText(), nullptr);
        vs->def = t;  // track AST location of def's ID (i.e., where in AST does this symbol defined)
        t->symbol = vs;   // track in AST
        currentScope->define(vs);

        auto temp = currentScope;
        currentScope = temp->getEnclosingScope();
        visit(t->children[1]);
        currentScope = temp;
    }
} // namespace gazprea