#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct ClumpyCommand {

    virtual ~ClumpyCommand() {}

    virtual std::string usage() const = 0;
    virtual std::string description() const = 0;
    virtual std::string example() const = 0;
    virtual bool exec(std::vector<std::string> args) = 0;

    using FactoryFn = std::function<ClumpyCommand*()>;

    using Registry = std::unordered_map<std::string, FactoryFn>;

    static Registry& registry() {
        static Registry r;
        return r;
    }

    struct Register {
        Register(std::string id, FactoryFn createCmd) { registry()[id] = createCmd; }
    };
};
