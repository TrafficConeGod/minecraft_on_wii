#pragma once
#include "gfx.hpp"

namespace dbg {
    void freeze();

    template<typename F>
    inline void error(F func) {
        gfx::console_state console;
        func();
        freeze();
    }

    template<typename F1, typename F2, typename ...Args>
    inline void try_catch(const F1& try_func, const F2& catch_func, Args&& ...args) {
        auto [ success, code ] = try_func(std::forward<Args>(args)...);
        if (!success) {
            gfx::console_state console;
            catch_func(code, std::forward<Args>(args)...);
            freeze();
        }
    }

    struct non_copyable {
        non_copyable() = default;
        non_copyable(const non_copyable&) = delete;
        non_copyable& operator=(const non_copyable&) = delete;
        non_copyable(non_copyable&&) {}
        non_copyable& operator=(non_copyable&&) { return *this; }
    };
}