#define BEDROCK_CLIENT_BUILD
#include "bedrock_client/client.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace bedrock {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kMinFov = 20.0F;
constexpr float kMaxFov = 120.0F;
constexpr float kNearPlane = 0.05F;
constexpr float kMaxDeltaSeconds = 0.1F;

float radians(float degrees) {
    return degrees * kPi / 180.0F;
}

float degrees(float radiansValue) {
    return radiansValue * 180.0F / kPi;
}

float clampFov(float value) {
    return std::clamp(value, kMinFov, kMaxFov);
}

float wrapDegrees(float value) {
    value = std::fmod(value + 180.0F, 360.0F);
    if (value < 0.0F) value += 360.0F;
    return value - 180.0F;
}

float length(const Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec3 normalize(const Vec3& v) {
    const float magnitude = length(v);
    if (magnitude < 1.0e-6F) return {};
    return v * (1.0F / magnitude);
}

Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

bool validNumber(float value) {
    return std::isfinite(value);
}

std::string directionForYaw(float yaw) {
    const float normalized = std::fmod(yaw + 360.0F, 360.0F);
    constexpr std::array<const char*, 8> directions{
        "South", "South-West", "West", "North-West",
        "North", "North-East", "East", "South-East"
    };

    const int index = static_cast<int>(
        std::floor((normalized + 22.5F) / 45.0F)
    ) % 8;

    return directions[static_cast<std::size_t>(index)];
}

Color colorForEntity(const EntitySnapshot& entity) {
    if (entity.isPlayer) {
        return {0.25F, 0.65F, 1.0F, 0.95F};
    }
    return {0.35F, 1.0F, 0.45F, 0.95F};
}

} // namespace

Vec3 Vec3::operator+(const Vec3& rhs) const {
    return {x + rhs.x, y + rhs.y, z + rhs.z};
}

Vec3 Vec3::operator-(const Vec3& rhs) const {
    return {x - rhs.x, y - rhs.y, z - rhs.z};
}

Vec3 Vec3::operator*(float scalar) const {
    return {x * scalar, y * scalar, z * scalar};
}

Client::Client() = default;

bool Client::enabled(Module module) const {
    switch (module) {
        case Module::Hud:         return config_.hudEnabled;
        case Module::Freecam:     return config_.freecamEnabled;
        case Module::Zoom:        return config_.zoomEnabled;
        case Module::Keystrokes:  return config_.keystrokesEnabled;
        case Module::EntityRadar: return config_.radarEnabled;
        case Module::Esp:         return config_.espEnabled;
        case Module::Count:       return false;
    }
    return false;
}

void Client::setEnabled(Module module, bool value) {
    switch (module) {
        case Module::Hud:         config_.hudEnabled = value; break;
        case Module::Freecam:     config_.freecamEnabled = value; break;
        case Module::Zoom:        config_.zoomEnabled = value; break;
        case Module::Keystrokes:  config_.keystrokesEnabled = value; break;
        case Module::EntityRadar: config_.radarEnabled = value; break;
        case Module::Esp:         config_.espEnabled = value; break;
        case Module::Count: break;
    }
}

ClientConfig Client::config() const {
    return config_;
}

void Client::setConfig(const ClientConfig& input) {
    config_ = input;

    if (!validNumber(config_.zoomFactor)) config_.zoomFactor = 2.0F;
    if (!validNumber(config_.freecamSpeed)) config_.freecamSpeed = 5.0F;
    if (!validNumber(config_.mouseSensitivity)) {
        config_.mouseSensitivity = 0.15F;
    }
    if (!validNumber(config_.radarRange)) config_.radarRange = 64.0F;
    if (!validNumber(config_.espRange)) config_.espRange = 64.0F;

    config_.zoomFactor = std::clamp(config_.zoomFactor, 1.0F, 10.0F);
    config_.freecamSpeed = std::clamp(config_.freecamSpeed, 0.1F, 50.0F);
    config_.mouseSensitivity =
        std::clamp(config_.mouseSensitivity, 0.01F, 2.0F);
    config_.radarRange = std::clamp(config_.radarRange, 1.0F, 512.0F);
    config_.espRange = std::clamp(config_.espRange, 1.0F, 512.0F);
}

