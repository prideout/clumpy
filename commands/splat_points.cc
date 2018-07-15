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
void splat_disks(vec2 const* ptlist, uint32_t npts, u32vec2 dims, uint8_t* dstimg, float alpha,
        int kernel_size);

namespace {

enum KernelType { u8disk, fp32sphere };

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
        return "bridson.npy 500x250 u8disk 5 1.0 splat.npy";
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
    const float alpha = atof(vargs[4].c_str());
    const string output_file = vargs[5];
    const uint32_t width = atoi(dims.c_str());
    const uint32_t height = atoi(dims.substr(dims.find('x') + 1).c_str());

    if (0 == (kernel_size % 2)) {
        fmt::print("Kernel size must be an odd integer.\n");
        return false;
    }

    KernelType kernel_type;
    if (kernel_typestring == "u8disk") {
        kernel_type = u8disk;
    } else if (kernel_typestring == "fp32sphere") {
        kernel_type = fp32sphere;
    } else {
        fmt::print("Kernel type must be u8disk/fp32sphere.\n");
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
    const u32vec2 size(width, height);
    if (alpha == 1 && kernel_size == 1) {
        splat_points(ptlist, npts, size, dstimg.data());
    } else if (kernel_type == u8disk) {
        splat_disks(ptlist, npts, size, dstimg.data(), alpha, kernel_size);
    } else {
        fmt::print("Only disks are implemented right now.\n");
        return false;
    }
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

void splat_disks(vec2 const* ptlist, uint32_t npts, u32vec2 dims, uint8_t* dstimg, float alpha,
        int kernel_size) {

    // First, make the sprite.
    if (0 == (kernel_size % 2)) {
        fmt::print("Kernel size must be an odd integer.\n");
        exit(1);
    }
    vector<uint8_t> sprite(kernel_size * kernel_size);
    const int middle = kernel_size / 2;
    const int r2 = middle * middle;
    for (int i = 0; i < kernel_size; i++) {
        for (int j = 0; j < kernel_size; j++) {
            int x = i - middle;
            int y = j - middle;
            int d2 = x * x + y * y;
            if (d2 < r2) {
                sprite[i + j * kernel_size] = 255;
            }
        }
    }

    // Blend in the points with src-over blending.
    const int32_t width = (int32_t) dims.x;
    int32_t height = (int32_t) dims.y;
    const int32_t h = kernel_size / 2;
    for (uint32_t i = 0; i < npts; ++i) {
        int32_t x = (int32_t) ptlist[i].x;
        int32_t y = (int32_t) ptlist[i].y;
        for (int32_t x0 = x - h; x0 <= x + h; ++x0) {
        for (int32_t y0 = y - h; y0 <= y + h; ++y0) {
            if (x0 >= 0 && y0 >= 0 && x0 < width && y0 < height) {
                dstimg[width * y0 + x0] = (uint8_t) ((1.0f - alpha) * dstimg[width * y0 + x0] +
                    alpha * 255.0f);
            }
        }
        }
    }
}
