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
    float const* pixels;
    float operator()(uint32_t x, uint32_t y) const {
        return (x >= width || y >= height) ? 0 : pixels[x + width * y];
    }
};

struct PointCloud {
    uint32_t count;
    vec2 const* coords;
};

vector<vec2> cull_points(PointCloud pc, Image img);

struct CullPoints : ClumpyCommand {
    CullPoints() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "remove points from areas with non-positive pixels";
    }
    string usage() const override {
        return "<input_pts> <mask_img> <output_pts>";
    }
    string example() const override {
        return "bridson.npy shapes.npy culled.npy";
    }
};

static ClumpyCommand::Register registrar("cull_points", [] {
    return new CullPoints();
});

bool CullPoints::exec(vector<string> vargs) {
    if (vargs.size() != 3) {
        fmt::print("This command takes 3 arguments.\n");
        return false;
    }
    const string input_pts = vargs[0];
    const string mask_img = vargs[1];
    const string output_pts = vargs[2];

    cnpy::NpyArray arr = cnpy::npy_load(input_pts);
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
    PointCloud pc {
        .count = (uint32_t) arr.shape[0],
        .coords = arr.data<vec2>()
    };

    cnpy::NpyArray img = cnpy::npy_load(mask_img);
    if (img.shape.size() != 2) {
        fmt::print("Input data has wrong shape.\n");
        return false;
    }
    if (img.word_size != sizeof(float) || img.type_code != 'f') {
        fmt::print("Input data has wrong data type.\n");
        return false;
    }
    Image sdf {
        .width = (uint32_t) img.shape[1],
        .height = (uint32_t) img.shape[0],
        .pixels = img.data<float>()
    };

    vector<vec2> result = cull_points(pc, sdf);
    cnpy::npy_save(output_pts, &result[0].x, {result.size(), 2}, "w");

    return true;
}

vector<vec2> cull_points(PointCloud pc, Image img) {
    vector<vec2> result;
    result.reserve(pc.count);
    uint32_t nremoved = 0;
    for (uint32_t i = 0; i < pc.count; ++i) {
        vec2 pt = pc.coords[i];
        if (img(pt.x, pt.y) > 0) {
            result.emplace_back(pt);
        } else {
            ++nremoved;
        }
    }
    fmt::print("Removed {} points.\n", nremoved);
    return result;
}

} // anonymous namespace
