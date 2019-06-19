#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec3.hpp>
#include <glm/ext.hpp>

#include <blend2d.h>

#include <limits>
#include <random>

using namespace glm;

using std::vector;
using std::string;
using std::numeric_limits;

void generate_pts(float width, float height, float radius, int seed, vector<float>& result);

namespace {

struct PendulumRender : ClumpyCommand {
    PendulumRender() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "";
    }
    string usage() const override {
        return "<image_type> <dims> <friction> <scale_x> <scale_y> <output_img>";
    }
    string example() const override {
        return "500x500 0.01 1.0 5.0 field.npy";
    }
    void drawLeftPanel(BLImage* dst);
    void drawRightPanel(BLImage* dst);
    float friction;
    float scalex;
    float scaley;
};

static ClumpyCommand::Register registrar("pendulum_render", [] {
    return new PendulumRender();
});

void PendulumRender::drawLeftPanel(BLImage* img) {
    BLContext ctx(*img);

    // https://blend2d.com/doc/getting-started.html
    // if blend2d doesn't work out...
    //     https://github.com/prideout/skia
    //     https://skia.org/user/api/skcanvas_creation

    ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.fillAll();

    ctx.setCompOp(BL_COMP_OP_SRC_OVER);

    double cx = 180, cy = 180, r = 30;

    ctx.setStrokeWidth(10);
    ctx.setStrokeStartCap(BL_STROKE_CAP_ROUND);
    ctx.setStrokeEndCap(BL_STROKE_CAP_BUTT);
    ctx.setStrokeStyle(BLRgba32(0xff7f7f7f));

    BLPath path;
    path.moveTo(img->width() / 2, img->height() / 2);
    path.lineTo(cx, cy);
    ctx.strokePath(path);

    BLGradient radial(BLRadialGradientValues(cx + r/3, cy + r/3, cx + r/3, cy + r/3, r * 1.75));
    radial.addStop(0.0, BLRgba32(0xff5cabff));
    radial.addStop(1.0, BLRgba32(0xff000000));
    ctx.setFillStyle(radial);
    ctx.fillCircle(cx, cy, r);
    ctx.setFillStyle(BLRgba32(0xFFFFFFFF));

    BLFontFace face;
    BLResult err = face.createFromFile("extras/NotoSans-Regular.ttf");
    if (err) {
        fmt::print("Failed to load a font-face: {}\n", err);
        return;
    }

    BLFont font;
    font.createFromFace(face, 40.0f);

    ctx.setFillStyle(BLRgba32(0xffffffff));
    ctx.fillUtf8Text(BLPoint(60, 80), font, "θ'' = -μθ' - g / L * sin(θ)");

    ctx.setStrokeStyle(BLRgba32(0xf0000000));
    ctx.setStrokeWidth(2);
    ctx.strokeUtf8Text(BLPoint(60, 80), font, "θ'' = -μθ' - g / L * sin(θ)");

    ctx.end();
}

