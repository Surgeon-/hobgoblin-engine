
#include <Hobgoblin/RigelNet/Handlermgmt.hpp>
#include <Hobgoblin/RigelNet/Node.hpp>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace rn {

// TODO Node impl.

namespace detail {

void HandleDataMessages(RN_Node& node, RN_PacketWrapper& packetWrap) {
    node._currentPacket = &packetWrap;

    while (!packetWrap.packet.endOfPacket()) {
        const RN_HandlerId handlerId = packetWrap.extractOrThrow<RN_HandlerId>();
        RN_HandlerFunc handlerFunc = RN_GlobalHandlerMapper::getInstance().handlerWithId(handlerId);
        (*handlerFunc)(node);
    }

    node._currentPacket = nullptr;
}

} // namespace detail

RN_Node::RN_Node(RN_NodeType nodeType)
    : _currentPacket{nullptr}
    , _nodeType{nodeType}
{
}

RN_NodeType RN_Node::getType() const noexcept {
    return _nodeType;
}

} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
