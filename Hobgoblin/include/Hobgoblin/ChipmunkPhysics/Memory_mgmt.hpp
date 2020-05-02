#ifndef UHOBGOBLIN_CHIPMUNKPHYSICS_MEMORY_MGMT_HPP
#define UHOBGOBLIN_CHIPMUNKPHYSICS_MEMORY_MGMT_HPP

#include <Hobgoblin/Private/chipmunk/chipmunk.h>

#include <cassert>
#include <memory>
#include <type_traits>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace cp {

namespace detail {

inline
void cpCleanUpAndDelete(cpShape* shape) { // TODO To .cpp file 
    cpSpace* space = cpShapeGetSpace(shape);
    if (space) {
        cpSpaceRemoveShape(space, shape);
    }
    cpShapeFree(shape);
}

inline
void cpCleanUpAndDelete(cpBody* body) { // TODO To .cpp file 
    cpSpace* space = cpBodyGetSpace(body);
    if (space) {
        cpSpaceRemoveBody(space, body);
    }
    cpBodyFree(body);
}

inline
void cpCleanUpAndDelete(cpConstraint* constraint) { // TODO To .cpp file 
    assert(0 && "Not implemented");
}

template <class T>
struct cpGenericDeleter {
    static_assert(std::is_same_v<T, cpBody> ||
                  std::is_same_v<T, cpShape> ||
                  std::is_same_v<T, cpConstraint>,
                  "Incompatible type provided");

    void operator()(T* object) {
        cpCleanUpAndDelete(object);
    }
};

using cpBodyDeleter = cpGenericDeleter<cpBody>;
using cpShapeDeleter = cpGenericDeleter<cpShape>;
using cpConstraintDeleter = cpGenericDeleter<cpConstraint>;

} // namespace detail

using cpBodyUPtr = std::unique_ptr<cpBody, detail::cpBodyDeleter>;
using cpShapeUPtr = std::unique_ptr<cpShape, detail::cpShapeDeleter>;
using cpConstraintUPtr = std::unique_ptr<cpConstraint, detail::cpConstraintDeleter>;

/*BodyUniquePtr MakeBodyUniquePtr(cpSpace* space, cpBody* body = nullptr) {
    return BodyUniquePtr(body, BodyDeleter{space});
}

ShapeUniquePtr MakeShapeUniquePtr(cpSpace* space, cpShape* shape = nullptr) {
    return ShapeUniquePtr(shape, ShapeDeleter{space});
}

ConstraintUniquePtr MakeConstraintUniquePtr(cpSpace* space, cpConstraint* constraint = nullptr) {
    return ConstraintUniquePtr(constraint, ConstraintDeleter{space});
}*/

} // namespace cp
HOBGOBLIN_NAMESPACE_END

#endif // !UHOBGOBLIN_CHIPMUNKPHYSICS_MEMORY_MGMT_HPP
