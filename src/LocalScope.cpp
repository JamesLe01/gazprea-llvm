#include "LocalScope.h"

namespace gazprea {
    LocalScope::LocalScope(std::shared_ptr<Scope> parent) : BaseScope(parent) {}

    std::string LocalScope::getScopeName() {
        return "local";
    }
}