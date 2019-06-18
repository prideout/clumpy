#define PAR_SHAPES_T uint32_t
#define PAR_SHAPES_IMPLEMENTATION
#include "par/par_shapes.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#include "clumpy_command.hh"
#include "fmt/core.h"

#include <string>

using namespace std;

constexpr auto kDescription = R"(
Welcome to clumpy. This tool can process or generate large swaths of image
data. It is meant to be used in conjunction with Python.
)";

void help() {
    auto& reg = ClumpyCommand::registry();
    fmt::print("{}\n", kDescription);
    fmt::print("{:20} {}\n", "help", "list all commands and their arguments");
    fmt::print("{:20} {}\n", "examples", "print a usage example for each command");
    for (auto r : reg) {
        auto cmd = r.second();
        fmt::print("{:20} {}\n", r.first, cmd->description());
        if (not cmd->usage().empty()) {
            fmt::print("{:20} usage: {} {}\n", "", r.first, cmd->usage());
        }
        delete cmd;
    }
}

void examples() {
    auto& reg = ClumpyCommand::registry();
    for (auto r : reg) {
        auto cmd = r.second();
        fmt::print("clumpy {} {}\n", r.first, cmd->example());
        delete cmd;
    }
}

int main(const int argc, const char *argv[]) {
    if (argc <= 1 || !strcmp(argv[1], "help")) {
        help();
        return 0;
    }
    if (!strcmp(argv[1], "examples")) {
        examples();
        return 0;
    }
    auto& reg = ClumpyCommand::registry();
    auto iter = reg.find(argv[1]);
    if (iter == reg.end()) {
        help();
        return 1;
    }
    auto cmd = iter->second();
    vector<string> all_args;
    for (int i = 2; i < argc; i++) {
        all_args.emplace_back(argv[i]);
    }
    bool success = cmd->exec(all_args);
    delete cmd;
    return success ? 0 : 1;
}
