#include "clumpy_command.hh"
#include "cnpy/cnpy.h"
#include "fmt/core.h"

using namespace std;

constexpr auto kTestSimplex = R"(
import numpy as np
from skimage import img_as_ubyte
from PIL import Image
arr = 0.5 * (np.load("noise.npy") + 1.0)
Image.fromarray(img_as_ubyte(arr)).show()
)";

constexpr auto kTestCurl = R"(
import numpy as np
from skimage import img_as_ubyte
from PIL import Image
red, grn = np.load("curl.npy")
arr = 0.5 * (grn + 1.0)
Image.fromarray(img_as_ubyte(arr)).show()
)";

constexpr auto kTestShapes = R"(
import numpy as np
from PIL import Image
Image.fromarray(np.load("shapes.npy"), "RGB").show()
)";

constexpr auto kTestPoints = R"(
import numpy as np
from PIL import Image

splats = np.load("splats.npy")
print(splats.shape)
Image.fromarray(splats, "L").show()

if False:
    pts = np.load("coords.npy")
    import matplotlib.pyplot as plt
    width, height = 500, 250
    u, v = pts.swapaxes(0,1)
    print(np.amin(u), np.amax(u))
    print(np.amin(v), np.amax(v))
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.scatter(u, v, c="#000000", alpha=0.6, lw=0, s=4)
    ax.set_aspect(1.0)
    ax.xaxis.set_visible(False)
    ax.yaxis.set_visible(False)
    plt.savefig("pts.png", dpi=1000)
    Image.open("pts.png").show()
)";

struct Test : ClumpyCommand {
    Test() {}
    string description() const override { return "test clumpy functionality (spawns python3)"; }
    string usage() const override { return ""; }
    string example() const override { return ""; }
    bool exec(vector<string> args) override;
};

bool Test::exec(vector<string> args) {
    fmt::print_colored(fmt::color::green, "Welcome to {}!\n", "clumpy");
    fmt::print_colored(fmt::color::white, "\n");

    auto spawn_python = [](string cmd) {
        system(fmt::format("python3 -c '{}'", cmd).c_str());
    };

    auto get_command = [](string cmd) {
        auto& reg = ClumpyCommand::registry();
        auto iter = reg.find(cmd);
        if (iter == reg.end()) {
            fmt::print_colored(fmt::color::red, "Unknown command: {}\n", cmd);
            exit(1);
        }
        return iter->second();
    };

    auto bridson_points = get_command("bridson_points");
    auto curl_2d = get_command("curl_2d");
    auto generate_dshapes = get_command("generate_dshapes");
    auto generate_simplex = get_command("generate_simplex");
    auto splat_points = get_command("splat_points");
    auto visualize_sdf = get_command("visualize_sdf");

    if (false) {
        generate_simplex->exec({ "500x250", "1.0", "16.0", "78", "noise.npy" });
        spawn_python(kTestSimplex);
        curl_2d->exec({ "noise.npy", "curl.npy" });
        spawn_python(kTestCurl);
    }

    if (false) {
        generate_dshapes->exec({ "500x250", "6", "47", "shapes.npy" });
        visualize_sdf->exec({ "shapes.npy", "shapes.npy" });
        spawn_python(kTestShapes);
    }

    if (false) {
        bridson_points->exec({ "500x250", "4", "987", "coords.npy" });
        splat_points->exec({ "coords.npy", "500x250", "gaussian", "5", "1.0", "splats.npy" });
        spawn_python(kTestPoints);
    }

    delete bridson_points;
    delete curl_2d;
    delete generate_dshapes;
    delete generate_simplex;
    delete splat_points;
    delete visualize_sdf;

    return true;
}

static ClumpyCommand::Register registrar("test_clumpy", [] {
    return new Test();
});
