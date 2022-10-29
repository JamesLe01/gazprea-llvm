grammar Gazprea;

tokens {
    SCALAR_VAR_DECLARATION_TOKEN,
    VECTOR_VAR_DECLARATION_TOKEN,
    TUPLE_VAR_DECLARATION_TOKEN,
    ASSIGNMENT_TOKEN,
    CONDITIONAL_TOKEN,
    ELSEIF_TOKEN,
    ELSE_TOKEN,
    INFINITE_LOOP_TOKEN,
    PRE_PREDICATE_LOOP_TOKEN,
    POST_PREDICATE_LOOP_TOKEN,
    ITERATOR_LOOP_TOKEN,
    INPUT_STREAM_TOKEN,
    OUTPUT_STREAM_TOKEN,
    GENERATOR_TOKEN,
    GENERATOR_DOMAIN_VARIABLE_TOKEN,
    GENERATOR_DOMAIN_VARIABLE_LIST_TOKEN,
    FILTER_TOKEN,
    FILTER_PREDICATE_TOKEN,
    EXPR_TOKEN,
    BLOCK_TOKEN,
    INDEXING_TOKEN,
    TUPLE_ACCESS_TOKEN,
    TYPE_QUALIFIER_TOKEN,
    UNARY_TOKEN,
    CALL_PROCEDURE_STATEMENT,
    CALL_PROCEDURE_FUNCTION_IN_EXPRESSION,
    STRING_CONCAT_TOKEN,
    EXPRESSION_LIST_TOKEN,
    TUPLE_LITERAL_TOKEN,
    SCALAR_TYPE_TOKEN,
    VECTOR_TYPE_TOKEN,
    TUPLE_TYPE_TOKEN,
    TYPE_TOKEN,
    CAST_TOKEN,
    VECTOR_SIZE_DECLARATION_LIST_TOKEN,
    TUPLE_TYPE_DECLARATION_ATOM,
    TUPLE_TYPE_DECLARATION_LIST,
    FUNCTION_DECLARATION_RETURN_TOKEN,
    PROCEDURE_DECLARATION_RETURN_TOKEN,
    FORMAL_PARAMETER_TOKEN,
    FORMAL_PARAMETER_LIST_TOKEN
}

compilationUnit: statement* EOF;

// statement: 'test';
statement: varDeclarationStatement
        | assignmentStatement
        | conditionalStatement
        | infiniteLoopStatement
        | prePredicatedLoopStatement
        | postPredicatedLoopStatement
        | breakStatement
        | continueStatement
        | iteratorLoopStatement
        | streamStatement
        | functionDeclarationDefinition
        | procedureDeclarationDefinition
        | callProcedure
        | typedefStatement
        ;

// Type and Type Qualifier
vectorSizeDeclarationAtom: '*' | expression ;
vectorSizeDeclarationList: vectorSizeDeclarationAtom (',' vectorSizeDeclarationAtom)? ;
tupleTypeDeclarationAtom: nonTupleType Identifier? ;
tupleTypeDeclarationList: tupleTypeDeclarationAtom (',' tupleTypeDeclarationAtom)* ;

scalarType: BOOLEAN | CHARACTER | INTEGER | REAL | STRING | INTERVAL | (INTEGER INTERVAL);
vectorType: scalarType '[' vectorSizeDeclarationList ']' ;
nonTupleType: scalarType | vectorType ;
tupleType: TUPLE '(' tupleTypeDeclarationList ')' ;
type: scalarType | vectorType | tupleType ;

typeQualifier: VAR | CONST ;

// typedef
typedefStatement: TYPEDEF type Identifier ';' ;

// Variable Declaration and Assignment
varDeclarationStatement:
    typeQualifier? scalarType Identifier ('=' expression)? ';'     # ScalarVarDeclaration
    | typeQualifier? vectorType Identifier ('=' expression)? ';'   # VectorVarDeclaration
    | typeQualifier? tupleType Identifier ('=' expression)? ';'    # TupleVarDeclaration
    ;
assignmentStatement: Identifier '=' expression ';' ;

// Function and Procedure
expressionList: expression (',' expression)* ;
formalParameter: typeQualifier? type Identifier ;
formalParameterList: formalParameter (',' formalParameter)* ;
functionDeclarationDefinition: FUNCTION Identifier '(' formalParameterList? ')' functionDeclarationReturn ';' ;
procedureDeclarationDefinition: PROCEDURE Identifier '(' formalParameterList? ')' procedureDeclarationReturn ';' ;
functionDeclarationReturn: RETURNS type (('=' expression) | block)? ;
procedureDeclarationReturn: (RETURNS type)? block? ;
callProcedure: CALL Identifier '(' expressionList? ')' ;

// Conditional
conditionalStatement: IF '('? expression ')'? (statement | block) elseIfStatement* elseStatement? ;
elseIfStatement: ELSE IF '('? expression ')'? (statement | block) ;
elseStatement: ELSE (statement | block) ;
// 
// Loop
infiniteLoopStatement: LOOP (statement | block) ;
prePredicatedLoopStatement: LOOP WHILE expression (statement | block) ;
postPredicatedLoopStatement: LOOP (statement | block) WHILE expression ;
iteratorLoopStatement: LOOP Identifier IN expression (statement | block) ;
// 
// Break and Continue
breakStatement: BREAK ';' ;
continueStatement: CONTINUE ';' ;
// 
// Stream
streamStatement:
    expression '->' STD_OUTPUT ';'      # OutputStream
    | expression '<-' STD_INPUT ';'     # InputStream
    ;