void PendulumRender::drawRightPanel(BLImage* img) {
    const int nframes = 200;
    const float mindist = 20;
    const float step_size = 2.5;
    const float decay = 0.99;
    const float width = img->width();
    const float height = img->height();
    const float g = 1, L = 1;

    auto getFieldVector = [=](vec2 v) {
        const float x = scalex * (v.x / width - 0.5);
        const float y = scaley * (v.y / height - 0.5);
        const float theta = x;
        const float omega = y;
        const float omega_dot = -friction * omega - g / L * sin(theta);
        return vec2(omega, omega_dot);
    };

    auto show_progress = [](uint32_t i, uint32_t count) {
        int progress = 100 * i / (count - 1);
        fmt::print("[");
        for (int c = 0; c < 100; ++c) putc(c < progress ? '=' : '-', stdout);
        fmt::print("]\r");
        fflush(stdout);
    };

    vector<float> points2d;
    generate_pts(img->width(), img->height(), mindist, 4421, points2d);
    const size_t npts = points2d.size() / 2;

    vector<uint32_t> age_offset(npts);
    {
        std::mt19937 generator;
        generator.seed(0);
        std::uniform_int_distribution<> get_age_offset(0, nframes - 1);
        for (uint32_t i = 0; i < npts; ++i) {
            age_offset[i] = get_age_offset(generator);
        }
    }

    vector<float> particle_age(npts);
    vector<vec2> advected_points(npts);
    for (uint32_t i = 0; i < npts; ++i) {
        advected_points[i].x = points2d[i * 2];
        advected_points[i].y = points2d[i * 2 + 1];
    }

    uint32_t simframe = 0;

    // Initial advection. (Does not get recorded.)
    for (; simframe < nframes; ++simframe) {
        for (uint32_t i = 0; i < npts; ++i) {
            vec2& pt = advected_points[i];
            pt += step_size * getFieldVector(pt);
            particle_age[i]++;
            if (simframe >= age_offset[i]) {
                pt.x = points2d[i * 2];
                pt.y = points2d[i * 2 + 1];
                particle_age[i] = 0;
                age_offset[i] = nframes;
            }
        }
    }

    BLPath axis;
    axis.moveTo(0, img->height() / 2);
    axis.lineTo(img->width(), img->height() / 2);
    axis.moveTo(img->width() / 2, 0);
    axis.lineTo(img->width() / 2, img->height());

    BLContext ctx(*img);

    // Background
    ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.setFillStyle(BLRgba32(0xffffffff));
    ctx.fillAll();
    ctx.setCompOp(BL_COMP_OP_SRC_OVER);

    // Axis Lines
    ctx.setStrokeStyle(BLRgba32(0xff000000));
    ctx.setStrokeWidth(3);
    ctx.strokePath(axis);
    ctx.setStrokeStartCap(BL_STROKE_CAP_ROUND);
    ctx.setStrokeEndCap(BL_STROKE_CAP_ROUND);

    // Stream Lines
    ctx.setStrokeStyle(BLRgba32(0x40000000));
    ctx.setStrokeWidth(2);

    for (uint32_t i = 0; i < npts; ++i) {
        show_progress(i, npts);
        BLPath streamlines;

        vec2& pt = advected_points[i];
        streamlines.moveTo(pt.x, pt.y);

        for (simframe = nframes; simframe < nframes * 2; ++simframe) {
            pt += step_size * getFieldVector(pt);
            streamlines.lineTo(pt.x, pt.y);
            particle_age[i]++;
            if (particle_age[i] >= nframes) {
                pt.x = points2d[i * 2];
                pt.y = points2d[i * 2 + 1];
                particle_age[i] = 0;
                streamlines.moveTo(pt.x, pt.y);
            }
        }
        ctx.strokePath(streamlines);
    }
    puts("");

    ctx.end();
}

bool PendulumRender::exec(vector<string> vargs) {
    if (vargs.size() != 5) {
        fmt::print("The command takes 5 arguments.\n");
        return false;
    }
    const string dims = vargs[0];
    friction = atof(vargs[1].c_str());
    scalex = atof(vargs[2].c_str());
    scaley = atof(vargs[3].c_str());
    const string output_file = vargs[4];
    const uint32_t output_width = atoi(dims.c_str());
    const uint32_t output_height = atoi(dims.substr(dims.find('x') + 1).c_str());

    BLImage limg(output_width, output_height, BL_FORMAT_PRGB32);
    drawLeftPanel(&limg);

    BLImage rimg(output_width, output_height, BL_FORMAT_PRGB32);
    drawRightPanel(&rimg);

    BLImageData limgdata;
    limg.getData(&limgdata);

    BLImageData rimgdata;
    rimg.getData(&rimgdata);

    const u8vec4* lsrcpixels = (const u8vec4*) limgdata.pixelData;
    const u8vec4* rsrcpixels = (const u8vec4*) rimgdata.pixelData;

    vector<vec4> dstimg(output_width * 2 * output_height);
    for (uint32_t row = 0; row < output_height; ++row) {
        for (uint32_t col = 0; col < output_width; ++col) {
            u8vec4 src = u8vec4(lsrcpixels[row * output_width + col]);
            dstimg[row * output_width * 2 + col] = vec4(src) / 255.0f;
        }
        for (uint32_t col = 0; col < output_width; ++col) {
            u8vec4 src = u8vec4(rsrcpixels[row * output_width + col]);
            dstimg[row * output_width * 2 + col + output_width] = vec4(src) / 255.0f;
        }
    }
    cnpy::npy_save(output_file, &dstimg.data()->x, {output_height, output_width * 2, 4}, "w");
    return true;
}

} // anonymous namespace
