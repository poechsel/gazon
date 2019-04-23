#pragma once

#include <functional>
#include <iostream>

/* Substitute for golang's defer.
   Execute some code when we leave the current scope
   (on exception / return...)

   Usage: DEFER(codetoexecute);
*/

class Defer {
 public:
    std::function<void()> function;
    ~Defer() {
        function();
    }
    Defer(std::function<void()> f): function(f) {}
};

#define DEFER(f) Defer _defer_##__line__([&](){f;})
