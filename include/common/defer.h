#pragma once

#include <functional>

class Defer {
 public:
    std::function<void()> function;
    ~Defer() {
        function();
    }
    Defer(std::function<void()> f): function(f) {}
};

#define DEFER(f) Defer _defer_##__line__([&](){f;})
