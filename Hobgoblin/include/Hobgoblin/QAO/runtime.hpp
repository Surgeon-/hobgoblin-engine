#ifndef UHOBGOBLIN_QAO_RUNTIME_HPP
#define UHOBGOBLIN_QAO_RUNTIME_HPP

#include <Hobgoblin/common.hpp>
#include <Hobgoblin/QAO/config.hpp>
#include <Hobgoblin/QAO/id.hpp>
#include <Hobgoblin/QAO/orderer.hpp>
#include <Hobgoblin/QAO/registry.hpp>
#include <Hobgoblin/Utility/Any_ptr.hpp>
#include <Hobgoblin/Utility/NoCopyNoMove.hpp>
#include <Hobgoblin/Utility/Passkey.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <typeinfo>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace qao {

constexpr std::int32_t QAO_ALL_EVENT_FLAGS = 0xFFFFFFFF;

class QAO_Base;

class QAO_Runtime : NO_COPY, NO_MOVE {
public:
    QAO_Runtime();
    ~QAO_Runtime();

    // Object manipulation
    detail::RuntimeInsertInfo addObject(QAO_Base& obj);

    template <class T>
    T* addObject(std::unique_ptr<T> object);

    template <class T>
    T* addObjectNoOwn(T& object);

    std::unique_ptr<QAO_Base> releaseObject(QAO_GenericId id);
    std::unique_ptr<QAO_Base> releaseObject(QAO_Base* obj);
    void eraseObject(QAO_GenericId id);
    void eraseObject(QAO_Base* obj);

    QAO_Base* find(const std::string& name) const;
    QAO_Base* find(QAO_GenericId id) const;

    template<class T>
    T* find(QAO_Id<T> id) const;

    void updateExecutionPriorityForObject(QAO_GenericId id, int new_priority);
    void updateExecutionPriorityForObject(QAO_Base* object, int new_priority);

    // Execution
    void startStep();
    void advanceStep(bool& done, std::int32_t eventFlags = QAO_ALL_EVENT_FLAGS);
    QAO_Event::Enum getCurrentEvent();

    // Other
    PZInteger getObjectCount() const noexcept;
    std::int64_t getIterationOrdinal() const noexcept {
        return _iteration_ordinal; // TODO
    }

    // User data
    void setUserData(std::nullptr_t);

    template <class T>
    void setUserData(T* value);

    template <class T>
    T* getUserData() const;

    template <class T>
    T* getUserDataOrThrow() const;

    // TODO Orderer iterations

private:
    detail::QAO_Registry _registry;
    detail::QAO_Orderer _orderer;
    std::int64_t _step_counter;
    std::int64_t _iteration_ordinal;
    QAO_Event::Enum _current_event;
    QAO_OrdererIterator _step_orderer_iterator;
    util::AnyPtr _user_data;

    QAO_Base* addObjectImpl(std::unique_ptr<QAO_Base> object);
    QAO_Base* addObjectNoOwnImpl(QAO_Base& object);
};

template <class T>
T* QAO_Runtime::addObject(std::unique_ptr<T> object) {
    return static_cast<T*>(addObjectImpl(std::move(object)));
}

template <class T>
T* QAO_Runtime::addObjectNoOwn(T& object) {
    return static_cast<T*>(addObjectNoOwnImpl(object));
}

template<class T>
T* QAO_Runtime::find(QAO_Id<T> id) const {
    return static_cast<T>(find(QAO_GenericId{id}));
}

template <class T>
void QAO_Runtime::setUserData(T* value) {
    _user_data.reset(value);
}

template <class T>
T* QAO_Runtime::getUserData() const {
    return _user_data.get<T>();
}

template <class T>
T* QAO_Runtime::getUserDataOrThrow() const {
    return _user_data.getOrThrow<T>();
}

} // namespace qao
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_QAO_RUNTIME_HPP