set_project("my-game")
set_version("0.1.0")
set_languages("c++26")

add_rules("mode.debug", "mode.release")
-- Include root is src/:  #include "engine/render/..."  #include "game/shared/..."
add_includedirs("src")

-- zlib: use the system zlib (macOS SDK).
add_requires("zlib", {system = true})
-- protoc and libprotobuf must be the SAME version (generated code must match the runtime).
-- The local protoc (brew) is 36.2 too; the highest version in xmake-repo is also 36.2.
add_requires("protobuf-cpp 36.2")
add_requires("glm")
-- assimp: the xmake recipe pulls in minizip, so we use Homebrew's assimp through pkg-config.
add_requires("assimp", {system = true})
add_requires("libsdl3")
-- ECS, physics (collision) and audio: ready-made libraries.
add_requires("entt")
add_requires("joltphysics")
add_requires("miniaudio")
-- Log: async, multi-threaded, ready-made sinks (console + rotating file).
add_requires("spdlog")
-- Font atlas (stb_truetype): TTF -> R8 atlas + glyph quad data.
add_requires("stb")
-- Dear ImGui: the engine's only UI system. Official backends: imgui_impl_sdl3 (input) + imgui_impl_sdlgpu3 (drawing, SDL_GPU).
add_requires("imgui", {configs = {sdl3 = true, sdl3_gpu = true}})
-- RmlUi: the interface the player sees (menu, HUD, scoreboard). HTML/CSS-like .rml/.rcss; drawn with SDL_GPU inside ui.dll.
add_requires("rmlui 6.3")

local function is_gcc_like()
    return not is_plat("windows")
end

-- Output layout: out/<plat>-<arch>-<mode>/engine/  engine DLLs
--               out/<plat>-<arch>-<mode>/game/    executables + game DLLs (client, server)
-- Build intermediates stay in xmake's own build/<plat>/<arch>/<mode> layout.
local outdir = "$(projectdir)/out/$(plat)-$(arch)-$(mode)"
local enginedir = outdir .. "/engine"
local gamedir = outdir .. "/game"

-- Engine DLLs sit side by side; game targets find the engine through ../engine.
local function engine_rpath()
    add_rpathdirs("@loader_path", "@executable_path")
end
local function game_rpath()
    add_rpathdirs("@loader_path", "@executable_path", "@loader_path/../engine", "@executable_path/../engine")
end

-- ===========================================================================
-- ENGINE (src/engine/): contains no game rules.
-- ===========================================================================

-- Shared static lib: CreateInterface registry, CModule (dlopen), engine interfaces.
target("interface")
    set_kind("static")
    set_targetdir(enginedir)
    add_files("src/engine/interface/*.cpp")
    add_headerfiles("src/engine/interface/*.hpp")
    add_packages("glm", {public = true})

