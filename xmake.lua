set_project("my-game")
set_version("0.1.0")
set_languages("c++26")

add_rules("mode.debug", "mode.release")
-- Include kökü src/:  #include "engine/render/..."  #include "game/shared/..."
add_includedirs("src")

-- zlib: sistemdeki (macOS SDK) zlib'i kullan.
add_requires("zlib", {system = true})
-- protoc ve libprotobuf AYNI sürüm olmalı (generated kod ile runtime uyumu).
-- Yerel protoc (brew) de 36.2; xmake-repo'daki en yüksek sürüm de 36.2.
add_requires("protobuf-cpp 36.2")
add_requires("glm")
-- assimp: xmake tarifi minizip'i çektiği için Homebrew assimp'i pkg-config ile kullanıyoruz.
add_requires("assimp", {system = true})
add_requires("libsdl3")
-- ECS, fizik (çarpışma) ve ses: hazır kütüphaneler.
add_requires("entt")
add_requires("joltphysics")
add_requires("miniaudio")
-- Log: async, çok thread'li, hazır sink'ler (konsol + dönen dosya).
add_requires("spdlog")
-- Font atlası (stb_truetype): TTF -> R8 atlas + harf quad bilgisi.
add_requires("stb")
-- Dear ImGui: motorun tek UI sistemi. Resmi backend'ler: imgui_impl_sdl3 (girdi) + imgui_impl_sdlgpu3 (çizim, SDL_GPU).
add_requires("imgui", {configs = {sdl3 = true, sdl3_gpu = true}})
-- RmlUi: oyuncunun gördüğü arayüz (menü, HUD, skor tablosu). HTML/CSS benzeri .rml/.rcss; çizimi ui.dll içinde SDL_GPU ile.
add_requires("rmlui 6.3")

local function is_gcc_like()
    return not is_plat("windows")
end

-- Çıktı düzeni: out/<plat>-<arch>-<mode>/engine/  motor DLL'leri
--               out/<plat>-<arch>-<mode>/game/    exe'ler + oyun DLL'leri (client, server)
-- Build ara dosyaları xmake'in kendi build/<plat>/<arch>/<mode> yapısında kalır.
local outdir = "$(projectdir)/out/$(plat)-$(arch)-$(mode)"
local enginedir = outdir .. "/engine"
local gamedir = outdir .. "/game"

-- Motor DLL'leri yan yana durur; oyun hedefleri motoru ../engine üzerinden bulur.
local function engine_rpath()
    add_rpathdirs("@loader_path", "@executable_path")
end
local function game_rpath()
    add_rpathdirs("@loader_path", "@executable_path", "@loader_path/../engine", "@executable_path/../engine")
end

-- ===========================================================================
-- ENGINE (src/engine/): oyun kuralı içermez.
-- ===========================================================================

-- Ortak statik lib: CreateInterface registry, CModule (dlopen), motor arayüzleri.
target("interface")
    set_kind("static")
    set_targetdir(enginedir)
    add_files("src/engine/interface/*.cpp")
    add_headerfiles("src/engine/interface/*.hpp")
    add_packages("glm", {public = true})

-- Motorun kendi modülü (src/engine/engine): çekirdek tipler, kamera, font, config, launch options, gzip.
target("engine")
    set_kind("shared")
    set_targetdir(enginedir)
    -- Windows: engine.dll. macOS/Linux: libengine.dylib/.so (-lengine linkinin beklediği isim).
    if is_plat("windows") then set_prefixname("") end
    add_defines("ENGINE_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_files("src/engine/engine/**.cpp")
    add_headerfiles("src/engine/engine/**.hpp")
    add_packages("zlib", "glm", "entt", {public = true})
    add_packages("stb")
    engine_rpath()

-- Log (spdlog, async). Arayüz: Log001. Host'lar ilk bunu yükler; diğer modüller BindLog ile bağlanır.
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

-- Ağ (TCP + UDP, tek port). Arayüz: Network001. Sadece bayt taşır; protokol oyun tarafında.
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

-- Dinamik modüller: her biri GAME_EXPORT_MODULE ile CreateInterface export eder.
-- Birbirlerini linklemezler; host üzerinden arayüzle konuşurlar.
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

-- Asset yükleme (Assimp): model dosyaları. Arayüz: Assets001.
target("assets")
    set_kind("shared")
    set_targetdir(enginedir)
    if is_plat("windows") then set_prefixname("") end
    add_defines("ASSETS_BUILD")
    if is_gcc_like() then add_cxflags("-fvisibility=hidden") end
    add_deps("interface", "engine")
    add_files("src/engine/assets/*.cpp")
    add_headerfiles("src/engine/assets/*.hpp")
    add_packages("assimp", "glm")
    engine_rpath()

-- Ses (miniaudio). Arayüz: Audio001.
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

-- UI: RmlUi (oyun arayüzü) + Dear ImGui (geliştirici konsolu). Arayüz: UI001.
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

-- Fizik / çarpışma (Jolt). Arayüz: Physics001. Server ve client (tahmin için) yükler.
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
-- OYUN (src/game/, src/client, src/server, src/host): motoru kullanır, motorun parçası değildir.
-- ===========================================================================

-- İki tarafın da kullandığı oyun kuralları: component'ler, World_t, replikasyon, protokol (proto), level, trace, hareket.
-- Statik lib: client.dll ve server.dll kendi kopyasını içine gömer.
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

-- Client container (src/client) + client tarafı oyun kuralları (src/game/client).
-- SDL'e doğrudan bağlı değil: pencere/girdi platform.dll, çizim render.dll üzerinden.
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

-- Server container (src/server) + server tarafı oyun kuralları (src/game/server).
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

-- Host'lar: modülleri dlopen ile yükler (motor: ../engine, oyun: kendi klasörü).
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

-- Başsız entegrasyon testi (physics + ECS + replikasyon + protobuf): xmake run ecs_test
target("ecs_test")
    set_kind("binary")
    set_targetdir(gamedir)
    add_deps("game_shared", "engine", "interface")
    add_files("src/tests/ecs_test.cpp", "src/game/client/**.cpp", "src/game/server/**.cpp")
    remove_files("src/game/client/ui/**.cpp") -- ImGui kodu headless testte yok
    game_rpath()

-- Ağ testi: network_system.cpp doğrudan derlenir (iki örnek: server + client). xmake run network_test
target("network_test")
    set_kind("binary")
    set_targetdir(gamedir)
    add_deps("interface")
    add_files("src/tests/network_test.cpp", "src/engine/network/network_system.cpp")
    game_rpath()

-- Font testi: atlas + metin mesh'i + billboard (başsız).
target("font_test")
    set_kind("binary")
    set_targetdir(gamedir)
    add_deps("engine", "interface")
    add_files("src/tests/font_test.cpp")
    game_rpath()

-- Dummy executable'lar: build / çıktı dizini / link zincirini doğrulamak için. Motor kodu içermez.
target("dummy_client")
    set_kind("binary")
    set_targetdir(gamedir)
    add_files("src/host/client/client.dummy.cpp")

target("dummy_server")
    set_kind("binary")
    set_targetdir(gamedir)
    add_files("src/host/server/server.dummy.cpp")
