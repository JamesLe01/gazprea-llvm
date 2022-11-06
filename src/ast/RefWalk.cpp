#include "RefWalk.h"

typedef int AType;
void hello(AType, int b) {

}

namespace gazprea {

    RefWalk::RefWalk(std::shared_ptr<SymbolTable> symtab) : symtab(symtab), currentScope(symtab->globals) {
    }
    RefWalk::~RefWalk() {}

    void RefWalk::visit(std::shared_ptr<AST> t) {
        if(!t->isNil()){
            switch(t->getNodeType()){
                //Statements & Intermediate Tokens
                case GazpreaParser::PROCEDURE:
                case GazpreaParser::FUNCTION:
                    visitSubroutineDeclDef(t);
                    break;
                case GazpreaParser::TYPEDEF: visitTypedefStatement(t); break;
                case GazpreaParser::IDENTIFIER_TOKEN: visitIdentifier(t); break;
                case GazpreaParser::VAR_DECLARATION_TOKEN:
                    visitVariableDeclaration(t);
                    break;
                case GazpreaParser::VECTOR_TYPE_TOKEN:
                    visitVectorMatrixType(t);
                    break;
                case GazpreaParser::SINGLE_TOKEN_TYPE_TOKEN:
                    visitSingleTokenType(t);
                    break;
                case GazpreaParser::UNQUALIFIED_TYPE_TOKEN:
                    visitUnqualifiedType(t);
                    break;
                default: visitChildren(t);
            };
        }
        else {
            visitChildren(t); // t must be the null root node 
        }
    }

    void RefWalk::visitChildren(std::shared_ptr<AST> t) { 
        for(auto stat : t->children){
            visit(stat);
        }
    }

    void RefWalk::visitVariableDeclaration(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto variableDeclarationSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
        
        if (t->children[0]->getNodeType() == GazpreaParser::INFERRED_TYPE_TOKEN) {
            variableDeclarationSymbol->typeQualifier = t->children[0]->children[0]->parseTree->getText();
        } else {
            if (t->children[0]->children[0]->getNodeType() == GazpreaParser::TYPE_QUALIFIER_TOKEN) {
                variableDeclarationSymbol->typeQualifier = t->children[0]->children[0]->parseTree->getText();
                variableDeclarationSymbol->type = t->children[0]->children[1]->type;
            } else {
                variableDeclarationSymbol->type = t->children[0]->children[0]->type;
            }
        }
    }

    void RefWalk::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
        visitChildren(t);
    }

    void RefWalk::visitTypedefStatement(std::shared_ptr<AST> t) {
        visit(t->children[0]);
        auto typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol);
        typedefTypeSymbol->type = t->children[0]->type;  // visitChildren(t) already populated t->children[0]->type for us
    }

    void RefWalk::visitIdentifier(std::shared_ptr<AST> t) {
        t->symbol = t->scope->resolve(t->parseTree->getText());
    }

    void RefWalk::visitSingleTokenType(std::shared_ptr<AST> t) {
        auto text = t->parseTree->getText();
        t->type = std::dynamic_pointer_cast<Type>(symtab->globals->resolve(text));
        if (t->type->isTypedefType()) {
            auto typeDefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
            typeDefTypeSymbol->resolveTargetType();  // If TypeDef is already called resolveTargetType once, this method will do nothing
        }
    }

    void RefWalk::visitVectorMatrixType(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->type = std::make_shared<MatrixType>(t->children[0]->type, (t->children[1]->children).size(), t);
    }

    void RefWalk::visitUnqualifiedType(std::shared_ptr<AST> t) {
        visitChildren(t);
        if ((t->children).size() == 2) {
            // Interval Type
            t->type = std::make_shared<IntervalType>(t->children[0]->type, t->children[1]->type);
            return;
        } else {
            // Not an interval type
            t->type = t->children[0]->type;
        }
    }
} // namespace gazprea 