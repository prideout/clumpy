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

void splat_disks(vec2 const* ptlist, uint32_t npts, u32vec2 dims, uint8_t* dstimg, float alpha,
        int kernel_size);

namespace {

struct CgalStreamlines : ClumpyCommand {
    CgalStreamlines() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "create a streamline diagram using CGAL";
    }
    string usage() const override {
        return "<velocities_img> <line_width> <separating_distance> <saturation_ratio> "
                "<arrow_type> <output_img>";
    }
    string example() const override {
        return "speeds.npy 5 25 2 0 streamlines.npy";
    }
};

static ClumpyCommand::Register registrar("cgal_streamlines", [] {
    return new CgalStreamlines();
});

bool CgalStreamlines::exec(vector<string> vargs) {
    if (vargs.size() != 6) {
        fmt::print("This command takes 6 arguments.\n");
        return false;
    }

    const string velocities_img = vargs[0];
    const int line_width = atoi(vargs[1].c_str());
    const float separating_distance = atof(vargs[2].c_str());
    const float saturation_ratio = atof(vargs[3].c_str());
    const int arrow_type = atoi(vargs[4].c_str());
    const string output_img = vargs[5];

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
    u32vec2 gridsize = imagesize / u32vec2(4);

    fmt::print("Populating CGAL field...\n");
    Field field(gridsize.x, gridsize.y, imagesize.x, imagesize.y);
    float const* velocityData = img.data<float>();
    for (uint32_t j = 0; j < gridsize.y; j++) {
        for (uint32_t i = 0; i < gridsize.x; i++) {
            float vx = *velocityData++;
            float vy = *velocityData++;
            // CGAL seems to render streamlines backwards from advection, so here
            // we negate the velocity.
            field.set_field(i, j, Vector_2(-vx, -vy));
            // Skip 3 columns.
            velocityData += 2;
            velocityData += 2;
            velocityData += 2;
        }
        // Skip 3 rows.
        velocityData += 2 * imagesize.x * 3;
    }

    fmt::print("Generating streamlines...\n");
    Runge_kutta_integrator runge_kutta_integrator;
    Strl slines(field, runge_kutta_integrator, separating_distance, saturation_ratio);
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
    vector<vec2> pts;
    int kernel_size;

    // Fast path: no arrows.
    if (arrow_type == 0) {
        kernel_size = line_width;
        pts.reserve(npts);
        for (auto sit = slines.begin(); sit != slines.end(); sit++) {
            for (Point_iterator pit = sit->first; pit != sit->second; ++pit) {
                Point_2 p = *pit;
                pts.emplace_back(vec2 {p.x(), p.y()});
            }
        }
    } else {
        kernel_size = 5;
        pts.reserve(npts * line_width);
        for (auto sit = slines.begin(); sit != slines.end(); sit++) {

            // Count the number of points in this streamline and find the max speed.
            int npts = 0;
            for (Point_iterator pit = sit->first; pit != sit->second; pit++){
                npts++;
            }

            // Compute various properties of this arrow.
            const int arrowhead_length = line_width * 3 / 2;
            const int padding = arrowhead_length;
            int ideal_length_with_padding = arrowhead_length * 6;
            const int ndivisions = (npts + ideal_length_with_padding - 1) /
                    ideal_length_with_padding;
            ideal_length_with_padding = npts / ndivisions;

            // Skip short streamlines.
            if (npts < arrowhead_length * 3) {
                continue;
            }

            // Skip streamlines that are near boundaries.

            // Plot a series of perpendicular lines for each streamline.
            int ptindex = 0;
            for (Point_iterator pit = sit->first; pit != sit->second; ++pit, ++ptindex) {

                int tail = ideal_length_with_padding * (ptindex / ideal_length_with_padding);
                int head = min(tail + ideal_length_with_padding, npts) - 1;

                float thickness;
                if (ptindex < tail + padding) {
                    continue;
                }
                if (ptindex < head - arrowhead_length) {
                    thickness = float(line_width);
                } else {
                    float arrowhead_progress = float(head - ptindex) / arrowhead_length;
                    thickness = 3 * float(line_width) * arrowhead_progress;
                }
                vec2 p {pit->x(), pit->y()};
                vec2 a, b;
                if (ptindex == 0) {
                    a = p;
                    ++pit;
                    b = {pit->x(), pit->y()};
                    --pit;
                } else {
                    --pit;
                    a = {pit->x(), pit->y()};
                    ++pit;
                    b = p;
                }
                vec2 d = normalize(b - a);
                vec2 perp {-d.y, d.x};
                a = p - 0.5f * thickness * perp;
                b = p + 0.5f * thickness * perp;
                vec2 dp = (b - a) / float(line_width);
                p = a;
                for (int i = 0; i < line_width; i++) {
                    pts.emplace_back(p);
                    p += dp;
                }
            }
        }
    }

    splat_disks(pts.data(), pts.size(), imagesize, dstimg.data(), 1.0f, kernel_size);
    cnpy::npy_save(output_img, dstimg.data(), {imagesize.y, imagesize.x}, "w");
    return true;
}

} // anonymous namespace