bool Client::saveConfig(const std::filesystem::path& path) const {
    const auto tempPath = path.string() + ".tmp";
    {
        std::ofstream out(tempPath, std::ios::trunc);
        if (!out) return false;

        out << "version=1\n"
            << std::boolalpha
            << "hudEnabled=" << config_.hudEnabled << '\n'
            << "freecamEnabled=" << config_.freecamEnabled << '\n'
            << "zoomEnabled=" << config_.zoomEnabled << '\n'
            << "keystrokesEnabled=" << config_.keystrokesEnabled << '\n'
            << "radarEnabled=" << config_.radarEnabled << '\n'
            << "espEnabled=" << config_.espEnabled << '\n'
            << std::setprecision(7)
            << "zoomFactor=" << config_.zoomFactor << '\n'
            << "freecamSpeed=" << config_.freecamSpeed << '\n'
            << "mouseSensitivity=" << config_.mouseSensitivity << '\n'
            << "radarRange=" << config_.radarRange << '\n'
            << "espRange=" << config_.espRange << '\n'
            << "showCoordinates=" << config_.showCoordinates << '\n'
            << "showFps=" << config_.showFps << '\n'
            << "showDistance=" << config_.showDistance << '\n';

        out.flush();
        if (!out) {
            out.close();
            std::error_code ignored;
            std::filesystem::remove(tempPath, ignored);
            return false;
        }
    }

    std::error_code error;
    std::filesystem::rename(tempPath, path, error);

    if (!error) return true;

    // Some filesystems cannot replace an existing destination atomically.
    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(tempPath, path, error);

    if (error) {
        std::filesystem::remove(tempPath, error);
        return false;
    }
    return true;
}

bool Client::loadConfig(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) return false;

    ClientConfig candidate = config_;
    std::string line;
    bool recognized = false;

    while (std::getline(in, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) continue;

        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);

        auto readBool = [&](bool& target) -> bool {
            if (value == "true" || value == "1") {
                target = true;
                return true;
            }
            if (value == "false" || value == "0") {
                target = false;
                return true;
            }
            return false;
        };

        auto readFloat = [&](float& target) -> bool {
            try {
                std::size_t consumed = 0;
                const float parsed = std::stof(value, &consumed);
                if (consumed != value.size() || !validNumber(parsed)) {
                    return false;
                }
                target = parsed;
                return true;
            } catch (...) {
                return false;
            }
        };

        bool known = true;
        bool valid = true;

        if (key == "version") {
            valid = value == "1";
        } else if (key == "hudEnabled") {
            valid = readBool(candidate.hudEnabled);
        } else if (key == "freecamEnabled") {
            valid = readBool(candidate.freecamEnabled);
        } else if (key == "zoomEnabled") {
            valid = readBool(candidate.zoomEnabled);
        } else if (key == "keystrokesEnabled") {
            valid = readBool(candidate.keystrokesEnabled);
        } else if (key == "radarEnabled") {
            valid = readBool(candidate.radarEnabled);
        } else if (key == "espEnabled") {
            valid = readBool(candidate.espEnabled);
        } else if (key == "zoomFactor") {
            valid = readFloat(candidate.zoomFactor);
        } else if (key == "freecamSpeed") {
            valid = readFloat(candidate.freecamSpeed);
        } else if (key == "mouseSensitivity") {
            valid = readFloat(candidate.mouseSensitivity);
        } else if (key == "radarRange") {
            valid = readFloat(candidate.radarRange);
        } else if (key == "espRange") {
            valid = readFloat(candidate.espRange);
        } else if (key == "showCoordinates") {
            valid = readBool(candidate.showCoordinates);
        } else if (key == "showFps") {
            valid = readBool(candidate.showFps);
        } else if (key == "showDistance") {
            valid = readBool(candidate.showDistance);
        } else {
            known = false;
        }

        if (known && !valid) return false;
        recognized = recognized || known;
    }

    if (!in.eof() || !recognized) return false;
    setConfig(candidate);
    return true;
}

HudData Client::makeHud(const CameraState& camera, float fps) const {
    HudData result;
    result.position = camera.position;
    result.fps = validNumber(fps) ? std::max(0.0F, fps) : 0.0F;
    result.yawDegrees = wrapDegrees(camera.yawDegrees);
    result.pitchDegrees = std::clamp(camera.pitchDegrees, -90.0F, 90.0F);
    result.direction = directionForYaw(result.yawDegrees);
    return result;
}

std::vector<RadarEntry> Client::makeRadar(
    const Vec3& origin,
    float yawDegrees,
    const std::vector<EntitySnapshot>& entities
) const {
    std::vector<RadarEntry> result;
    if (!config_.radarEnabled) return result;

    const float maxRange = config_.radarRange;

    for (const auto& entity : entities) {
        if (!entity.alive) continue;

        const Vec3 delta = entity.position - origin;
        const float distance = length(delta);

        if (!validNumber(distance) || distance < 0.001F ||
            distance > maxRange) {
            continue;
        }

        const float worldBearing = degrees(std::atan2(delta.x, delta.z));
        const float relativeBearing = wrapDegrees(worldBearing - yawDegrees);

        result.push_back({
            entity.id,
            entity.name,
            distance,
            relativeBearing,
            entity.isPlayer
        });
    }

    std::sort(result.begin(), result.end(),
        [](const RadarEntry& a, const RadarEntry& b) {
            return a.distance < b.distance;
        });

    return result;
}

