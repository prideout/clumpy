#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec2.hpp>
#include <glm/ext.hpp>

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#include <limits>

using namespace glm;

using std::vector;
using std::string;
using std::numeric_limits;

namespace {

struct SimulatePendulum : ClumpyCommand {
    SimulatePendulum() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "generate a θ-ω field of 2D vectors";
    }
    string usage() const override {
        return "<dims> <friction> <output_img>";
    }
    string example() const override {
        return "500x500 0.01 field.npy";
    }
};

static ClumpyCommand::Register registrar("simulate_pendulum", [] {
    return new SimulatePendulum();
});

bool SimulatePendulum::exec(vector<string> vargs) {
    if (vargs.size() != 3) {
        fmt::print("The command takes 6 arguments.\n");
        return false;
    }
    const string dims = vargs[0];
    const float friction = atof(vargs[1].c_str());
    const string output_file = vargs[2];
    const uint32_t width = atoi(dims.c_str());
    const uint32_t height = atoi(dims.substr(dims.find('x') + 1).c_str());

    vector<vec2> dstimg(width * height);
    for (uint32_t row = 0; row < width; ++row) {
        for (uint32_t col = 0; col < height; ++col) {
            const float x = float(col) / width;
            const float y = float(row) / height;
            // ...
        }
    }

    cnpy::npy_save(output_file, &dstimg.data()->x, {height, width, 2}, "w");

    return true;
}

} // anonymous namespace
