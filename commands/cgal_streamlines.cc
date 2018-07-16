#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec2.hpp>
#include <glm/ext.hpp>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Stream_lines_2.h>
#include <CGAL/Runge_kutta_integrator_2.h>
#include <CGAL/Regular_grid_2.h>
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Regular_grid_2<K> Field;
typedef CGAL::Runge_kutta_integrator_2<Field> Runge_kutta_integrator;
typedef CGAL::Stream_lines_2<Field, Runge_kutta_integrator> Strl;
typedef Strl::Point_iterator_2 Point_iterator;
typedef Strl::Stream_line_iterator_2 Strl_iterator;
typedef Strl::Point_2 Point_2;
typedef Strl::Vector_2 Vector_2;

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

struct CgalStreamlines : ClumpyCommand {
    CgalStreamlines() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "create a streamline diagram using CGAL";
    }
    string usage() const override {
        return "<velocities_img> <??> <??> <output_img>";
    }
    string example() const override {
        return "speeds.npy ?? ?? streamlines.npy";
    }
};

static ClumpyCommand::Register registrar("cgal_streamlines", [] {
    return new CgalStreamlines();
});

bool CgalStreamlines::exec(vector<string> vargs) {
    if (vargs.size() != 4) {
        fmt::print("This command takes 4 arguments.\n");
        return false;
    }

    const string input_pts = vargs[0];
    const string velocities_img = vargs[1];
    const string output_img = vargs[3];

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

    // splat_disks(points.data(), points.size(), dims, dstimg.data(), alpha, kernel_size);
    return true;
}

} // anonymous namespace
