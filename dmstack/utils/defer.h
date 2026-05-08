#ifndef DEFER_H
#define DEFER_H

#include <utility>  // std::move

template<typename FnType>
class ScopedLambda
{
public:
    // Explicit constructor
    explicit ScopedLambda(FnType fn) : fn_(std::move(fn)) { }

    // Default movable constructor and operator=
    ScopedLambda(ScopedLambda&&) = default;
    ScopedLambda& operator=(ScopedLambda&&) = default;

    // Non-cppyable
    ScopedLambda(const ScopedLambda&) = delete;
    ScopedLambda& operator=(const ScopedLambda&) = delete;

    ~ScopedLambda() { fn_(); }

private:
    FnType fn_;
};

template<typename FnType>
ScopedLambda<FnType> MakeScopedLambda(FnType fn)
{
    return ScopedLambda<FnType>(std::move(fn));
}

#define TOKEN_PASTE(x, y) x ## y
#define TOKEN_PASTE2(x, y) TOKEN_PASTE(x, y)
#define SCOPE_UNIQUE_NAME(name) TOKEN_PASTE2(name, __LINE__)
#define DEFER(...)                                                              \
    auto SCOPE_UNIQUE_NAME(defer_var) =                                         \
        MakeScopedLambda([&] { __VA_ARGS__; })


#endif
