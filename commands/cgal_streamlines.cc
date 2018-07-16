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

namespace {

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

    const string velocities_img = vargs[0];
    const string output_img = vargs[3];

    cnpy::NpyArray img = cnpy::npy_load(velocities_img);
    if (img.shape.size() != 3 || img.shape[2] != 2) {
        fmt::print("Velocities have wrong shape.\n");
        return false;
    }
    if (img.word_size != sizeof(float) || img.type_code != 'f') {
        fmt::print("Velocities have wrong data type.\n");
        return false;
    }
    const uint32_t width = (uint32_t) img.shape[1];
    const uint32_t height = (uint32_t) img.shape[0];

    const u32vec2 imagesize(width, height);
    const u32vec2 gridsize = imagesize / u32vec2(4);

    fmt::print("Populating CGAL field...\n");
    Field field(gridsize.x, gridsize.y, imagesize.x, imagesize.y);
    float const* velocityData = img.data<float>();
    for (uint32_t j = 0; j < gridsize.y; j++) {
        for (uint32_t i = 0; i < gridsize.x; i++) {
            float vx = *velocityData++;
            float vy = *velocityData++;
            field.set_field(i, j, Vector_2(vx, vy));
            velocityData += 2;
            velocityData += 2;
            velocityData += 2;
        }
        velocityData += 2 * imagesize.x * 3;
    }

    fmt::print("Generating streamlines...\n");
    double dSep = 3.5 * 8;
    double dRat = 1.6 * 1;
    Runge_kutta_integrator runge_kutta_integrator;
    Strl slines(field, runge_kutta_integrator, dSep, dRat);
    uint32_t nlines = 0;
    uint32_t npts = 0;
    for (auto sit = slines.begin(); sit != slines.end(); sit++) {
        for (Point_iterator pit = sit->first; pit != sit->second; pit++){
            npts++;
        }
        nlines++;
    }
    fmt::print("{} streamlines, {} points.\n", nlines, npts);

    fmt::print("Plotting points...\n");
    vector<uint8_t> dstimg(width * height);
    auto pixels = dstimg.data();
    auto set_pixel = [width, height, pixels](uint32_t x, uint32_t y) {
        if (x <  width && y < height) {
            pixels[x + width * y] = 255;
        }
    };
    for (auto sit = slines.begin(); sit != slines.end(); sit++) {
        int i = 0;
        for (Point_iterator pit = sit->first; pit != sit->second; pit++){
            Point_2 p = *pit;
            for (int dx = -5 ; dx <= 5; ++dx)
            for (int dy = -5 ; dy <= 5; ++dy)
            set_pixel(p.x() + dx, p.y() + dy);
        }
    }

    cnpy::npy_save(output_img, dstimg.data(), {imagesize.y, imagesize.x}, "w");
    return true;
}

} // anonymous namespace
