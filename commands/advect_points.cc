#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec2.hpp>
#include <glm/ext.hpp>

using namespace glm;

using std::vector;
using std::string;

void splat_points(vec2 const* ptlist, uint32_t npts, u32vec2 size, uint8_t* dstimg);

namespace {

struct Image {
    uint32_t height;
    uint32_t width;
    vec2 const* pixels;
    vec2 texel_fetch(uint32_t x, uint32_t y) const {
        return (x >= width || y >= height) ? vec2(0) : pixels[x + width * y];
    }
    // TODO: bilinear interpolation with 3 mixes and 4 fetches. Might want to test it separately.
    vec2 sample(vec2 coord) {
        return texel_fetch(coord.x, coord.y);
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
    const string suffix = vargs[4];

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

    auto show_progress = [](uint32_t i, uint32_t count) {
        int progress = 100 * i / (count - 1);
        fmt::print("[");
        for (int c = 0; c < 100; ++c) putc(c < progress ? '=' : '-', stdout);
        fmt::print("]\r");
        fflush(stdout);
    };

    vector<uint8_t> dstimg;
    u32vec2 dims(vel.width, vel.height);
    dstimg.resize(vel.width * vel.height);
    for (uint32_t frame = 0; frame < nframes; ++frame) {
        show_progress(frame, nframes);
        advect_points(pc, vel, step_size);
        memset(dstimg.data(), 0, dstimg.size());
        splat_points(pc.coords, pc.count, dims, dstimg.data());
        string filename = fmt::format("{:03}{}", frame, suffix);
        cnpy::npy_save(filename, dstimg.data(), {dims.y, dims.x}, "w");
    }

    fmt::print("\nGenerated {:03}{} through {:03}{}.\n", 0, suffix, nframes - 1, suffix);
    return true;
}

void advect_points(PointCloud pc, Image velocities, float step_size) {
    for (uint32_t i = 0; i < pc.count; ++i) {
        vec2& pt = pc.coords[i];
        pt += step_size * velocities.sample(pt);
    }
}

} // anonymous namespace