-- The engine's own module (src/engine/engine): core types, camera, font, config, launch options, gzip.
target("engine")
    set_kind("shared")
    set_targetdir(enginedir)
    -- Windows: engine.dll. macOS/Linux: libengine.dylib/.so (the name the -lengine link expects).
    if is_plat("windows") then set_prefixname("") end
    add_defines("ENGINE_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_files("src/engine/engine/**.cpp")
    add_headerfiles("src/engine/engine/**.hpp")
    add_packages("zlib", "glm", "entt", {public = true})
    add_packages("stb")
    engine_rpath()

-- Log (spdlog, async). Interface: Log001. The hosts load this first; other modules bind through BindLog.
target("log")
    set_kind("shared")
    set_targetdir(enginedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("LOG_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("interface")
    add_files("src/engine/log/*.cpp")
    add_packages("spdlog")
    engine_rpath()

-- Network (TCP + UDP, one port). Interface: Network001. Carries only bytes; the protocol lives on the game side.
target("network")
    set_kind("shared")
    set_targetdir(enginedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("NETWORK_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("interface")
    add_files("src/engine/network/*.cpp")
    add_headerfiles("src/engine/network/*.hpp")
    engine_rpath()

-- Dynamic modules: each one exports CreateInterface via GAME_EXPORT_MODULE.
-- They do not link each other; they talk through interfaces via the host.
target("platform")
    set_kind("shared")
    set_targetdir(enginedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("PLATFORM_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("interface")
    add_files("src/engine/platform/*.cpp")
    add_headerfiles("src/engine/platform/*.hpp")
    add_packages("libsdl3", "glm")
    engine_rpath()

target("render")
    set_kind("shared")
    set_targetdir(enginedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("RENDER_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("interface")
    add_files("src/engine/render/**.cpp")
    add_headerfiles("src/engine/render/**.hpp")
    add_packages("libsdl3", "glm")
    engine_rpath()

target("particles")
    set_kind("shared")
    set_targetdir(enginedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("PARTICLES_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("interface")
    add_files("src/engine/particles/*.cpp")
    add_headerfiles("src/engine/particles/*.hpp")
    add_packages("glm")
    engine_rpath()

-- Asset loading (Assimp): model files. Interface: Assets001.
target("assets")
    set_kind("shared")
    set_targetdir(enginedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("ASSETS_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("interface", "engine")
    add_files("src/engine/assets/*.cpp")
    add_headerfiles("src/engine/assets/*.hpp")
    add_packages("assimp", "glm", "stb")
    engine_rpath()

-- Audio (miniaudio). Interface: Audio001.
target("audio")
    set_kind("shared")
    set_targetdir(enginedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("AUDIO_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("interface", "engine")
    add_files("src/engine/audio/*.cpp")
    add_headerfiles("src/engine/audio/*.hpp")
    add_packages("miniaudio")
    engine_rpath()

-- UI: RmlUi (game UI) + Dear ImGui (developer console). Interface: UI001.
target("ui")
    set_kind("shared")
    set_targetdir(enginedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("UI_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("interface", "engine")
    add_files("src/engine/ui/**.cpp")
    add_headerfiles("src/engine/ui/**.hpp")
    add_packages("glm", "imgui", "libsdl3", "rmlui", "stb")
    engine_rpath()

-- Physics / collision (Jolt). Interface: Physics001. Loaded by the server and by the client (for prediction).
target("physics")
    set_kind("shared")
    set_targetdir(enginedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("PHYSICS_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("interface")
    add_files("src/engine/physics/*.cpp")
    add_headerfiles("src/engine/physics/*.hpp")
    add_packages("joltphysics", "glm")
    engine_rpath()

-- ===========================================================================
-- GAME (src/game/, src/client, src/server, src/host): uses the engine; it is not part of the engine.
-- ===========================================================================

-- Game rules used by both sides: components, World_t, replication, protocol (proto), level, trace, movement.
-- Static lib: client.dll and server.dll each embed their own copy.
target("game_shared")
    set_kind("static")
    set_targetdir(gamedir)
    if is_gcc_like() then add_cxflags("-fvisibility=hidden", "-fPIC") end
    add_rules("protobuf.cpp")
    add_deps("engine", "interface")
    add_files("src/game/shared/*.cpp")
    add_headerfiles("src/game/shared/*.hpp")
    add_files("proto/*.proto", {proto_public = true})
    add_packages("protobuf-cpp", "glm", "entt", {public = true})

-- Client container (src/client) + client-side game rules (src/game/client).
-- Not directly linked to SDL: window/input go through platform.dll, drawing through render.dll.
target("client")
    set_kind("shared")
    set_targetdir(gamedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("CLIENT_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("game_shared", "engine", "interface")
    add_files("src/client/**.cpp", "src/game/client/**.cpp")
    add_headerfiles("src/client/**.hpp", "src/game/client/**.hpp")
    add_packages("glm", "imgui")
    game_rpath()

-- Server container (src/server) + server-side game rules (src/game/server).
target("server")
    set_kind("shared")
    set_targetdir(gamedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("SERVER_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("game_shared", "engine", "interface")
    add_files("src/server/**.cpp", "src/game/server/**.cpp")
    add_headerfiles("src/server/**.hpp", "src/game/server/**.hpp")
    game_rpath()

-- Hosts: load the modules with dlopen (engine: ../engine, game: its own folder).
target("game")
    set_kind("binary")
    set_targetdir(gamedir)
    add_deps("engine", "interface")
    add_files("src/host/client/main.cpp")
    game_rpath()
    set_rundir("$(projectdir)")

target("dedicated_server")
    set_kind("binary")
    set_targetdir(gamedir)
    add_deps("engine", "interface")
    add_files("src/host/server/main.cpp")
    game_rpath()
    set_rundir("$(projectdir)")

-- Headless integration test (physics + ECS + replication + protobuf): xmake run ecs_test
target("ecs_test")
    set_kind("binary")
    set_targetdir(gamedir)
    add_deps("game_shared", "engine", "interface")
    add_files("src/tests/ecs_test.cpp", "src/game/client/**.cpp", "src/game/server/**.cpp")
    remove_files("src/game/client/ui/**.cpp") -- no ImGui code in the headless test
    game_rpath()

-- Network test: network_system.cpp is compiled directly (two instances: server + client). xmake run network_test
target("network_test")
    set_kind("binary")
    set_targetdir(gamedir)
    add_deps("interface")
    add_files("src/tests/network_test.cpp", "src/engine/network/network_system.cpp")
    game_rpath()

-- Animation test: skinned_loader.cpp is compiled directly (Assimp, skeleton, pose, clip blending; headless).
target("anim_test")
    set_kind("binary")
    set_targetdir(gamedir)
    add_deps("engine", "interface")
    add_files("src/tests/anim_test.cpp", "src/engine/assets/skinned_loader.cpp")
    add_packages("assimp", "glm", "stb")
    game_rpath()
    set_rundir("$(projectdir)")

-- Font test: atlas + text mesh + billboard (headless).
target("font_test")
    set_kind("binary")
    set_targetdir(gamedir)
    add_deps("engine", "interface")
    add_files("src/tests/font_test.cpp")
    game_rpath()

-- Dummy executables: verify the build / output directory / link chain. They contain no engine code.
target("dummy_client")
    set_kind("binary")
    set_targetdir(gamedir)
    add_files("src/host/client/client.dummy.cpp")

target("dummy_server")
    set_kind("binary")
    set_targetdir(gamedir)
    add_files("src/host/server/server.dummy.cpp")
