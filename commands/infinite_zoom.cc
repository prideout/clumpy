#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec2.hpp>
#include <glm/ext.hpp>

#include <random>

using namespace glm;
using namespace std;

namespace {

static constexpr int NoiseTableSize = 256;
static const int NoiseTableMask = NoiseTableSize - 1;
static int NoiseTablePerm[NoiseTableSize];
static vec2 NoiseTableGrad[NoiseTableSize];

void initnoise(uint32_t seed) {
    mt19937 randomGenerator(seed);
    iota(NoiseTablePerm, NoiseTablePerm + NoiseTableSize, 0);
    shuffle(NoiseTablePerm, NoiseTablePerm + NoiseTableSize, randomGenerator);
    for (int index = 0; index < NoiseTableMask; ++index) {
        const float theta = 2.0f * pi<float>() * index / NoiseTableMask;
        NoiseTableGrad[index] = {cosf(theta), sinf(theta)};
    }
}

vec2 noisegrad(i32vec2 v) {
    int i = v.x, j = v.y;
    int hash = NoiseTablePerm[ ( NoiseTablePerm[ i & NoiseTableMask ] + j ) & NoiseTableMask ];
    return NoiseTableGrad[hash];
}

vec4 vcolor(uint32_t hex) {
    return {hex >> 16, (hex >> 8) & 0xff, hex & 0xff, 0xff};
}

vec3 gradnoise(vec2 p);
float gridlines(vec2 p);

struct PixelRay {
    u16vec2 origin;
    u16vec2 target;
};

float falloff(float t) {
    return 1.0f - (3.0f - 2.0f * abs(t)) * t * t;
}

struct InfiniteZoom : ClumpyCommand {
    InfiniteZoom() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "generate infinite zoom animation";
    }
    string usage() const override {
        return "<dims> <nframes> <seed>";
    }
    string example() const override {
        return "512x512 240 0";
    }
};

ClumpyCommand::Register registrar("infinite_zoom", [] {
    return new InfiniteZoom();
});

bool InfiniteZoom::exec(vector<string> vargs) {
    if (vargs.size() != 3) {
        fmt::print("The command takes 3 arguments.\n");
        return false;
    }
    string dims = vargs[0];
    const uint32_t width = atoi(dims.c_str());
    const uint32_t height = atoi(dims.substr(dims.find('x') + 1).c_str());
    const uint32_t nframes = atoi(vargs[1].c_str());
    const uint32_t seed = atoi(vargs[2].c_str());
    const string output_suffix = fmt::format("island{:02}.npy", seed);

    // Initialize the table of gradient vectors.
    initnoise(seed);

    // "Raster Space" is <ui16,ui16> with 0,0 at upper left.
    // "Viewport Space" is <fp32,fp32> with -1,+1 at upper left.
    // -1.0 is the left  edge of pixel (0)
    // +1.0 is the right edge of pixel (w-1)

    const float dx = 2.0f / width;
    const float sx = -1.0f + dx * 0.5;
    const float dy = 2.0f / height;
    const float sy = 1.0f - dy * 0.5;

    vector<uint32_t> result(width * height);
    u8vec4* pdata = (u8vec4*) result.data();
    for (int row = 0; row < height; ++row) {
        const float y = sy - row * dy;
        for (int col = 0; col < width; ++col, ++pdata) {
            const float x = sx + col * dx;
            const vec2 p(x, y);

            float freq = 4;
            float ampl = 0.7;
            float N = 0;
            float G = 0;
            N += ampl * gradnoise(p * freq).x;
            G += ampl * gridlines(p * freq);
            freq *= 2; ampl /= 2;
            N += ampl * gradnoise(p * freq).x;
            G += ampl * gridlines(p * freq);
            freq *= 2; ampl /= 2;
            N += ampl * gradnoise(p * freq).x;
            G += ampl * gridlines(p * freq);
            freq *= 2; ampl /= 2;
            N += ampl * gradnoise(p * freq).x;
            G += ampl * gridlines(p * freq);
            freq *= 2; ampl /= 2;
            N += ampl * gradnoise(p * freq).x;
            G += ampl * gridlines(p * freq);

            const float F = falloff(x) * falloff(y);
            const float H = F + N - 0.5f;
            const float B = smoothstep(0.02f, 0.0f, abs(H));
            vec4 color = H > 0 ? vcolor(0x547238) : vcolor(0x365978);
            color = mix(color, vcolor(0xb8c289), B);
            const vec4 gridcolor = H > 0 ? vcolor(0x000000) : vcolor(0xffffff);
            color = mix(color, gridcolor, G * 0.25);

            *pdata = u8vec4(color);
        }
    }

    cnpy::npy_save(output_suffix, result.data(), {height, width}, "w");
    fmt::print("\nWrote {:03}{} through {:03}{}.\n", 0, output_suffix, nframes - 1, output_suffix);
    return true;
}

// Returns gradient noise in .x and its derivatives in .yz
// The range is well inside [-1,+1], approx [-0.7,+0.7].
vec3 gradnoise(vec2 p) {
    i32vec2 i(floor(p));
    vec2 f(fract(p));

    // Quintic interpolation.
    vec2 u = f*f*f*(f*(f*6.0f-15.0f)+10.0f);

    vec2 ga = noisegrad( i + i32vec2(0,0) );
    vec2 gb = noisegrad( i + i32vec2(1,0) );
    vec2 gc = noisegrad( i + i32vec2(0,1) );
    vec2 gd = noisegrad( i + i32vec2(1,1) );

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

float gridlines(vec2 p) {
    return 0;
    // vec2 f = abs(vec2(0.5) - fract(p));
    // float m = std::max(f.x, f.y);
    // return smoothstep(0.45f, 0.5f, m);
}

}
