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

enum ImageType { pendulum, field };

struct SimulatePendulum : ClumpyCommand {
    SimulatePendulum() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "generate a θ-ω field of 2D vectors";
    }
    string usage() const override {
        return "<image_type> <dims> <friction> <output_img>";
    }
    string example() const override {
        return "field 500x500 0.01 field.npy";
    }
};

static ClumpyCommand::Register registrar("simulate_pendulum", [] {
    return new SimulatePendulum();
});

bool SimulatePendulum::exec(vector<string> vargs) {
    if (vargs.size() != 4) {
        fmt::print("The command takes 4 arguments.\n");
        return false;
    }
    const string image_typestring = vargs[0];
    const string dims = vargs[1];
    const float friction = atof(vargs[2].c_str());
    const string output_file = vargs[3];
    const uint32_t width = atoi(dims.c_str());
    const uint32_t height = atoi(dims.substr(dims.find('x') + 1).c_str());

    ImageType image_type;
    if (image_typestring == "pendulum") {
        image_type = pendulum;
    } else if (image_typestring == "field") {
        image_type = field;
    } else {
        fmt::print("Image type must be 'pendulum' or 'field'.\n");
        return false;
    }

    const float g = 1.0f;
    const float L = 1.0f;
    const vec2 graph_scale(M_PI / 0.5, 5);

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
