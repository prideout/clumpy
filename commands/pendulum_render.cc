#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec3.hpp>
#include <glm/ext.hpp>

#include <blend2d.h>

#include <limits>

using namespace glm;

using std::vector;
using std::string;
using std::numeric_limits;

namespace {

struct PendulumRender : ClumpyCommand {
    PendulumRender() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "";
    }
    string usage() const override {
        return "<phase_npy> <output_dim> <output_img>";
    }
    string example() const override {
        return "phase.npy 500x500 render.npy";
    }
};

static ClumpyCommand::Register registrar("pendulum_render", [] {
    return new PendulumRender();
});

bool PendulumRender::exec(vector<string> vargs) {
    if (vargs.size() != 3) {
        fmt::print("The command takes 3 arguments.\n");
        return false;
    }
    const string phase_file = vargs[0];
    const string output_dims = vargs[1];
    const string output_file = vargs[2];
    const uint32_t output_width = atoi(output_dims.c_str());
    const uint32_t output_height = atoi(output_dims.substr(output_dims.find('x') + 1).c_str());

    BLImage img(output_width, output_height, BL_FORMAT_PRGB32);
    BLContext ctx(img);

    // https://blend2d.com/doc/getting-started.html
    // if blend2d doesn't work out...
    //     https://github.com/prideout/skia
    //     https://skia.org/user/api/skcanvas_creation

    ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.fillAll();

    BLPath path;
    path.moveTo(26, 31);
    path.cubicTo(642, 132, 587, -136, 25, 464);
    path.cubicTo(882, 404, 144, 267, 27, 31);

    ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    ctx.setFillStyle(BLRgba32(0xFFFFFFFF));
    ctx.fillPath(path);

    BLFontFace face;
    BLResult err = face.createFromFile("extras/NotoSans-Regular.ttf");
    if (err) {
        fmt::print("Failed to load a font-face: {}\n", err);
        return false;
    }

    BLFont font;
    font.createFromFace(face, 20.0f);

    BLFontMetrics fm = font.metrics();
    BLTextMetrics tm;
    BLGlyphBuffer gb;

    BLPoint p(20, 190 + fm.ascent);
    const char* text = "Hello Blend2D!\n"
                        "I'm a simple multiline text example\n"
                        "that uses BLGlyphBuffer and fillGlyphRun!";
    for (;;) {
        const char* end = strchr(text, '\n');
        gb.setUtf8Text(text, end ? (size_t)(end - text) : SIZE_MAX);
        font.shape(gb);
        font.getTextMetrics(gb, tm);

        p.x = (480.0 - (tm.boundingBox.x1 - tm.boundingBox.x0)) / 2.0;
        ctx.fillGlyphRun(p, font, gb.glyphRun());
        p.y += fm.ascent + fm.descent + fm.lineGap;

        if (!end) break;
        text = end + 1;
    }

    ctx.end();

    BLImageData imgdata;
    img.getData(&imgdata);
    const u8vec4* srcpixels = (const u8vec4*) imgdata.pixelData;

    vector<vec3> dstimg(output_width * output_height);
    for (uint32_t row = 0; row < output_height; ++row) {
        for (uint32_t col = 0; col < output_width; ++col) {
            u8vec3 src = u8vec3(srcpixels[row * output_width + col]);
            dstimg[row * output_width + col] = vec3(src) / 255.0f;
        }
    }
    cnpy::npy_save(output_file, &dstimg.data()->x, {output_height, output_width, 3}, "w");
    return true;
}

} // anonymous namespace
