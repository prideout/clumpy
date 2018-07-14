#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec2.hpp>
#include <glm/ext.hpp>

using namespace glm;

using std::vector;
using std::string;

namespace {

struct Image {
    uint32_t height;
    uint32_t width;
    vec2 const* pixels;
    vec2 operator()(uint32_t x, uint32_t y) const {
        return (x >= width || y >= height) ? vec2(0) : pixels[x + width * y];
    }
};

struct PointCloud {
    uint32_t count;
    vec2* coords;
};

void advect_points(PointCloud pc, Image velocities, float step_size);

struct AdvectPoints : ClumpyCommand {
    AdvectPoints() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "create an animation by moving points according to a velocity field";
    }
    string usage() const override {
        return "<input_pts> <velocities_img> <step_size> <nframes> <suffix_img>";
    }
    string example() const override {
        return "coords.npy speeds.npy 0.01 240 anim.npy";
    }
};

static ClumpyCommand::Register registrar("advect_points", [] {
    return new AdvectPoints();
});

bool AdvectPoints::exec(vector<string> vargs) {
    if (vargs.size() != 5) {
        fmt::print("This command takes 5 arguments.\n");
        return false;
    }

    const string input_pts = vargs[0];
    const string velocities_img = vargs[1];
    const float step_size = atof(vargs[2].c_str());
    const uint32_t nframes = atoi(vargs[3].c_str());
    const string suffix_img = vargs[4];

    cnpy::NpyArray arr = cnpy::npy_load(input_pts);
    if (arr.shape.size() != 2 || arr.shape[1] != 2) {
        fmt::print("Input points have wrong shape.\n");
        return false;
    }
    if (arr.word_size != sizeof(float) || arr.type_code != 'f') {
        fmt::print("Input points has wrong data type.\n");
        return false;
    }
    PointCloud pc {
        .count = (uint32_t) arr.shape[0],
        .coords = arr.data<vec2>()
    };

    cnpy::NpyArray img = cnpy::npy_load(velocities_img);
    if (img.shape.size() != 3 || img.shape[2] != 2) {
        fmt::print("Velocities have wrong shape.\n");
        return false;
    }
    if (img.word_size != sizeof(float) || img.type_code != 'f') {
        fmt::print("Velocities have wrong data type.\n");
        return false;
    }
    Image vel {
        .width = (uint32_t) img.shape[1],
        .height = (uint32_t) img.shape[0],
        .pixels = img.data<vec2>()
    };

    for (uint32_t frame = 0; frame < nframes; ++frame) {
        advect_points(pc, vel, step_size);
    }

    return true;
}

void advect_points(PointCloud pc, Image velocities, float step_size) {

}

} // anonymous namespace
