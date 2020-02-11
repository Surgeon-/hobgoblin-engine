#ifndef UHOBGOBLIN_QAO_REGISTRY_HPP
#define UHOBGOBLIN_QAO_REGISTRY_HPP

#include <Hobgoblin_include/QAO/config.hpp>
#include <Hobgoblin_include/Utility/SlabIndexer.hpp>

#include <cstdint>
#include <memory>
#include <set>
#include <vector>

#include <Hobgoblin_include/Private/pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace qao {

class QAO_Base;

namespace detail {

class QAO_Registry {
public:
    QAO_Registry(int capacity = 1);

    QAO_SerialIndexPair insert(std::unique_ptr<QAO_Base> ptr);
    QAO_SerialIndexPair insertNoOwn(QAO_Base* ptr);
    std::unique_ptr<QAO_Base> release(int index);
    void erase(int index);

    int size() const;
    QAO_Base* objectAt(int index) const;
    std::int64_t serialAt(int index) const;
    int instanceCount() const;
    bool isSlotEmpty(int index) const;

private:
    struct Elem {
        std::int64_t serial;
        std::unique_ptr<QAO_Base> ptr;
        bool no_own = false;

        Elem() = default;

        Elem(const Elem&) = default;
        Elem& operator=(const Elem&) = default;

        Elem(Elem&&) = default;
        Elem& operator=(Elem&&) = default;

        ~Elem() {
            if (no_own) {
                ptr.release();
            }
        }
    };

    util::SlabIndexer _indexer;
    std::vector<Elem> _elements;
    std::int64_t _serial_counter;

    void adjustSize();
    std::int64_t nextSerial();
};

} // namespace detail
} // namespace qao
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin_include/Private/pmacro_undef.hpp>

#endif // !UHOBGOBLIN_QAO_REGISTRY_HPP