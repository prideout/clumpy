#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <limits>

#include <glm/vec2.hpp>
#include <glm/ext.hpp>

using namespace glm;

using std::vector;
using std::string;
using std::numeric_limits;

void generate_pts(float width, float height, float radius, int seed, vector<float>& result);

namespace {

struct BridsonPoints : ClumpyCommand {
    BridsonPoints() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "generate list of 2D points";
    }
    string usage() const override {
        return "<dim> <minradius> <seed> <output_pts>";
    }
    string example() const override {
        return "500x250 10 987 bridson.npy";
    }
};

static ClumpyCommand::Register registrar("bridson_points", [] {
    return new BridsonPoints();
});

bool BridsonPoints::exec(vector<string> vargs) {
    if (vargs.size() != 4) {
        fmt::print("This command takes 4 arguments.\n");
        return false;
    }
    string dims = vargs[0];
    const uint32_t width = atoi(dims.c_str());
    const uint32_t height = atoi(dims.substr(dims.find('x') + 1).c_str());
    const float minradius = atof(vargs[1].c_str());
    const int seed = atoi(vargs[2].c_str());
    const string output_file = vargs[3].c_str();

    vector<float> result;
    generate_pts(width, height, minradius, seed, result);
    size_t npts = result.size() / 2;
    fmt::print("Generated {} points.\n", npts);
    cnpy::npy_save(output_file, result.data(), {npts, 2}, "w");

    return true;
}

// Transforms even the sequence 0,1,2,3,... into reasonably good random numbers.
unsigned int randhash(unsigned int seed) {
    unsigned int i = (seed ^ 12345391u) * 2654435769u;
    i ^= (i << 6) ^ (i >> 26);
    i *= 2654435769u;
    i += (i << 5) ^ (i >> 12);
    return i;
}

float randhashf(unsigned int seed, float a, float b) {
    return (b - a) * randhash(seed) / (float) numeric_limits<uint32_t>::max() + a;
}

vec2 sample_annulus(float radius, vec2 center, int* seedptr) {
    unsigned int seed = *seedptr;
    vec2 r;
    float rscale = 1.0f / UINT_MAX;
    while (1) {
        r.x = 4 * rscale * randhash(seed++) - 2;
        r.y = 4 * rscale * randhash(seed++) - 2;
        float r2 = dot(r, r);
        if (r2 > 1 && r2 <= 4) {
            break;
        }
    }
    *seedptr = seed;
    return r * radius + center;
}

} // anonymous namespace

#define GRIDF(vec) \
    grid[(int) (vec.x * invcell) + ncols * (int) (vec.y * invcell)]

#define GRIDI(vec) grid[(int) vec.y * ncols + (int) vec.x]

void generate_pts(float width, float height, float radius, int seed, vector<float>& result) {

    int maxattempts = 30;
    float rscale = 1.0f / UINT_MAX;
    vec2 rvec;
    rvec.x = rvec.y = radius;
    float r2 = radius * radius;

    // Acceleration grid.
    float cellsize = radius / sqrtf(2);
    float invcell = 1.0f / cellsize;
    int ncols = ceil(width * invcell);
    int nrows = ceil(height * invcell);
    int maxcol = ncols - 1;
    int maxrow = nrows - 1;
    int ncells = ncols * nrows;
    int* grid = (int*) malloc(ncells * sizeof(int));
    for (int i = 0; i < ncells; i++) {
        grid[i] = -1;
    }

    // Active list and resulting sample list.
    int* actives = (int*) malloc(ncells * sizeof(int));
    int nactives = 0;
    result.resize(ncells * 2);
    vec2* samples = (vec2*) result.data();
    int nsamples = 0;

    // First sample.
    vec2 pt;
    pt.x = width * randhash(seed++) * rscale;
    pt.y = height * randhash(seed++) * rscale;
    GRIDF(pt) = actives[nactives++] = nsamples;
    samples[nsamples++] = pt;

    while (nsamples < ncells) {
        int aindex = min(randhashf(seed++, 0, nactives), nactives - 1.0f);
        int sindex = actives[aindex];
        int found = 0;
        vec2 j, minj, maxj, delta;
        int attempt;
        for (attempt = 0; attempt < maxattempts && !found; attempt++) {
            pt = sample_annulus(radius, samples[sindex], &seed);

            // Check that this sample is within bounds.
            if (pt.x < 0 || pt.x >= width || pt.y < 0 || pt.y >= height) {
                continue;
            }

            // Test proximity to nearby samples.
            maxj = (pt + rvec) * invcell;
            minj = (pt - rvec) * invcell;

            minj.x = clamp((int) minj.x, 0, maxcol);
            minj.y = clamp((int) minj.y, 0, maxrow);
            maxj.x = clamp((int) maxj.x, 0, maxcol);
            maxj.y = clamp((int) maxj.y, 0, maxrow);
            int reject = 0;
            for (j.y = minj.y; j.y <= maxj.y && !reject; j.y++) {
                for (j.x = minj.x; j.x <= maxj.x && !reject; j.x++) {
                    int entry = GRIDI(j);
                    if (entry > -1 && entry != sindex) {
                        delta = samples[entry] - pt;
                        if (dot(delta, delta) < r2) {
                            reject = 1;
                        }
                    }
                }
            }
            if (reject) {
                continue;
            }
            found = 1;
        }
        if (found) {
            GRIDF(pt) = actives[nactives++] = nsamples;
            samples[nsamples++] = pt;
        } else {
            if (--nactives <= 0) {
                break;
            }
            actives[aindex] = actives[nactives];
        }
    }

    result.resize(nsamples * 2);

    free(grid);
    free(actives);
}

#undef GRIDF
#undef GRIDI
