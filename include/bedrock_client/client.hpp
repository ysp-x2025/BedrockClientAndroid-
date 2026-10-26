#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
    #if defined(BEDROCK_CLIENT_BUILD)
        #define BC_API __declspec(dllexport)
    #else
        #define BC_API __declspec(dllimport)
    #endif
#else
    #define BC_API __attribute__((visibility("default")))
#endif

namespace bedrock {

struct Vec2 {
    float x{};
    float y{};
};

struct Vec3 {
    float x{};
    float y{};
    float z{};

    Vec3 operator+(const Vec3& rhs) const;
    Vec3 operator-(const Vec3& rhs) const;
    Vec3 operator*(float scalar) const;
};

struct Color {
    float r{1.0F};
    float g{1.0F};
    float b{1.0F};
    float a{1.0F};
};

enum class Module : std::uint8_t {
    Hud,
    Freecam,
    Zoom,
    Keystrokes,
    EntityRadar,
    Esp,
    Count
};

struct EntitySnapshot {
    std::uint64_t id{};
    std::string name;
    Vec3 position;
    Vec3 boundsMin{-0.3F, 0.0F, -0.3F};
    Vec3 boundsMax{0.3F, 1.8F, 0.3F};
    bool isPlayer{};
    bool alive{true};
};

struct CameraState {
    Vec3 position;
    float yawDegrees{};
    float pitchDegrees{};
    float verticalFovDegrees{70.0F};
};

struct InputState {
    float forward{};
    float strafe{};
    float vertical{};
    float lookX{};
    float lookY{};
    bool jump{};
    bool sneak{};
    bool sprint{};
};

struct Viewport {
    float width{1920.0F};
    float height{1080.0F};
};

struct HudData {
    Vec3 position;
    float fps{};
    float yawDegrees{};
    float pitchDegrees{};
    std::string direction;
};

struct RadarEntry {
    std::uint64_t entityId{};
    std::string name;
    float distance{};
    float bearingDegrees{};
    bool isPlayer{};
};

struct EspEntry {
    std::uint64_t entityId{};
    std::string label;
    Vec2 topLeft;
    Vec2 bottomRight;
    Color color;
    float distance{};
};

struct ClientConfig {
    bool hudEnabled{true};
    bool freecamEnabled{false};
    bool zoomEnabled{false};
    bool keystrokesEnabled{true};
    bool radarEnabled{true};
    bool espEnabled{true};

    float zoomFactor{2.0F};
    float freecamSpeed{5.0F};
    float mouseSensitivity{0.15F};
    float radarRange{64.0F};
    float espRange{64.0F};

    bool showCoordinates{true};
    bool showFps{true};
    bool showDistance{true};
};

class BC_API Client {
public:
    Client();

    bool enabled(Module module) const;
    void setEnabled(Module module, bool enabled);

    ClientConfig config() const;
    void setConfig(const ClientConfig& config);

    bool saveConfig(const std::filesystem::path& path) const;
    bool loadConfig(const std::filesystem::path& path);

    HudData makeHud(const CameraState& camera, float fps) const;

    std::vector<RadarEntry> makeRadar(
        const Vec3& origin,
        float yawDegrees,
        const std::vector<EntitySnapshot>& entities
    ) const;

    std::vector<EspEntry> makeEsp(
        const CameraState& camera,
        const Viewport& viewport,
        const std::vector<EntitySnapshot>& entities
    ) const;

    CameraState updateFreecam(
        const CameraState& current,
        const InputState& input,
        float deltaSeconds
    ) const;

    float effectiveFov(float baseFovDegrees) const;

    static std::optional<Vec2> worldToScreen(
        const Vec3& point,
        const CameraState& camera,
        const Viewport& viewport
    );

private:
    ClientConfig config_;
};

} // namespace bedrock