std::optional<Vec2> Client::worldToScreen(
    const Vec3& point,
    const CameraState& camera,
    const Viewport& viewport
) {
    if (!validNumber(viewport.width) || !validNumber(viewport.height) ||
        viewport.width <= 0.0F || viewport.height <= 0.0F) {
        return std::nullopt;
    }

    const float yaw = radians(camera.yawDegrees);
    const float pitch = radians(camera.pitchDegrees);

    const Vec3 forward{
        -std::sin(yaw) * std::cos(pitch),
        -std::sin(pitch),
         std::cos(yaw) * std::cos(pitch)
    };

    const Vec3 right{std::cos(yaw), 0.0F, std::sin(yaw)};
    const Vec3 up = normalize(cross(right, forward));
    const Vec3 relative = point - camera.position;

    const float depth = dot(relative, forward);
    if (!validNumber(depth) || depth <= kNearPlane) {
        return std::nullopt;
    }

    const float fov = radians(clampFov(camera.verticalFovDegrees));
    const float focal = (viewport.height * 0.5F) / std::tan(fov * 0.5F);

    const float screenX =
        viewport.width * 0.5F + dot(relative, right) * focal / depth;
    const float screenY =
        viewport.height * 0.5F - dot(relative, up) * focal / depth;

    if (!validNumber(screenX) || !validNumber(screenY)) {
        return std::nullopt;
    }

    return Vec2{screenX, screenY};
}

std::vector<EspEntry> Client::makeEsp(
    const CameraState& camera,
    const Viewport& viewport,
    const std::vector<EntitySnapshot>& entities
) const {
    std::vector<EspEntry> result;
    if (!config_.espEnabled) return result;

    for (const auto& entity : entities) {
        if (!entity.alive) continue;

        const float distance = length(entity.position - camera.position);
        if (!validNumber(distance) || distance > config_.espRange) continue;

        std::array<Vec2, 8> corners{};
        std::size_t index = 0;
        bool allValid = true;

        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const Vec3 point{
                        entity.position.x + (x ? entity.boundsMax.x
                                               : entity.boundsMin.x),
                        entity.position.y + (y ? entity.boundsMax.y
                                               : entity.boundsMin.y),
                        entity.position.z + (z ? entity.boundsMax.z
                                               : entity.boundsMin.z)
                    };

                    auto projected = worldToScreen(point, camera, viewport);
                    if (!projected) {
                        allValid = false;
                        break;
                    }
                    corners[index++] = *projected;
                }
                if (!allValid) break;
            }
            if (!allValid) break;
        }

        if (!allValid || index != corners.size()) continue;

        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float maxY = std::numeric_limits<float>::lowest();

        for (const auto& point : corners) {
            minX = std::min(minX, point.x);
            minY = std::min(minY, point.y);
            maxX = std::max(maxX, point.x);
            maxY = std::max(maxY, point.y);
        }

        if (maxX < 0.0F || maxY < 0.0F ||
            minX > viewport.width || minY > viewport.height) {
            continue;
        }

        result.push_back({
            entity.id,
            entity.name,
            {minX, minY},
            {maxX, maxY},
            colorForEntity(entity),
            distance
        });
    }

    return result;
}

CameraState Client::updateFreecam(
    const CameraState& current,
    const InputState& input,
    float deltaSeconds
) const {
    CameraState next = current;
    if (!config_.freecamEnabled) return next;

    const float dt = std::clamp(
        validNumber(deltaSeconds) ? deltaSeconds : 0.0F,
        0.0F,
        kMaxDeltaSeconds
    );

    next.yawDegrees = wrapDegrees(
        current.yawDegrees + input.lookX * config_.mouseSensitivity
    );

    next.pitchDegrees = std::clamp(
        current.pitchDegrees - input.lookY * config_.mouseSensitivity,
        -89.0F,
        89.0F
    );

    const float yaw = radians(next.yawDegrees);
    const float pitch = radians(next.pitchDegrees);

    const Vec3 forward{
        -std::sin(yaw) * std::cos(pitch),
        -std::sin(pitch),
         std::cos(yaw) * std::cos(pitch)
    };

    const Vec3 right{std::cos(yaw), 0.0F, std::sin(yaw)};
    const float vertical = std::clamp(input.vertical, -1.0F, 1.0F);
    const float forwardAmount = std::clamp(input.forward, -1.0F, 1.0F);
    const float strafeAmount = std::clamp(input.strafe, -1.0F, 1.0F);

    Vec3 movement = forward * forwardAmount +
                    right * strafeAmount +
                    Vec3{0.0F, vertical, 0.0F};

    const float magnitude = length(movement);
    if (magnitude > 1.0F) movement = movement * (1.0F / magnitude);

    const float speed = config_.freecamSpeed *
                        (input.sprint ? 2.0F : 1.0F);

    next.position = current.position + movement * (speed * dt);
    next.verticalFovDegrees = clampFov(current.verticalFovDegrees);
    return next;
}

float Client::effectiveFov(float baseFovDegrees) const {
    const float base = clampFov(
        validNumber(baseFovDegrees) ? baseFovDegrees : 70.0F
    );

    if (!config_.zoomEnabled) return base;

    return clampFov(
        degrees(2.0F * std::atan(
            std::tan(radians(base) * 0.5F) / config_.zoomFactor
        ))
    );
}

} // namespace bedrock
