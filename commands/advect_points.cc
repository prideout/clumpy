#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec2.hpp>
#include <glm/ext.hpp>

#include <random>

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
        return "coords.npy speeds.npy 1.0 240 anim.npy";
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
    PointCloud original_points {
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
    Image velocities {
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

    vector<uint8_t> dstimg(velocities.width * velocities.height);

    const u32vec2 dims(velocities.width, velocities.height);

    const uint32_t npts = original_points.count;
    vector<float> particle_age(npts);
    vector<vec2> advected_points(original_points.coords, original_points.coords + npts);

    vector<uint32_t> age_offset(npts);
    {
        std::mt19937 generator;
        generator.seed(0);
        std::uniform_int_distribution<> get_age_offset(0, nframes - 1);
        for (uint32_t i = 0; i < npts; ++i) {
            age_offset[i] = get_age_offset(generator);
        }
    }

    uint32_t animframe = 0;
    uint32_t simframe = 0;
    for (; simframe < 2 * nframes; ++simframe) {
        show_progress(simframe, 2 * nframes);

        // Initial advection.
        if (simframe < nframes) {

            for (uint32_t i = 0; i < npts; ++i) {
                vec2& pt = advected_points[i];
                pt += step_size * velocities.sample(pt);
                particle_age[i]++;
                if (simframe >= age_offset[i]) {
                    pt = original_points.coords[i];
                    particle_age[i] = 0;
                    age_offset[i] = nframes;
                }
            }

        // Recorded
        } else {

            for (uint32_t i = 0; i < npts; ++i) {
                vec2& pt = advected_points[i];
                pt += step_size * velocities.sample(pt);
                particle_age[i]++;
                if (particle_age[i] >= nframes) {
                    pt = original_points.coords[i];
                    particle_age[i] = 0;
                }
            }

            memset(dstimg.data(), 0, dstimg.size());
            splat_points(advected_points.data(), npts, dims, dstimg.data());
            const string filename = fmt::format("{:03}{}", animframe++, suffix);
            cnpy::npy_save(filename, dstimg.data(), {dims.y, dims.x}, "w");
        }
    }

    fmt::print("\nGenerated {:03}{} through {:03}{}.\n", 0, suffix, animframe - 1, suffix);
    return true;
}

} // anonymous namespace
