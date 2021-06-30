#ifndef SPEMPE_GAME_CONTEXT_CONTEXT_COMPONENTS_HPP
#define SPEMPE_GAME_CONTEXT_CONTEXT_COMPONENTS_HPP

#include <Hobgoblin/Utility/String_utils.hpp>

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace jbatnozic {
namespace spempe {

//! A class that wants to be registered as a Component in a GameContext
//! must inherit from ContextComponent. It must also be tagged (see
//! macro SPEMPE_CTXCOMP_TAG()).
class ContextComponent {
public:
    using TagHash = std::size_t;

    virtual ~ContextComponent();
};

//! Define a component's tag.
//! For example: SPEMPE_CTXCOMP_TAG("PhysicsManager"); (can be in the private section)
#define SPEMPE_CTXCOMP_TAG(_tag_string_) \
    ::std::string __spempeimpl_getComponentTag() const { \
        return ::std::string{_tag_string_}; \
    } \
    ::jbatnozic::spempe::ContextComponent::TagHash __spempeimpl_getComponentTagHash() const { \
        constexpr static auto TAG_HASH_ = \
            ::jbatnozic::hobgoblin::util::HornerHash(_tag_string_); \
        return TAG_HASH_; \
    } \
    friend class ::jbatnozic::spempe::detail::ComponentTable

namespace detail {

class ComponentTable {
public:
    //! Number of slots in the component hash table.
    ComponentTable(std::size_t aTableSize);

    //! Throws hg::TracedLogicError in case of tagHash clash.
    template <class taComponent>
    void attachComponent(taComponent& aComponent);

    //! No-op if component isn't present.
    void detachComponent(ContextComponent& aComponent);

    //! Throws hg::TracedLogicError if the component isn't present.
    template <class taComponent>
    taComponent& getComponent() const;

    std::string toString(char aSeparator = '\n') const;

private:
    struct Node {
        ContextComponent* component = nullptr;
        ContextComponent::TagHash tagHash = 0;
        std::string tag;
    };

    std::vector<Node> _table;

    void _attachComponent(ContextComponent& aComponent,
                          std::string aTag,
                          ContextComponent::TagHash aTagHash);

    ContextComponent& _getComponent(ContextComponent::TagHash aTagHash) const;
};

template <class taComponent>
void ComponentTable::attachComponent(taComponent& aComponent) {
    const auto tag = aComponent.__spempeimpl_getComponentTag();
    const auto tagHash = aComponent.__spempeimpl_getComponentTagHash();
    _attachComponent(aComponent, std::move(tag), tagHash);
}

template <class taComponent>
taComponent& ComponentTable::getComponent() const {
    //! Ugly hack that relies on the fact that __spempeimpl_getComponentTagHash
    //! doesn't actually at any point dereference 'this'.
    const auto tagHash = 
        reinterpret_cast<taComponent*>(0x12345678)->__spempeimpl_getComponentTagHash();
    return static_cast<taComponent&>(_getComponent(tagHash));
}

} // namespace detail

} // namespace spempe
} // namespace jbatnozic

#endif // !SPEMPE_GAME_CONTEXT_CONTEXT_COMPONENTS_HPP