#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec2.hpp>
#include <glm/ext.hpp>

#include <random>
#include <csignal>

using namespace glm;
using namespace std;

namespace {

#define CHECK(c) check(c, __LINE__)

// Asserts a safety condition even in release builds.
void check(bool condition, int lineno) {
    if (!condition) {
        fmt::print(stderr, "GradientNoise failure on line {}.\n", lineno);
        raise(SIGINT);
    }
}

// Parses strings like "99x99"
u32vec2 to_ivec2(string dims) {
    CHECK(dims.size() >= 3);
    int x = atoi(dims.c_str());
    int y = atoi(dims.substr(dims.find('x') + 1).c_str());
    return {x, y};
}

// Parses strings like "1.0,2.0,3.0,4.0"
vec4 to_vec4(string arg) {
    CHECK(arg.size() >= 7);
    string tuple = arg;
    float x = atof(tuple.c_str());
    tuple = tuple.substr(tuple.find(',') + 1);
    float y = atof(tuple.c_str());
    tuple = tuple.substr(tuple.find(',') + 1);
    float z = atof(tuple.c_str());
    tuple = tuple.substr(tuple.find(',') + 1);
    float w = atof(tuple.c_str());
    return {x, y, z, w};
}

struct GradientNoise : ClumpyCommand {
    GradientNoise() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "generate gradient noise in [-1,+1]";
    }
    string usage() const override {
        return "<dims> <viewport> <frequency> <seed>";
    }
    string example() const override {
        return "1024x1024 '-1.0,-1.0,+1.0,+1.0' 3.0 42";
    }
};

static ClumpyCommand::Register registrar("gradient_noise", [] {
    return new GradientNoise();
});

constexpr int NoiseTableSize = 256;
constexpr int NoiseTableMask = NoiseTableSize - 1;
int NoiseTablePerm[NoiseTableSize];
vec2 NoiseTableGrad[NoiseTableSize];

void initnoise(uint32_t seed) {
    mt19937 randomGenerator(seed);
    iota(NoiseTablePerm, NoiseTablePerm + NoiseTableSize, 0);
    shuffle(NoiseTablePerm, NoiseTablePerm + NoiseTableSize, randomGenerator);
    for (int index = 0; index < NoiseTableMask; ++index) {
        float theta = 2.0f * pi<float>() * index / NoiseTableMask;
        NoiseTableGrad[index] = {cosf(theta), sinf(theta)};
    }
}

vec2 noisegrad(i32vec2 v) {
    int i = v.x, j = v.y;
    int hash = NoiseTablePerm[ ( NoiseTablePerm[ i & NoiseTableMask ] + j ) & NoiseTableMask ];
    return NoiseTableGrad[hash];
}

// Returns gradient noise in .x and its derivatives in .yz
// The range is well inside [-1,+1], approx [-0.7,+0.7].
vec3 gradnoise(vec2 p, int32_t seed) {
    i32vec2 i(floor(p));
    vec2 f(fract(p));

    // Quintic interpolation.
    vec2 u = f*f*f*(f*(f*6.0f-15.0f)+10.0f);

    vec2 ga = noisegrad( i + seed + i32vec2(0,0) );
    vec2 gb = noisegrad( i + seed + i32vec2(1,0) );
    vec2 gc = noisegrad( i + seed + i32vec2(0,1) );
    vec2 gd = noisegrad( i + seed + i32vec2(1,1) );

    float va = dot( ga, f - vec2(0,0) );
    float vb = dot( gb, f - vec2(1,0) );
    float vc = dot( gc, f - vec2(0,1) );
    float vd = dot( gd, f - vec2(1,1) );

    // Resulting scalar noise.
    float v = va + u.x*(vb-va) + u.y*(vc-va) + u.x*u.y*(va-vb-vc+vd);

    // Analytic derivatives.
    vec2 du = 30.0f*f*f*(f*(f-2.0f)+1.0f);
    vec2 derivatives(ga + u.x*(gb-ga) + u.y*(gc-ga) + u.x*u.y*(ga-gb-gc+gd) +
            du * (vec2(u.y, u.x)*(va-vb-vc+vd) + vec2(vb,vc) - va));

    return vec3(v, derivatives);
}

bool GradientNoise::exec(vector<string> vargs) {
    if (vargs.size() != 4) {
        fmt::print("The command takes 4 arguments.\n");
        return false;
    }
    const u32vec2 dims = to_ivec2(vargs[0]);
    const vec4 viewport = to_vec4(vargs[1]);
    const float frequency = atof(vargs[2].c_str());
    const int seed = atoi(vargs[3].c_str());
    const string output_file = "gradient_noise.npy";
    fmt::print("gradient_noise {}x{} [{},{},{},{}] {} {}\n",
        dims.x, dims.y, viewport.x, viewport.y, viewport.z, viewport.w, frequency, seed);

    // Initialize the table of gradient vectors.
    initnoise(seed);

    // "Raster Space" is <ui16,ui16> with 0,0 at upper left.
    // Viewport Space is <fp32,fp32> with -,+ at upper left.
    // If the viewport is -1,-1 through +1,+1, then:
    //     -1.0 is the left  edge of pixel (0)
    //     +1.0 is the right edge of pixel (w-1)
    //     Freq=1 is a 2x2 grid of surflets

    const float vpwidth = viewport.z - viewport.x;
    const float vpheight = viewport.w - viewport.y;
    const float dx = vpwidth / dims.x;
    const float sx = viewport.x + dx * 0.5;
    const float dy = vpheight / dims.y;
    const float sy = viewport.w - dy * 0.5;

    auto add_octave = [&](vector<float>& grid, float freq, int seed) {
        float* fdata = grid.data();
        for (int row = 0; row < dims.y; ++row) {
            const float y = sy - row * dy;
            for (int col = 0; col < dims.x; ++col, ++fdata) {
                const float x = sx + col * dx;
                const vec2 p(x, y);
                *fdata += gradnoise(p * freq, seed).x;
            }
        }
    };

    vector<float> result(dims.x * dims.y);
    add_octave(result, frequency, seed);

    cnpy::npy_save(output_file, result.data(), {dims.y, dims.x}, "w");
    fmt::print("Wrote {}\n", output_file);
    return true;
}

} // anonymous namespace
