#include "clumpy_command.hh"
#include "cnpy/cnpy.h"
#include "fmt/core.h"

using namespace std;

constexpr auto kTestPoints = R"(
import numpy as np
from PIL import Image

splats = np.load("splats.npy")
print(splats.shape)
Image.fromarray(255 - splats, "L").show()

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
        // Strip leading space if the string starts with "\n ".
        if (cmd.size() > 1 && cmd[0] == '\n' && cmd[1] == ' ') {
            int i = 1;
            while (cmd[++i] == ' ');
            stringstream ss(cmd);
            string result, ln;
            while (getline(ss, ln, '\n')) {
                if (ln.size() > i - 1) {
                    ln = ln.substr(i - 1);
                }
                result = result + ln + "\n";
            }
            cmd = result;
        }
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

    auto exec = [](auto cmd, string args) {
        stringstream ss(args);
        istream_iterator<string> begin(ss);
        istream_iterator<string> end;
        vector<string> vstrings(begin, end);
        bool success = cmd->exec(vstrings);
        if (!success) {
            fmt::print_colored(fmt::color::red, "Failure: {}\n", args);
            exit(1);
        }
    };

    auto advect_points = get_command("advect_points");
    auto bridson_points = get_command("bridson_points");
    auto cull_points = get_command("cull_points");
    auto curl_2d = get_command("curl_2d");
    auto generate_dshapes = get_command("generate_dshapes");
    auto generate_simplex = get_command("generate_simplex");
    auto infinite_zoom = get_command("infinite_zoom");
    auto splat_points = get_command("splat_points");
    auto visualize_sdf = get_command("visualize_sdf");

    if (false) {
        exec(generate_simplex, "500x250 1.0 16.0 78 noise.npy");
        spawn_python(R"(
            import numpy as np
            from skimage import img_as_ubyte
            from PIL import Image
            arr = 0.5 * (np.load("noise.npy") + 1.0)
            Image.fromarray(img_as_ubyte(arr)).show())");
        exec(curl_2d, "noise.npy curl.npy");
        spawn_python(R"(
            import numpy as np
            from skimage import img_as_ubyte
            from PIL import Image
            red, grn = np.load("curl.npy")
            arr = 0.5 * (grn + 1.0)
            Image.fromarray(img_as_ubyte(arr)).show())");
    }

    if (false) {
        exec(generate_dshapes, "500x250 6 47 shapes.npy");
        exec(visualize_sdf, "shapes.npy rgba vizshapes.npy");
        spawn_python(R"(
            import numpy as np
            from PIL import Image
            Image.fromarray(np.load("vizshapes.npy"), "RGBA").show())");
    }

    if (false) {
        exec(bridson_points, "500x250 15 987 coords.npy");
        exec(generate_dshapes, "500x250 1 0 shapes.npy");
        exec(cull_points, "coords.npy shapes.npy culled.npy");
        exec(splat_points, "culled.npy 500x250 u8disk 7 1.0 splats.npy");
        spawn_python(kTestPoints);
    }

    if (true) {
        // from PIL import ImageOps
        // im = ImageOps.expand(im, border=(int((1920-1024)/2),0), fill=0xff785936)
        exec(infinite_zoom, "1024x1024 1 22");
        exec(infinite_zoom, "1024x1024 1 07");
        spawn_python(R"(
            import numpy as np
            from os import system
            from PIL import Image
            im0 = np.load("island22.npy")
            im1 = np.load("island07.npy")
            im = np.hstack([im0, im1])
            im = Image.fromarray(im, "RGBA")
            im = im.resize((1024, 512), Image.BILINEAR)
            im.save("islands.png")
            system("open islands.png") )");
    }

    delete advect_points;
    delete bridson_points;
    delete cull_points;
    delete curl_2d;
    delete generate_dshapes;
    delete generate_simplex;
    delete infinite_zoom;
    delete splat_points;
    delete visualize_sdf;

    return true;
}

static ClumpyCommand::Register registrar("test_clumpy", [] {
    return new Test();
});
