#include "clumpy.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec2.hpp>
#include <glm/ext.hpp>

#include <random>

using namespace glm;

using std::vector;
using std::string;
using std::numeric_limits;

struct GenerateShapes : ClumpyCommand {
    GenerateShapes() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "generate signed distance field of random shapes";
    }
    string usage() const override {
        return "<dim> <nshapes> <seed> <output_file>";
    }
    string example() const override {
        return "400x200 4 26 out.npy";
    }
    inline float shade(double u, double v, double w, double h);
    int32_t nshapes;
    int32_t seed;
    std::mt19937 generator;
};

static ClumpyCommand::Register registrar("generate_dshapes", [] {
    return new GenerateShapes();
});

bool GenerateShapes::exec(vector<string> vargs) {
    if (vargs.size() != 4) {
        return false;
    }
    string dims = vargs[0];
    const uint32_t width = atoi(dims.c_str());
    const uint32_t height = atoi(dims.substr(dims.find('x') + 1).c_str());
    nshapes = atoi(vargs[1].c_str());
    seed = atoi(vargs[2].c_str());
    const string output_file = vargs[3].c_str();

    double dx, dy;
    double w, h;
    if (width > height) {
        dy = 1.0 / height;
        dx = dy;
        h = 1.0;
        w = dy * width;
    } else {
        dx = 1.0 / width;
        dy = dx;
        w = 1.0;
        h = dx * height;
    }

    float minval = numeric_limits<float>::max();
    float maxval = numeric_limits<float>::lowest();
    vector<float> result(width * height);
    float* pdata = result.data();
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col, ++pdata) {
            double u = dx * col;
            double v = dy * row;
            *pdata = shade(u, v, w - dx, h - dy);
            minval = std::min(minval, *pdata);
            maxval = std::max(maxval, *pdata);
        }
    }
    fmt::print("SDF range is {} to {}\n", minval, maxval);
    cnpy::npy_save(output_file, result.data(), {height, width}, "w");

    return true;
}

// "2d signed distance functions" inspired by Maarten's shadertoy:
//     https://www.shadertoy.com/view/4dfXDn

float smoothMerge(float d1, float d2, float k) {
    float h = clamp(0.5f + 0.5f*(d2 - d1)/k, 0.0f, 1.0f);
    return mix(d2, d1, h) - k * h * (1.0-h);
}

float merge(float d1, float d2) {
    return min(d1, d2);
}

float mergeExclude(float d1, float d2) {
    return min(max(-d1, d2), max(-d2, d1));
}

float substract(float d1, float d2) {
    return max(-d1, d2);
}

float intersect(float d1, float d2) {
    return max(d1, d2);
}

vec2 rotateCCW(vec2 p, float a) {
    mat2 m = mat2(cos(a), sin(a), -sin(a), cos(a));
    return p * m;	
}

vec2 rotateCW(vec2 p, float a) {
    mat2 m = mat2(cos(a), -sin(a), sin(a), cos(a));
    return p * m;
}

float circleDist(vec2 p, float radius) {
    return length(p) - radius;
}

float triangleDist(vec2 p, float radius) {
    return max(	abs(p).x * 0.866025f + 
                   p.y * 0.5f, -p.y) 
                -radius * 0.5f;
}

float boxDist(vec2 p, vec2 size, float radius) {
    size -= vec2(radius);
    vec2 d = abs(p) - size;
      return min(max(d.x, d.y), 0.0f) + length(max(d, 0.0f)) - radius;
}

float lineDist(vec2 p, vec2 start, vec2 end, float width) {
    vec2 dir = start - end;
    float lngth = length(dir);
    dir /= lngth;
    vec2 proj = max(0.0f, min(lngth, dot((start - p), dir))) * dir;
    return length( (start - p) - proj ) - (width / 2.0);
}

float GenerateShapes::shade(double u, double v, double w, double h) {
    vec2 p = vec2(u, v);
    float d = -boxDist(p - vec2(w / 2, h / 2), vec2(w / 2, h / 2), 0.0);

    // First, check if this is hardcoded scene (seed == 0).

    if (nshapes == 1 && seed == 0) {
        float c = circleDist(p - vec2(w / 3, h / 2), h / 4);
        return merge(d, c);
    }

    // Random shape generator.

    generator.seed(seed);
    std::uniform_int_distribution<> shape_rand(0, 3);
    std::uniform_real_distribution<> x_rand(0, w);
    std::uniform_real_distribution<> y_rand(0, h);
    std::uniform_real_distribution<> sz_rand(0.1, 0.3);
    std::uniform_real_distribution<> rot_rand(0, 2 * glm::pi<float>());

    for (int32_t i = 0; i < nshapes; ++i) {
        float e;
        int shape = shape_rand(generator);
        float randx = x_rand(generator);
        float randy = y_rand(generator);
        float randsz = sz_rand(generator);
        float randrot = rot_rand(generator);
        float linelen = 0.5 * randsz;
        float linewid = 0.05;
        float cornerrad = 0.05;
        vec2 linevec = vec2(cos(randrot), sin(randrot));
        switch (shape) {
            case 0:
                e = lineDist(p, vec2(randx, randy) - linelen * linevec,
                        vec2(randx, randy) + linelen * linevec, linewid);
                break;
            case 1:
                e = boxDist(rotateCW(p - vec2(randx, randy), randrot), vec2(randsz / 2), cornerrad);
                break;
            case 2:
                e = triangleDist(rotateCW(p - vec2(randx, randy), randrot), randsz);
                break;
            case 3:
                e = circleDist(p - vec2(randx, randy), randsz);
                break;
        }
        d = merge(d, e);
    }
    return d;
}
