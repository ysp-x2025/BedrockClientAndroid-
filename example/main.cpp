#include "bedrock_client/client.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

int main() {
    using namespace bedrock;

    Client client;

    ClientConfig cfg = client.config();
    cfg.zoomEnabled = true;
    cfg.zoomFactor = 3.0F;
    cfg.radarEnabled = true;
    cfg.espEnabled = true;
    cfg.freecamEnabled = true;
    client.setConfig(cfg);

    assert(client.enabled(Module::Zoom));
    assert(client.effectiveFov(70.0F) < 70.0F);

    CameraState camera{
        {0.0F, 0.0F, 0.0F},
        0.0F,
        0.0F,
        70.0F
    };

    InputState input;
    input.forward = 1.0F;

    const CameraState moved =
        client.updateFreecam(camera, input, 0.05F);

    assert(moved.position.z > camera.position.z);

    const std::vector<EntitySnapshot> entities{
        {
            1, "ExamplePlayer", {0.0F, 0.0F, 10.0F},
            {-0.3F, 0.0F, -0.3F}, {0.3F, 1.8F, 0.3F},
            true, true
        },
        {
            2, "ExampleMob", {3.0F, 0.0F, 12.0F},
            {-0.4F, 0.0F, -0.4F}, {0.4F, 1.0F, 0.4F},
            false, true
        }
    };

    const auto radar = client.makeRadar(
        camera.position, camera.yawDegrees, entities
    );

    const auto esp = client.makeEsp(
        camera, Viewport{1920.0F, 1080.0F}, entities
    );

    const auto hud = client.makeHud(camera, 60.0F);

    assert(radar.size() == 2);
    assert(!esp.empty());
    assert(std::abs(hud.fps - 60.0F) < 0.01F);

    const auto configPath =
        std::filesystem::temp_directory_path() / "bedrock-client-test.cfg";

    assert(client.saveConfig(configPath));

    Client restored;
    assert(restored.loadConfig(configPath));
    assert(restored.enabled(Module::Zoom));

    std::error_code ignored;
    std::filesystem::remove(configPath, ignored);

    std::cout << "All library tests passed.\n";
    std::cout << "Radar entries: " << radar.size() << '\n';
    std::cout << "ESP entries: " << esp.size() << '\n';
    std::cout << "Direction: " << hud.direction << '\n';

    return 0;
}
