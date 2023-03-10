#include "exceptions.h"

namespace gazprea {

GazpreaError::GazpreaError(std::string descr, std::string errorText, std::string fullText, int line, int charPos){
    std::stringstream sstream;
    sstream <<  descr
        << "\033[36m" << errorText << "\033[0m" << 
        " " << line << "\033[0m"<< ":" << charPos << "\n"
        << "\t|\n"
        << line << "\t|" << " '" << fullText << "'\n"
        << "\t|\n"; 

    msg = sstream.str();
}

//Binary op between incompatible Types Error
BinaryOpTypeError::BinaryOpTypeError(std::string lhs, std::string rhs, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "cannot perform binary operation between "
            << "\033[36m" << lhs  << "\033[0m" << 
            " and " 
            << "\033[36m" << rhs  << "\033[0m" <<  
            " " << line << "\033[0m"<< ":" << charPos << "\n"
            << "\t  | \n" << "" << line << "\t  |" << " '" << fullText << "'\n" << "\t  | \n";
    msg = sstream.str();
} 

//Tuples of Incompatible Size Error 
TupleSizeError::TupleSizeError(std::string lhs, std::string rhs, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "incompatible tuples sizes for "
            << "\033[36m" << lhs  << "\033[0m" << 
            " and " 
            << "\033[36m" << rhs  << "\033[0m" <<  
            " " << line << "\033[0m"<< ":" << charPos << "\n"

            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
} 

//Parallel Assignment Error
ParallelAssignmentError::ParallelAssignmentError(std::string rhs, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "invlaid value in parallel assignement "
            << "\033[36m" << rhs << "\033[0m" << 
            " is not of type: " 
            << "\033[36m" << "tuple" << "\033[0m" <<  
            " " << line << "\033[0m"<< ":" << charPos << "\n"

            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
}

UndefinedIdError::UndefinedIdError(std::string id, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "id '"
            << "\033[36m" << id << "\033[0m" << 
            "' can not be resolved in this scope" 
            " " << line << "\033[0m"<< ":" << charPos << "\n"
            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
}

RedefineIdError::RedefineIdError(std::string id, std::string fullText, int line, int charPos){
    std::stringstream sstream;
    sstream << "previously defined id '"
        << "\033[36m" << id << "\033[0m" << 
        "' can not be redefined in this scope" 
        " " << line << "\033[0m"<< ":" << charPos << "\n"
        << "\t|\n"
        << line << "\t|" << " '" << fullText << "'\n"
        << "\t|\n";
    msg = sstream.str();
}

MissingMainProcedureError::MissingMainProcedureError(std::string main) {
    std::stringstream sstream;
    sstream << "missing definition of procedure " 
            << "\033[36m" << main << "\033[0m"; 
    msg = sstream.str();
}

MainReturnIntegerError::MainReturnIntegerError(std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "procedure "
            << "\033[36m" << "main" << "\033[0m" << 
            " must return type integer " 
            " " << line << "\033[0m"<< ":" << charPos << "\n"
            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
}

BadReturnTypeError::BadReturnTypeError(std::string nodeText, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "must return type '"
            << "\033[36m" << nodeText << "\033[0m" << 
            "' in current procedure/function" 
            " " << line << "\033[0m"<< ":" << charPos << "\n"
            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
}

MainArgumentsPresentError::MainArgumentsPresentError(std::string fullText, int line, int charPos){
    std::stringstream sstream;
    sstream << "procedure main can must have zero arguments"
            " " << line << "\033[0m"<< ":" << charPos << "\n"
            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
}


GlobalVariableQualifierError::GlobalVariableQualifierError(std::string message, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << message << " " << line << "\033[0m"<< ":" << charPos << "\n"
            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
}

InvalidArgumentError::InvalidArgumentError(std::string nodeText, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "wrong arguments passed to procedure/function "
        << "\033[36m" << nodeText << "\033[0m" << 
        " " << line << "\033[0m"<< ":" << charPos << "\n"
        << "\t|\n"
        << line << "\t|" << " '" << fullText << "'\n"
        << "\t|\n"; 

    msg = sstream.str();
}

ConstAssignmentError::ConstAssignmentError(std::string nodeText, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "can not reassign value to const variable "
        << "\033[36m" << nodeText << "\033[0m" << 
        " " << line << "\033[0m"<< ":" << charPos << "\n"
        << "\t|\n"
        << line << "\t|" << " '" << fullText << "'\n"
        << "\t|\n"; 

    msg = sstream.str();
}

InvalidDeclarationError::InvalidDeclarationError(std::string nodeText, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "type of the variabel being assigned to unknown "
        << "\033[36m" << nodeText << "\033[0m" << 
        " " << line << "\033[0m"<< ":" << charPos << "\n"
        << "\t|\n"
        << line << "\t|" << " '" << fullText << "'\n"
        << "\t|\n"; 

    msg = sstream.str();
}

IncompatibleTypeError::IncompatibleTypeError(std::string varType, std::string exprType, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "Cannot assign value of type "
        << "\033[36m" << exprType << "\033[0m" << 
            " to variable of type " 
        << "\033[36m" << varType << "\033[0m" << 
        " " << line << "\033[0m"<< ":" << charPos << "\n"
        << "\t|\n"
        << line << "\t|" << " '" << fullText << "'\n"
        << "\t|\n"; 

    msg = sstream.str();
}


} // namespace gazprea

