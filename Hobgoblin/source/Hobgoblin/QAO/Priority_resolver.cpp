
#include <Hobgoblin/Utility/Exceptions.hpp>
#include <Hobgoblin/QAO/Priority_resolver.hpp>

#include <cassert>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace qao {

void QAO_PriorityResolver::CategoryDefinition::reset() {
    _dependees.clear();
    _priority.reset();
    _dependencyFulfilledCounter = 0;
}

bool QAO_PriorityResolver::CategoryDefinition::priorityAssigned() const {
    return _priority.has_value();
}

bool QAO_PriorityResolver::CategoryDefinition::dependenciesSatisfied() const {
    return (static_cast<int>(_dependencies.size()) == _dependencyFulfilledCounter);
}

int QAO_PriorityResolver::getPriorityOf(int categoryId) const {
    return _priorityMapping.at(categoryId);
}

QAO_PriorityResolver::CategoryDefinition& QAO_PriorityResolver::category(int categoryId) {
    for (auto& definition : _definitions) {
        if (FRIEND_ACCESS definition._categoryId == categoryId) {
            assert(false && "Definition with the same category ID already exists");
        }
    }

    _definitions.emplace_back(categoryId);
    return _definitions.back();
}

void QAO_PriorityResolver::resolveAll() {
    _priorityCounter = 0; // TODO Parametrize

    // Reset all definitions:
    for (auto& definition : _definitions) {
        FRIEND_ACCESS definition.reset();
    }

    // For all definitions, list them as dependees in their dependencies:
    for (auto definitionIter = _definitions.begin(); definitionIter != _definitions.end(); definitionIter++) {
        for (auto& dependencyId : FRIEND_ACCESS (*definitionIter)._dependencies) {
            auto dependencyIter = findDefinition(dependencyId);
            FRIEND_ACCESS (*dependencyIter)._dependees.push_back(definitionIter);
        }
    }

    // Try to resolve all:
    while (true) {
        // Find definition with all dependencies satisfied:
        std::vector<CategoryDefinition>::iterator curr;
        for (curr = _definitions.begin(); curr != _definitions.end(); curr = std::next(curr)) {
            if (FRIEND_ACCESS (*curr).priorityAssigned() == false &&
                FRIEND_ACCESS (*curr).dependenciesSatisfied()) {
                break;
            }
        }
        if (curr == _definitions.end()) {
            break;
        }

        for (auto& dependeeIter : FRIEND_ACCESS (*curr)._dependees) {
            (*dependeeIter)._dependencyFulfilledCounter += 1;
        }

        FRIEND_ACCESS (*curr)._priority = _priorityCounter;
        _priorityCounter -= 1; // TODO Parametrize
    }

    // Verify & populate mapping:
    _priorityMapping.clear();
    for (auto& definition : _definitions) {
        if (!definition.priorityAssigned()) {
            throw util::TracedLogicError("Cannot resolve priorities - impossible situation requested");
        }
        _priorityMapping[FRIEND_ACCESS definition._categoryId] = FRIEND_ACCESS *definition._priority;
        FRIEND_ACCESS definition.reset();
    }
}

std::vector<QAO_PriorityResolver::CategoryDefinition>::iterator QAO_PriorityResolver::findDefinition(int categoryId) {
    for (auto curr = _definitions.begin(); curr != _definitions.end(); curr = std::next(curr)) {
        if ((*curr)._categoryId == categoryId) {
            return curr;
        }
    }
    throw util::TracedLogicError("Definition with ID " + std::to_string(categoryId) + " not found");
}

} // namespace qao
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>