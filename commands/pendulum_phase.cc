#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec2.hpp>
#include <glm/ext.hpp>

#include <limits>

using namespace glm;

using std::vector;
using std::string;
using std::numeric_limits;

namespace {

enum ImageType { pendulum, field };

struct PendulumPhase : ClumpyCommand {
    PendulumPhase() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "generate a θ-ω field of 2D vectors";
    }
    string usage() const override {
        return "<image_type> <dims> <friction> <scale_x> <scale_y> <output_img>";
    }
    string example() const override {
        return "500x500 0.01 1.0 5.0 field.npy";
    }
};

static ClumpyCommand::Register registrar("pendulum_phase", [] {
    return new PendulumPhase();
});

bool PendulumPhase::exec(vector<string> vargs) {
    if (vargs.size() != 5) {
        fmt::print("The command takes 5 arguments.\n");
        return false;
    }
    const string dims = vargs[0];
    const float friction = atof(vargs[1].c_str());
    const float scalex = atof(vargs[2].c_str());
    const float scaley = atof(vargs[3].c_str());
    const string output_file = vargs[4];
    const uint32_t width = atoi(dims.c_str());
    const uint32_t height = atoi(dims.substr(dims.find('x') + 1).c_str());

    const float g = 1.0f;
    const float L = 1.0f;
    const vec2 graph_scale(scalex * M_PI / 0.5, scaley * 1);

    vector<vec2> dstimg(width * height);
    for (uint32_t row = 0; row < height; ++row) {
        for (uint32_t col = 0; col < width; ++col) {
            const float x = graph_scale.x * (float(col) / width - 0.5);
            const float y = graph_scale.y * (float(row) / height - 0.5);
            const float theta = x;
            const float omega = y;
            const float omega_dot = -friction * omega - g / L * sin(theta);
            dstimg[row * width + col].x = omega;
            dstimg[row * width + col].y = omega_dot;
        }
    }

    cnpy::npy_save(output_file, &dstimg.data()->x, {height, width, 2}, "w");

    return true;
}

} // anonymous namespace
