
// IMPORTANT: Always include this as the last header file

// General:
#define HOBGOBLIN_NAMESPACE_START namespace jbatnozic { namespace hobgoblin {
#define HOBGOBLIN_NAMESPACE_END }}
#define NO_COPY public util::NonCopyable
#define NO_MOVE public util::NonMoveable
#define PASSKEY {}
#define SELF (*this)
#define SWITCH_FALLTHROUGH (void)0
#define NO_OP() (void)0
#define CRTP_METHOD(_type_param_, _name_, ...) { return static_cast<_type_param_*>(this)->_name_(__VA_ARGS__); }
#define FRIEND_ACCESS

// Compiler-specific:
#if defined(_MSC_VER)
#define CURRENT_FUNCTION __FUNCSIG__
#else
#define CURRENT_FUNCTION __PRETTY_FUNCTION__
#endif