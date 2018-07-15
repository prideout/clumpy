#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec3.hpp>
#include <glm/ext.hpp>

using namespace glm;
using std::vector;
using std::string;

struct VisualizeSdf : ClumpyCommand {
    VisualizeSdf() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "replace non-positive values with color";
    }
    string usage() const override {
        return "<input_img> <mode> <output_img>";
    }
    string example() const override {
        return "in.npy rgb out.npy";
    }
};

static ClumpyCommand::Register registrar("visualize_sdf", [] {
    return new VisualizeSdf();
});

bool VisualizeSdf::exec(vector<string> vargs) {
    if (vargs.size() != 3) {
        fmt::print("This command takes 3 arguments.\n");
        return false;
    }
    const string input_file = vargs[0];
    const string mode = vargs[1];
    const string output_file = vargs[2];

    cnpy::NpyArray arr = cnpy::npy_load(input_file);
    if (arr.shape.size() != 2) {
        fmt::print("Input data has wrong shape.\n");
        return false;
    }
    if (arr.word_size != sizeof(float) || arr.type_code != 'f') {
        fmt::print("Input data has wrong data type.\n");
        return false;
    }
    uint32_t height = arr.shape[0];
    uint32_t width = arr.shape[1];

    uint32_t ncomps;
    if (mode == "rgb") {
        ncomps = 3;
    } else if (mode == "rgba") {
        ncomps = 4;
    } else {
        fmt::print("mode must be 'rgb' or 'rgba'.\n");
        return false;
    }

    float const* psrc = arr.data<float>();
    vector<uint8_t> result(width * height * ncomps);

    if (ncomps == 3) {
        u8vec3* dst = (u8vec3*) result.data();
        const vec3 fill_color(0.2, 0.6, 0.8);
        const vec3 border_color(0.8, 0.6, 0.2);
        for (uint32_t row = 0; row < height; ++row) {
            for (uint32_t col = 0; col < width; ++col, ++dst) {
                float p = psrc[col + row * width];
                vec3 color(clamp(p, 0.0f, 1.0f));
                color = mix(fill_color, color, smoothstep(-0.001f, +0.001f, p));
                color = mix(border_color, color, smoothstep(0.0f, +0.01f, abs(p)));
                *dst = color * 255.0f;
            }
        }
    } else {
        u8vec4* dst = (u8vec4*) result.data();
        // const vec4 fill_color(0.2, 0.6, 0.8, 1.0);
        // const vec4 border_color(0.8, 0.6, 0.2, 1.0);
        const vec4 fill_color(0.6, 0.6, 0.6, 1.0);
        const vec4 border_color(0.2, 0.2, 0.2, 1.0);
        for (uint32_t row = 0; row < height; ++row) {
            for (uint32_t col = 0; col < width; ++col, ++dst) {
                float p = psrc[col + row * width];
                vec4 color = border_color;
                color.a = 0.0f;
                color = mix(fill_color, color, smoothstep(-0.001f, +0.001f, p));
                color = mix(border_color, color, smoothstep(0.0f, +0.01f, abs(p)));
                *dst = clamp(color * 255.0f, 0.0f, 255.0f);
            }
        }
    }

    cnpy::npy_save(output_file, result.data(), {height, width, ncomps}, "w");
    return true;
}