// 
// Block
block: '{' statement* '}' ;
// 
// Expression
expression: expr ;
expr: 
    Identifier '(' expressionList? ')'                                         # CallProcedureFunctionInExpression
    | AS '<' type '>' '(' expression ')'                                       # Cast
    | '(' expressionList ')'                                                   # TupleLiteral
    | expr '.' expr                                                            # TupleAccess
    | '(' expr ')'                                                             # Parenthesis
    | expr '[' expr ']'                                                        # Indexing
    | expr '..' expr                                                           # Interval
    | <assoc=right> op=('+' | '-' | 'not') expr                                # UnaryOp
    | <assoc=right> expr op='^' expr                                           # BinaryOp
    | expr op=('*' | '/' | '%' | '**') expr                                    # BinaryOp
    | expr op=('+' | '-') expr                                                 # BinaryOp
    | expr op='by' expr                                                        # BinaryOp
    | expr op=('>' | '<' | '<=' | '>=') expr                                   # BinaryOp
    | expr op=('==' | '!=') expr                                               # BinaryOp
    | expr op='and' expr                                                       # BinaryOp
    | expr op=('or' | 'xor') expr                                              # BinaryOp
    | <assoc=right> expr '||' expr                                             # Concatenation
    | '[' generatorDomainVariableList '|' expression ']'                       # Generator
    | '[' Identifier IN expression '&' filterPredicate ']'                     # Filter
    | Identifier                                                               # IdentifierAtom
    | IntegerConstant                                                          # IntegerAtom
    | RealConstant                                                             # RealAtom
    | CharacterConstant                                                        # CharacterAtom
    | StringLiteral                                                            # StringLiteralAtom
    | IDENTITY                                                                 # IdentityAtom
    ;
//
// Generator and Filter
generatorDomainVariable: Identifier IN expression ;
generatorDomainVariableList: generatorDomainVariable (',' generatorDomainVariable)? ;
filterPredicate: expression (',' expression)* ;
// 
// Reserve Keywords
AND : 'and' ;
AS : 'as' ;
BOOLEAN: 'boolean' ;
BREAK : 'break' ;
BY: 'by' ;
CALL : 'call' ;
CHARACTER : 'character' ;
COLUMNS : 'columns' ;
CONST : 'const' ;
CONTINUE : 'continue' ;
ELSE : 'else' ;
FALSE : 'false' ;
FUNCTION: 'function' ;
IDENTITY : 'identity' ;
IF: 'if' ;
IN: 'in' ;
INTEGER: 'integer' ;
INTERVAL: 'interval' ;
LENGTH : 'length' ;
LOOP : 'loop' ;
NOT : 'not' ;
NULL_TOKEN : 'null' ;
OR : 'or' ;
PROCEDURE : 'procedure' ;
REAL : 'real' ;
RETURN : 'return' ;
RETURNS : 'returns' ;
REVERSE : 'reverse' ;
ROWS: 'rows' ;
STD_INPUT : 'std_input' ;
STD_OUTPUT : 'std_output' ;
STREAM_STATE : 'stream_state' ;
STRING : 'string' ;
TRUE : 'true' ;
TUPLE : 'tuple' ;
TYPEDEF: 'typedef' ;
VAR : 'var' ;
WHILE : 'while' ;
XOR : 'xor' ;
//
PLUS: '+' ;
MINUS: '-' ;
ASTERISK: '*' ;
DIV: '/' ;
MODULO: '%' ;
DOTPRODUCT: '**' ;
LESSTHAN: '<' ;
GREATERTHAN: '>' ;
LESSTHANOREQUAL: '<=' ;
GREATERTHANOREQUAL: '>=' ;
ISEQUAL: '==' ;
ISNOTEQUAL: '!=' ;
// 
Identifier: [a-zA-Z_][a-zA-Z0-9_]* ;
// 
// Integer and Real
IntegerConstant : DigitSequence;
RealConstant: FractionalConstant ExponentPart?;
fragment FractionalConstant:
    DigitSequence? '.' DigitSequence
    |   DigitSequence '.'
    ;
fragment ExponentPart: 'e' Sign? DigitSequence ;
fragment Sign: [+-] ;
fragment DigitSequence: Digit+ ;
fragment Digit: [0-9] ;
// 
// String and Char
CharacterConstant: '\'' CChar '\'' ;
fragment CChar
    :   ~[']
    |   EscapeSequence
    ;
// 
StringLiteral: '"' SCharSequence? '"' ;
fragment SCharSequence: SChar+ ;
fragment SChar
    :   ~["]
    |   EscapeSequence
    ;
fragment EscapeSequence: '\\' ['"0abnrt\\] ;
//
// Comment
LineComment : '//' ~[\r\n]* -> skip ;
BlockComment: '/*' .*? '*/' -> skip ;
// Skip whitespace
WS : [ \t\r\n]+ -> skip ;