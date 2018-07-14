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

void splat_points(vec2 const* ptlist, uint32_t npts, u32vec2 dims, uint8_t* dstimg);

namespace {

enum KernelType { DISTANCE, GAUSSIAN, CIRCLE };

struct SplatPoints : ClumpyCommand {
    SplatPoints() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "consume a list of 2-tuples and create an image";
    }
    string usage() const override {
        return "<input_pts> <dims> <kernel_type> <kernel_size> <alpha> <output_img>";
    }
    string example() const override {
        return "bridson.npy 500x250 gaussian 5 1.0 splat.npy";
    }
};

static ClumpyCommand::Register registrar("splat_points", [] {
    return new SplatPoints();
});

bool SplatPoints::exec(vector<string> vargs) {
    if (vargs.size() != 6) {
        fmt::print("The command takes 6 arguments.\n");
        return false;
    }
    const string pts_file = vargs[0];
    const string dims = vargs[1];
    const string kernel_typestring = vargs[2];
    const int kernel_size = atoi(vargs[3].c_str());
    //const float alpha = atof(vargs[4].c_str());
    const string output_file = vargs[5];
    const uint32_t width = atoi(dims.c_str());
    const uint32_t height = atoi(dims.substr(dims.find('x') + 1).c_str());

    if (0 == (kernel_size % 2)) {
        fmt::print("Kernel size must be an odd integer.\n");
        return false;
    }

    KernelType kernel_type;
    if (kernel_typestring == "distance") {
        kernel_type = DISTANCE;
    } else if (kernel_typestring == "gaussian") {
        kernel_type = GAUSSIAN;
    } else if (kernel_typestring == "circle") {
        kernel_type = CIRCLE;
    } else {
        fmt::print("Kernel type must be distance/gaussian/circle.\n");
        return false;
    }

    cnpy::NpyArray arr = cnpy::npy_load(pts_file);
    if (arr.shape.size() != 2) {
        fmt::print("Input data has wrong shape.\n");
        return false;
    }
    if (arr.shape[1] != 2) {
        fmt::print("Input points must be 2D.\n");
        return false;
    }
    if (arr.word_size != sizeof(float) || arr.type_code != 'f') {
        fmt::print("Input data has wrong data type.\n");
        return false;
    }
    uint32_t npts = arr.shape[0];
    vec2 const* ptlist = arr.data<vec2>();

    vector<uint8_t> dstimg;
    dstimg.resize(width * height);
    fmt::print("Drawing {} points.\n", npts);
    splat_points(ptlist, npts, u32vec2(width, height), dstimg.data());
    cnpy::npy_save(output_file, dstimg.data(), {height, width}, "w");

    return true;
}

} // anonymous namespace

void splat_points(vec2 const* ptlist, uint32_t npts, u32vec2 size, uint8_t* dstimg) {
    for (uint32_t i = 0; i < npts; ++i) {
        uint32_t x = ptlist[i].x;
        uint32_t y = ptlist[i].y;
        if (x < size.x && y < size.y) {
            dstimg[size.x * y + x] = 255;
        }
    }
}
