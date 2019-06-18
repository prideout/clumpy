// TODO: should I be using blend2d instead?

#define PAR_SHAPES_T uint32_t
#include "par/par_shapes.h"
#include "stb/stb_truetype.h"

#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec3.hpp>
#include <glm/ext.hpp>

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

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

static void addFloorGeometry(RTCScene scene, RTCDevice device) {

}

static void addSphereGeometry(RTCScene scene, RTCDevice device) {
    
}

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

    vector<vec3> dstimg(output_width * output_height);

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);
    addFloorGeometry(scene, device);
    addSphereGeometry(scene, device);
    rtcCommitScene(scene);
    // TODO
    rtcReleaseScene(scene);
    rtcReleaseDevice(device);

    cnpy::npy_save(output_file, &dstimg.data()->x, {output_height, output_width, 3}, "w");
    return true;
}

} // anonymous namespace
