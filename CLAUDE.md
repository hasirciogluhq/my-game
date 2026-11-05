# my-game

Online game skeleton, **ECS-based** (EnTT). Two halves: a reusable **game engine** (`src/engine/`, many independent DLLs, no game rules) and the **game** (`src/game/` rules, `src/client` and `src/server` containers, `src/host/*` executables). The server is authoritative.

## Working rules

- **Keep code simple.** Short functions, no clever abstractions. When code gets complex, simplify it (keep simplifying as you go).
- **Use ready-made libraries** (glm, EnTT, Jolt, miniaudio, Assimp, protobuf, SDL3) instead of writing our own; extend them only when needed.
- **Fix wrong/buggy old code instead of building on it.**
- **Prefer namespaced free functions over classes.** Use a class only when it holds state or implements an interface (`CAssetSystem` cache, `CPhysicsSystem`, `CClientApp`). Stateless helpers are `namespace game::xxx { fn }` (`gzip::compress`, `protocol::encode`, `world::tick`).
- File edits: use the built-in Read/Edit/Write tools, not sed/python replacement scripts.
- **NO backward compatibility. Ever.** Never keep an old interface, version, function or file alive "so old code does not break". When an API changes, change it, delete the old one, and update every caller in the same change; the build breaking in between is fine. Do not add `Foo002 : Foo`, compat shims, deprecated wrappers or aliases. The game is not released yet, so there is nothing to stay compatible with.
- **Interface versions are never bumped unless the user explicitly asks.** The version number in an interface name (`Renderer001`, ...) is a release concern, not a local-dev tool: during development you change the current version in place and fix all callers. Do not bump it on your own to "mark" a change. After release, a version bump may be needed; then it is done either as one direct change, or, if that is not possible, in separate steps. That is decided with the user at that time, never by you.
- **One system, one file pair; nested folders. (Rule 1)** Never put several systems or managers in one all-in-one file. Every system (`systems::movement`, `systems::weapon`, ...) and every manager (`CNetClient`, `CNetServer`, ...) gets its own `hpp`/`cpp` in a nested folder named for its group. Applies to `src/game/client`, `src/game/server`, `src/client`, `src/server` (e.g. `game/server/systems/movement.cpp`, `game/server/control/control_request.cpp`, `game/client/prediction/prediction.cpp`, `client/net/net_client.cpp`, `server/net/net_server.cpp`, `client/scene/draw.cpp`). A container's root keeps only its app/module files. Build globs are recursive (`**.cpp`).
- **Entity list + connection manager in each container. (Rule 2)** `client.dll` and `server.dll` each own their **entity list**: a `World_t` (EnTT registry of entity ids + components, plus the `netId → entity` map) held by the app (`m_world`), and their own **network manager** that holds the connection(s): `CNetClient` (client: one server connection) / `CNetServer` (server: all player connections). The entity list is not engine: it lives in the containers (the type comes from `game_shared`). The client renders **everything** in the world from its entity list (`Transform_t` + `Visual_t` entities through `scene::draw`); nothing is drawn from outside it.
- **One renderer interface:** `IRenderer` (`Renderer001`). There is no `IRenderer002`/`IRenderer003` and there never will be a second one. Any file you touch must use the current renderer format described in "Camera and rendering" below.

## Source layout (separation rules)

Include root is `src/`: `#include "engine/render/render_system.hpp"`, `#include "game/shared/world.hpp"`.

```
src/
  engine/            ENGINE ROOT. Reusable game engine. Never contains game rules, never includes game/, client/, server/, host/.
    engine/          engine.dll: core (mesh types, tick constants, camera, primitives, font, config, launch options, asset path, ray), net/gzip
    interface/       `interface` static lib: CreateInterface registry, CModule loader, every engine interface (i_*.hpp), render_types.hpp
    log/ platform/ render/ particles/ assets/ audio/ ui/ physics/ network/
                     each one its own DLL behind an interface (log.dll, platform.dll, ... network.dll)
    client/ server/  (create only when needed) engine features that are client-side / server-side only. Still no game rules.
  game/              GAME RULES (Call-of-Duty-like shooter). No engine code lives here; it only uses the engine.
    shared/          rules used by both sides: components, World_t, replication, protocol (proto), level, trace, movement, projectile math
    server/          server-side rules: process input commands, weapon, projectiles, damage, world tick
    client/          client-side rules, one nested folder per system: input/ prediction/ interpolation/ effects/ camera/ (death cam) ui/ (all game ImGui code: hud, scoreboard, console window)
                     (server rules: game/server/{systems,control}/ + world_tick; see Working rules 1 and 2)
  client/            client.dll = client CONTAINER (startup/main): holds the entity list (World_t), runs the client systems, owns the network manager (`CNetClient`), scene, console. Contains no game rules (those are in game/).
  server/            server.dll = server CONTAINER (startup/main): loads config and map, opens sockets through the engine (`INetwork`), runs the tick loop, registers hooks and routes packets. Contains no game rules.
  host/client/       executable `game`: prepares the environment, loads engine modules + client.dll, binds them and starts it.
  host/server/       executable `dedicated_server`: same for server.dll.
  dummy/ tests/      empty executables / headless tests. Dummies contain no engine code.
```

- **Engine and game are separate.** The engine will be extracted as a standalone game engine, so nothing under `src/engine/` (including `src/engine/engine/`) includes game code. `src/client` and `src/server` are containers, not engine and not rules: game rules (firing, damage, movement, prediction, input commands) always live under `src/game/`.
- **`engine/engine/` vs `engine/`**: `src/engine` is the root of the whole engine; `src/engine/engine` is the engine's own module (`engine.dll`) next to the other modules. Every other module follows the same format (`src/engine/<module>/{api.hpp,module.cpp,...}`).
- **Offline mode (design):** `client.dll` can load `server.dll` itself and run a local server with bots (offline play, like CS2 with bots). The client container therefore depends on the server container's interface only (`IServerApp`), never on server rules.
- **Engine components are independent, open systems** (log, audio, physics, render, platform, assets, particles, ui, network...), each its own DLL behind an interface. The game picks what it uses. Build more components this way instead of growing `engine.dll`.
- **No lag compensation / rewind in this game.** If it is ever needed it lives under `src/game/server/`, never in the engine.
- **Output layout:** `out/<plat>-<arch>-<mode>/engine/` holds the engine DLLs (`engine`, `log`, `platform`, `render`, `physics`, `network`, ...). `out/<plat>-<arch>-<mode>/game/` holds the executables (`game`, `dedicated_server`, tests) and the game DLLs (`client`, `server`). Binaries find the engine through an rpath of `../engine`; hosts load engine modules from `<exeDir>/../engine` and game modules from `<exeDir>`.
- **Nested paths are allowed** but do not overdo it. **No unnecessary abstraction.** More files means longer compile times: add an `hpp`/`cpp` pair only when it is really needed.
- Moving files: move first (`git mv`), then fix includes, then write code. Do not mix a move with behaviour changes.

## Modules

On Windows the output is `engine.dll`; on macOS `libengine.dylib`; on Linux `libengine.so`. On macOS/Linux the `lib` prefix is mandatory: the linker resolves `-lengine` by looking for `libengine.dylib`, so a prefix-less `engine.dylib` is not found. `moduleFileName()` follows the same rule.

| Target | Kind | Contents |
| --- | --- | --- |
| `interface` | **static lib** (`src/engine/interface/`) | CreateInterface registry, `queryInterface<T>`, `CModule` (dlopen/LoadLibrary), engine interface definitions (`IRenderer`, `IParticles`, `IPlatform`, `IAssets`, `IAudio`, `IUI`, `IPhysics`, `INetwork`, `ILog`) + `net::Task` coroutine helpers |
| `engine` | shared (`engine.dll`, `src/engine/engine/`) | `core` (mesh types, tick constants, camera, primitives, font, config, launch options, asset path, ray), `net/gzip`. Used through headers by everything. |
| `game_shared` | **static lib** (`src/game/shared/`) | Rules both sides use: components, `World_t`, replication, protocol (proto + codec), level, trace, movement. Compiled into `client.dll` and `server.dll` (each gets its own copy), never into the engine. |
| `platform` | shared (`platform.dll`) | SDL3 window, event pump, key state by name, typed text → `CPlatform : IPlatform` (`Platform001`). The only place that touches SDL on the input/window side. |
| `render` | shared (`render.dll`) | Backend-independent renderer → `CRenderSystem : IRenderer` (`Renderer001`). Passes: shadow → scene (sky, opaque, alpha-blend) → debug lines → post (bloom, tonemap, vignette) → overlay (Dear ImGui callback onto the swapchain). GPU access goes through `render/rhi/` (`IRhiDevice`; `CSdlGpuDevice` = Metal). Types shared with callers: `interface/render_types.hpp`. |
| `particles` | shared (`particles.dll`) | CPU particle simulation → `CParticleSystem : IParticles` (`Particles001`) |
| `assets` | shared (`assets.dll`) | Assimp model loading + cache → `CAssetSystem : IAssets` (`Assets001`): `loadMesh` (static) and `loadSkinned` (skeleton + skin + clips, `skinned_loader.cpp`) |
| `audio` | shared (`audio.dll`) | miniaudio → `CAudioSystem : IAudio` (`Audio001`). Optional: the client runs silent without it. |
| `ui` | shared (`ui.dll`) | The engine's only UI system: **Dear ImGui** (xmake package `imgui`, configs `sdl3`, `sdl3_gpu`) with the official `imgui_impl_sdl3` + `imgui_impl_sdlgpu3` backends → `CUiSystem : IUI` (`UI001`). Game code writes ImGui directly; drawing is done by the renderer overlay pass. Optional: no console without it. |
| `log` | shared (`log.dll`) | spdlog (async, rotating file + colored console) → `CLogSystem : ILog` (`Log001`, in `engine/interface/i_log.hpp`). Loaded first by both hosts; see "Logging". |
| `physics` | shared (`physics.dll`) | Jolt Physics: static boxes, character capsules, raycast, `setCharacterPosition`, optional character-vs-character collision → `CPhysicsSystem : IPhysics` (`Physics001`). Loaded by the server and by the client (own instance, for prediction). |
| `network` | shared (`network.dll`) | TCP + UDP on **one port** → `CNetworkSystem : INetwork` (`Network001`). See "Networking". Loaded by both hosts. |
| `client` | shared (`client.dll`, `src/client/` + `src/game/client/`) | client container: startup, entity list (`World_t`), client systems (`scene/`), network manager (`CNetClient`), console, client loop → `CClientApp : IClientApp` (`ClientApp001`). Game rules come from `game_shared` + `src/game/client/` (input system, prediction, interpolation). No SDL dependency. |
| `server` | shared (`server.dll`, `src/server/` + `src/game/server/`) | server container: config, map, entity list (`World_t`), network manager (`CNetServer`: connections, packet routing, welcome / view target / snapshot), tick loop → `CServerApp : IServerApp` (`ServerApp001`). Game rules come from `game_shared` + `src/game/server/` (`systems/` input, ai, movement, weapon, projectiles, owners; `control/` pawn command + spectate / take-over rules). |
| `game` | binary (`src/host/client/`) | host: loads `log` first, then `platform, render, particles, assets, audio, ui, physics, network, client`, runs `ClientApp001` |
| `dedicated_server` | binary (`src/host/server/`) | host: loads `log` first, then `physics, network, server`, runs `ServerApp001` |
| `ecs_test` | binary | headless integration test: physics + ECS + replication + protobuf (`xmake run ecs_test`) |
| `anim_test` | binary | headless test: glTF skinned load (Assimp), skin weights, pose evaluation, clip blend (`xmake run anim_test`) |
| `dummy_client`, `dummy_server` | binary | empty executables that only verify the build, output directory and link chain (`src/dummy/`) |

## Interface system (Source-style CreateInterface)

- Every loadable module exports **one** C function: `void* CreateInterface(const char* name, int* returnCode)`. The `GAME_EXPORT_MODULE()` macro generates it; use it once in the module's `module.cpp`.
- Classes register themselves in the module-local registry with `GAME_EXPOSE_INTERFACE(ImplClass, InterfaceType, kVersionName)`. The instance is `static`: no ownership transfer, one object for the lifetime of the module.
- Interface names are versioned and **every interface is at `001`**: `Platform001`, `Renderer001`, `Particles001`, `Assets001`, `Audio001`, `UI001`, `Physics001`, `Network001`, `ClientApp001`, `ServerApp001`, `Log001`. **When an interface changes, change it in place and fix all callers. Do not add a new version next to the old one and do not keep the old one registered.** Before release every interface stays at `001`, and versions change only when the user explicitly asks (see Working rules).
- Hosts (`game`, `dedicated_server`) load modules with `CModule::load` and collect their factories with `registerModuleFactory`. The app module (`client` / `server`) is loaded last.
- The host calls `connect(&game::resolveInterface)` on the app; the app gets what it needs with `queryInterface<IFoo>(factory, kFooInterface)`. **Modules never link each other directly.**
- Loaded modules stay open until process exit (no `dlclose`).
- Interface pointers travel as `void*` inside the factory; `queryInterface<T>` does the `static_cast`.
- To add an engine module: `src/engine/<name>/{api.hpp,module.cpp,...}`, an interface in `src/engine/interface/i_<name>.hpp`, a `target` in `xmake.lua`, and add its name to the host's module list. The `IClientApp` / `IServerApp` interfaces live next to their containers (`src/client/i_client.hpp`, `src/server/i_server.hpp`).
- `engine.dll` is intentionally not an interface: it holds plain shared types and helpers. If it ever needs to be behind an interface, add `IEngine001`.

Dependency direction: `engine/interface` ← all modules. `engine/engine` ← `game_shared`, `client`, `server`, `assets`, `audio`, the hosts. `game_shared` ← `client`, `server`. Direct `#include`s across engine modules are limited to `engine/interface/` and `engine/engine/` headers; `client`/`server` never include each other (except `i_server.hpp` from `client` for offline mode).

## Networking (`network.dll`, `src/engine/network/`)

- Engine component, bytes only (protobuf/gzip stay on the game side). `INetwork`: `listen` (server), `connect` (client), `send(peer, bytes, EDeliver)`, `broadcast`, `pump`, `nextEvent`.
- **One port, two transports:** `EDeliver::kReliable` = TCP (ordered, guaranteed), `kUnreliable` = UDP (may be lost / reordered; payload at most `kMaxUnreliableSize` 1200 bytes, `send` returns false otherwise or before the UDP path is bound).
- **State:** a client owns at most one `Connection_t` (`m_connection` set = connected). The server owns `m_clients`, a map of `Client_t` each holding its `Connection_t`. A connection = TCP socket + buffers + UDP token/address.
- **Handshake:** the server's first TCP frame carries `[peer id][UDP token]`. The client then sends its token over UDP until the server acknowledges it (the server learns the client's UDP address from that datagram). Wire: TCP `[u32 length][data]`, UDP `[u32 token][data]`.
- Events: `kConnected`, `kDisconnected`, `kPacket` (with `peer`, `deliver`, `data`). The client addresses the server as `kServerPeer`. The peer id is the player id.
- **Coroutines:** `engine/interface/network_task.hpp` has `net::Task` (fire-and-forget) and `co_await net::nextEvent(network)`. `pump()` resumes the waiting coroutine when events arrive; one waiter at a time. The server container processes all network events in one such coroutine.
- Game usage: user commands go `kReliable`, snapshots go `kUnreliable` (every snapshot is the full state; falls back to `kReliable` when larger than `kMaxUnreliableSize`).

## Language and modernity

- Language: **C++26**. `set_languages("c++26")`. Older standards (`c++17/20/23` targets) are not allowed.
- Modern C++ is mandatory. Old C++ and C habits are forbidden.
- Forbidden: `NULL`, `new`/`delete` (only where a library's API demands it, e.g. Jolt `Factory`/`Ref<>`, with a comment), C arrays, `printf`, `typedef`, `using namespace` in headers, hand-rolled proto packing loops, `Manager` in names.
- Required: `nullptr`, RAII, `std::unique_ptr` / `std::shared_ptr`, `std::optional`, `std::expected`, `std::span`, `std::array`, `enum class`, `[[nodiscard]]`, `override`, designated initializers, ranges.
- Types that cross a DLL boundary: exported classes and functions are marked with `*_API`. Headers include only their own module's `api.hpp` for export macros.

## Naming

| Kind | Rule | Example |
| --- | --- | --- |
| File / folder | lowercase `snake_case` | `metal_renderer.hpp`, `net_server.cpp` |
| Proto file | lowercase | `proto/game.proto` |
| Class (only if stateful / interface impl) | `C` + PascalCase | `CClientApp`, `CNetServer`, `CPhysicsSystem` |
| Struct (data, components, world) | PascalCase + `_t` suffix | `Transform_t`, `Player_t`, `World_t`, `UserCmd_t` |
| Enum | `E` + PascalCase | `EButton`, `EAction` |
| Enum value | `k` + PascalCase | `kAttack`, `kJump` |
| Member | `m_` + camelCase | `m_simulation`, `m_playerId` |
| Function | camelCase | `applyInput`, `captureSnapshot` |
| Constant | `k` + PascalCase | `kDefaultTickRate`, `kDefaultPort` |
| Namespace | `game`, plus a sub-namespace per function group | `game::world::tick`, `game::systems::movement`, `game::replication::apply` |
| Export macro | `<MODULE>_API` | `ENGINE_API`, `RENDER_API` |
| System class | `C{Domain}System` | `CAssetSystem`, `CParticleSystem` (never `Manager`) |

Header rule: `class_name.hpp` → class `CClassName`. `user_cmd.hpp` → struct `UserCmd_t`.

## ECS

- Every game object (player, projectile, level prop) is an **entity** in `World_t::registry` (EnTT). Components are plain data structs in `game/shared/components.hpp`: `Transform_t`, `Velocity_t`, `Player_t`, `Projectile_t`, `Animator_t`, `Replicated_t`, `Collider_t`, `CommandQueue_t`. Client-only components (`Visual_t`, `Spin_t`, `LocalPlayer_t`) live in `client/scene.hpp`.
- **Systems** are free functions in `game::systems` (`input`, `ai`, `movement`, `animation`, `weapon`, `projectiles`, `death`, `owners`, `projectileTransforms`). `world::tick` runs them in that order on the server, then increments the tick.
- Entity creation/destruction goes through `world::` helpers (`spawnPlayer`, `createReplicated`, `destroy`) so the `netId → entity` map stays correct.
- **Replication:** entities with `Replicated_t{netId}` are sent each tick as `EntitySnapshot_t` (the optional components they own). `replication::capture` (server) / `replication::apply` (client) create, update and delete entities by `netId`. Entities without `Replicated_t` are local to that process (client props).
- To add a replicated component: struct in `components.hpp` → message in `proto/game.proto` → field in `EntitySnapshot_t` → `writeEntity`/`readEntity` in `net/protocol.cpp` → one `copyOf`/`write` line in `ecs/replication.cpp`.
- **Level:** `level::boxes()` is the single list of static boxes. The server adds them to physics (`level::buildPhysics`), the client draws them as entities.

## Pawn / Controller (`src/engine/engine/ecs/control.{hpp,cpp}`)

- Engine-level, no game rules, no input types. **Pawn** (`control::Pawn_t`) = the entity that is controlled. **Controller** (`control::Controller_t`) = what controls it (player, bot); it drives at most one pawn. Both are plain entities in the registry.
- API (`game::control`): `makePawn`, `createController(protectsPawn)`, `possess`, `unpossess`, `pawnOf`, `controllerOf`, `setViewTarget` / `clearViewTarget` / `viewTarget` (spectate), `onDestroy` (called by `world::destroy`).
- **Spectate:** a controller can watch any pawn (`viewTarget`) without driving it. **Take over:** `possess` on a pawn that another controller drives. The displaced controller is remembered (`Pawn_t::displaced`); `unpossess` hands the pawn back to it. `protectsPawn` controllers (players) cannot be taken over.
- Game side: `PlayerController_t{playerId, homePawn}`, `AiController_t` (bot patrol), `CommandQueue_t` (input queue + `current`) live on the **controller**; `Weapon_t` on the pawn. Pawn systems get their command via `control_rules::commandOf(world, pawn)` (controller's command, or idle if nobody drives it). `systems::owners` writes the driving player into the replicated `Player_t::playerId` (0 = bot / nobody), so the client finds its current pawn with `world::findPlayer(playerId)`; this switches by itself on take-over. `Projectile_t::ownerId` is the shooter pawn's **netId**.
- Rules (`game/server/control/control_request.cpp`, `world::handleControl`): `kSpectate <netId>` (releases own pawn), `kTakeOver` (only the watched pawn, only if an AI drives it), `kRelease` (back to `homePawn`). Wire: client → server `TYPE_CONTROL` (`ControlRequest`), server → client `TYPE_VIEW_TARGET` (`view_target_net_id`, 0 = not spectating). Console: `pawns`, `spectate <netId>`, `takeover`, `release`. Server config `bots.count` spawns bots at start.
- Camera: own pawn → first person with prediction; no pawn and no watched pawn (dead) → `death_cam` (`game/client/camera/`): slow orbit high above the map around the living characters; otherwise the watched pawn's eye and view angles (`CClientApp::watchedEntity`). The pawn the camera is inside is not drawn (`scene::draw(hidden)`).

## Physics

- `physics.dll` (Jolt) runs on the server (authoritative) and on the client (its own instance, only to predict the local player's movement). Players are `CharacterVirtual` capsules (0.3 radius, 1.8 height, origin at the feet). `systems::movement` applies gravity every tick, calls `IPhysics::moveCharacter` and takes back `position` and `onGround`. Without a physics module it falls back to a flat y=0 ground.
- Projectiles trace from this tick's position to the next each tick (`trace::line`, see "Ray trace") and are destroyed on hit (or after `kProjectileLifetime`).
- Jolt gotcha: `RegisterDefaultAllocator()` must run **before** constructing any Jolt object (`TempAllocatorImpl`, `PhysicsSystem`). That is why `CPhysicsSystem::Impl_t` is created inside `initialize()`.

## Tick model

- Default tick rate: **64** (`kDefaultTickRate`). Change it with `--tick-rate=N` (10–512). The server is authoritative; it sends the tick rate to the client in the welcome packet.
- The client sends one `UserCmd` per tick. `UserCmd_t::tick` is the client's command **sequence number** (starts at 1), not a server tick.
- When a command packet arrives, the server queues it and processes one command per tick. The queue is capped at `kMaxCmdBacklog` (3): older commands are dropped and their edge flags are OR'd into the next one, so a jump/shot is not lost.
- If no packet arrives, the last command's movement is repeated, but the edge flags (`buttonsPressed` / `buttonsReleased`) are cleared.
- The server writes the last processed sequence number into `Player_t::lastCmdSeq` (replicated; the prediction ack).
- **No rewind / lag compensation.** Hits are evaluated against the server's current positions.
- Transport is `network.dll` (see "Networking"): commands over TCP (`kReliable`, `TCP_NODELAY`), snapshots over UDP (`kUnreliable`) on the same port.

### Client-side prediction (`src/game/client/prediction/prediction.{hpp,cpp}`)

- Only the **local player's movement** is predicted (position, velocity, jump, `onGround`). Health, projectiles and other players are not.
- The client runs the same `systems::movePlayer(world, entity, cmd)` the server uses, on its own `physics.dll` instance (`level::buildPhysics`, `Collider_t` attached by `prediction::attach`). Without a physics module it falls back to the flat y=0 ground.
- Per client tick: `prediction::predict` (apply the command it just sent, keep it in `unacked`) → when a snapshot arrives, `replication::apply` overwrites the local entity with the server state → `prediction::reconcile` drops acked commands (`tick <= lastCmdSeq`), moves the physics character to the server position and **replays** the rest.
- The difference between the old predicted position and the replayed one goes into `errorOffset` (camera/visual only) and decays with `exp(-dt / kSmoothTime)` (`prediction::decay`). Errors of `kSnapDistance` (2 m) or more snap instantly.
- Console: `cl_predict 0|1`. Window title shows `err` (length of `errorOffset`).
- Known limits: client and server `CharacterVirtual` internal states are not shared (tiny drift is hidden by `errorOffset`); the client clock is not locked to the server (the backlog cap absorbs drift).

### Snapshot interpolation (`src/game/client/interpolation/interpolation.{hpp,cpp}`)

- `CNetClient::consumeSnapshots()` returns **every** snapshot received since the last call (tick order, old ticks dropped). The newest is applied to the world (create/destroy/health); all go into the interpolation buffer (max `kMaxSnapshots` = 32).
- `renderTick` is the display time in server ticks. Target = newest tick − `max(kMinDelayTicks, ceil(delaySeconds * tickRate))` (default `cl_interp` 0.05 s). Each client tick it advances `1 + kCatchUp * (target − renderTick)`; if it is more than `kJumpThreshold` (3) ticks away it jumps to the target. It never passes the newest tick (**no extrapolation**).
- `interpolation::apply` blends remote players' `Transform_t` between the two snapshots around `renderTick` (`glm::mix` for position, shortest-arc `lerpAngle` for yaw/pitch). **The local player is never interpolated** (prediction owns it).
- Projectiles use the fractional `renderTick`: `systems::projectileTransforms(world, atTick)` / `projectilePosition(projectile, double atTick, dt)` (before `spawnTick` the projectile sits at `origin`). Your own projectile therefore appears about `delay` late.
- Console: `cl_interpolate 0|1`, `cl_interp <seconds 0.0-0.5>`.

## Logging

- Engine component `log.dll` (`src/engine/log/log_system.cpp`, spdlog, async, `overrun_oldest`, flush every 1 s and on `error`). Interface: `engine/interface/i_log.hpp` (`ILog`, `ELogLevel`, `Log001`).
- Use the macros, never `std::cout`/`std::cerr`/`printf`: `GAME_LOG_TRACE/DEBUG/INFO/WARN/ERROR/FATAL("category", "fmt {}", args...)` (std::format syntax; `FATAL` flushes and aborts). Categories are short lowercase names (`net`, `server`, `client`, `combat`, `audio`, `physics`).
- Before the log module is loaded (or if it fails to load) the macros fall back to stderr. The only places allowed to print directly are the host `main`s before logging starts (`std::println`), tests and dummies.
- Auto binding: `GAME_EXPORT_MODULE()` also exports `BindLog(ILog*)`; `CModule::load` calls it with the host's sink, so no module needs wiring. The linked `engine.dll` is bound by the host with `bindEngineLog`. Each DLL has its own copy of the header-only state, which is why binding is required.
- Command line (both hosts): `--log-level=trace|debug|info|warn|error|fatal|off`, `--log-cat=net:trace,render:warn` (per-category override), `--log-file=<path>` (default `out/<plat>-<arch>-<mode>/logs/<name>.log`, rotating 5 MB × 3).

## Ray trace (hit test)

- `engine/engine/core/ray.hpp`: `raySphere`, `rayCapsule` (analytic vertical capsule: cylinder + 2 spheres). Capsule size constants `kCharacterRadius` / `kCharacterHeight` live in `engine/interface/i_physics.hpp` and are shared with Jolt.
- `game::trace::line(const World_t&, const TraceParams_t&) -> TraceHit_t` (`src/game/shared/trace.{hpp,cpp}`): `IPhysics::raycast` for the static world **plus** `rayCapsule` against every `Player_t` entity (Jolt `CharacterVirtual` is invisible to raycasts). Nearest hit wins; `TraceParams_t::ignore` is the shooter. Result kind: `kNone` / `kWorld` / `kPlayer`.
- Projectiles use it each tick; a player hit does `kProjectileDamage` (20), health never goes below 0. `systems::death` (`game/server/systems/death.cpp`, runs after `projectiles`) despawns every pawn at 0 hp: a bot loses its AI controller too; a player keeps the controller/connection but has no pawn (respawn is not implemented). Watchers of the dead pawn lose their view target.. Use `trace::line` for any new hitscan weapon, too.

## Input (UserCmd)

- Movement: `forwardMove` (W=+1, S=-1), `sideMove` (D=+1, A=-1), `upMove` (currently 0). Range -1..1.
- View: `yaw`, `pitch` (radians), clamped by `clampViewAngles`.
- Buttons are **actions** (`EButton`): `kAttack`, `kJump`, `kCrouch`, `kUse`, `kReload`, `kWalk`. Physical keys never appear in the protocol.
- `buttons` → held (bitmask). `buttonsPressed` → started this tick. `buttonsReleased` → released this tick.

### Input system (rebindable)

- `EAction` (`game/client/input/input_map.hpp`) is every thing the player can do: movement, attack, jump, crouch, use, reload, walk, `kFreeCam`, `kConsole`. Game code asks for **actions**, never physical keys.
- `Bindings_t` maps action → key name (SDL names: `"W"`, `"Space"`, `"Left Shift"`, `"`"`, `"Mouse1"`). Defaults and action names live in one table in `game/client/input/input_map.cpp` (`kActions`). **Console default: `` ` `` (the key under Esc).**
- Saved in `assets/config/input.cfg` (`bind <action> <key>` per line); written with defaults on first run, loaded on start, rewritten by the console `bind` command.
- `input::isDown` / `input::wasPressed` read through `IPlatform` (`keyDownByName`, `keyPressedByName`). Only `platform.dll` touches SDL. `CInputSampler` turns actions into `UserCmd_t` buttons (`kButtonActions` table).
- To add an action: `EAction` value → one row in `kActions` → (if it goes to the server) one row in `kButtonActions` in `input/input_sampler.cpp`.

### Developer console

- The `console` action toggles it (`Esc` also closes). It is a draggable, wrapping, scrolling Dear ImGui window with command history (Up/Down), written directly in ImGui in `game/client/ui/console_window.{hpp,cpp}` (state in `ui::Console_t`, owned by `CClientApp`); the command table stays in the container (`client/console/console_commands.cpp`) because it touches `CNetClient`.
- **Focus (mouse lock):** opening the console gives it focus: mouse free, typing goes to the window, game input stops. Clicking **outside** the window (ImGui `WantCaptureMouse` false) locks the mouse back to the game (the window stays visible, drawn with `NoInputs`); `Enter` gives the focus back. `CClientApp::setConsoleFocus` switches `IPlatform::setRelativeMouse` and `CInputSampler::setEnabled`.
- **UI is Dear ImGui only, built into the engine.** `ui.dll` hosts ImGui with the **official** backends `imgui_impl_sdl3` (input) + `imgui_impl_sdlgpu3` (drawing), which use the same `SDL_GPUDevice` as our SDL_GPU renderer backend (`IRenderer::nativeDevice()`, window from `IPlatform::nativeWindow()`). There is no custom UI draw-data bridge, UI shaders or UI pass. Game code writes ImGui calls directly.
- ImGui is a static package linked into both `ui.dll` and `client.dll`; `CClientApp::initUi` shares the context (`IUI::context()` → `ImGui::SetCurrentContext`, `IUI::allocatorFunctions` → `ImGui::SetAllocatorFunctions`).
- Flow per frame: `IPlatform::pollEvents` passes every `SDL_Event` to the hook set with `setEventHook` (→ `IUI::processEvent`) → `IUI::beginFrame` → client ImGui code, drawn bottom to top: `ui::drawHud` (crosshair, health) → `ui::drawScoreboard` (`scoreboard` action, default Tab, held) → `ui::draw` (console) → `IUI::endFrame` (`ImGui::Render`). Drawing happens inside `IRenderer::endFrame` through `COverlayPass` (`render/passes/overlay_pass.cpp`): the renderer calls `RenderOverlayFn` (set once with `IRenderer::setOverlay(ui->overlay(), ui->overlayUser())`) first outside a render pass (`ImGui_ImplSDLGPU3_PrepareDrawData`), then inside the swapchain pass (`RenderDrawData`). Enter inside the window returns the line from `console::draw`; the client runs it in `client/console/console_commands.cpp` (`help`, `clear`, `echo`, `binds`, `bind <action> <key>`, `sensitivity <v>`, `pawns`, `spectate`, `takeover`, `release`, `quit`). Add a command = one `else if` + one function there.

## Server config

- `assets/config/server.cfg`: `key value` per line, `#` comments. Loaded by `CServerApp::connect` (`config::load`, `config::getBool`).
- `robots.count` (default 1): animated robots spawned at start (`world::spawnAnimated`).
- `player.collisions.enabled` (1/0, default 1): players block each other (Jolt `CharacterVsCharacterCollisionSimple` via `IPhysics::setCharacterCollisions`).

## Skeletal animation (server-driven)

- **Split:** the engine only knows *data and math*; the game decides *which animation plays*.
  - Engine (`engine/engine/anim/skinned_model.{hpp,cpp}`): `SkinnedModel_t` (vertices with 4 joints + weights, node tree, `Joint_t` = node + inverse bind, `Clip_t` keyframes), `anim::findClip`, `anim::evaluate(model, from, to, blend, skin)` (samples both clips, blends at local TRS level, resolves the hierarchy, writes one skin matrix per joint). Clips loop. `assets.dll` loads glTF/FBX through `IAssets::loadSkinned`.
  - Game: **server** `systems::animation` (`game/server/systems/animation.cpp`, runs after `movement`) is the "brain": the animated entity turns toward the nearest player within `kLookRadius` and plays `kGreet` (robot: wave, dancer: breakdance) inside `kGreetEnterRadius` (leaves at `kGreetExitRadius`, hysteresis), else `kIdle`. It only writes `Animator_t{model, state, startTick, prevState, prevStartTick}` when the state changes.
- **Replication:** `Animator_t` is a normal replicated component (proto `Animator`). **No pose is sent.** The client computes the pose from `(displayTick - startTick) * dt` (`interpolation::displayTick`), blending from `prevState` over `kAnimBlendSeconds` (`client/scene/skinned.cpp`, `scene::drawSkinned`). Same idea as projectiles: replicate the start, derive the rest.
- Client maps `EAnimState` → clip by name (`idle` + a per-model greet clip name; missing names fall back to the first clip) in `scene::uploadSkinned`; `EModel` → `SkinnedAsset_t` in `scene::Resources_t`. Spawn on the server: `world::spawnAnimated(world, EModel, position)`.
- **Renderer:** `EVertexLayout::kPosNormalUvSkin` (52 bytes: pos, normal, uv, ubyte4 joints, float4 weights = `anim::SkinnedVertex_t`). `DrawItem_t::bones` takes the skin matrices (max `kMaxBones` = 128, the uniform block is 8 KB); `submit()` copies them. Skinned meshes use `vertex_scene_skinned` / `vertex_shadow_skinned` (bone block = vertex buffer slot 2), same fragment shaders and materials as static meshes (material = `kRimLit` etc., colour/texture in the material). Culling radius gets extra margin for skinned meshes.
- Model: `assets/models/robot.glb` (11 joints, `idle` 2.0 s, `wave` 1.6 s). It was generated by a script (boxes with blended skin weights at the elbows/knees), faces -Z, feet at y=0, units in metres. Real character: `assets/models/dancer/` (directory mode of `loadSkinned`: `character.glb` = mesh + skeleton + base color texture, every other model file in the folder = one extra clip named by its file stem, e.g. `idle.glb`; bone names must match). Currently the Mixamo character with its embedded "Breakdance Swipes" clip (see `CREDITS.txt`). All models are GLB only (convert with `assimp export in.fbx out.glb`). The client auto-fits it to 1.8 m (`fitSkinned`, bind pose) and applies `yawOffset` (π: Mixamo/glTF face +Z). Server config `dancers.count` (default 1). To add a model: put a GLB (or a folder as above) with ≤ 128 joints and clips under `assets/models/`, add an `EModel` value, one `SkinnedAsset_t` in `Resources_t`, one `case` in `assetFor` (`skinned.cpp`).
- To add an animation state: `EAnimState` value → clip name in `uploadSkinned` → choose it in `systems::animation` (extend `chooseState`).

## Projectiles

- A projectile's position is not updated every tick. Its initial state is stored: `origin`, `velocity`, `spawnTick`.
- Position is computed by projection: `pos(t) = origin + velocity * (t - spawnTick) * dt` (`systems::projectilePosition`). The client system `systems::projectileTransforms(world, atTick)` writes it into `Transform_t` each frame (`atTick` is the interpolation `renderTick`).
- The server destroys the entity when its lifetime expires or its path hits the physics world.

## Wire format

- Messages: **protobuf only** (`proto/game.proto`). No hand-written binary packing.
- Pipeline: proto serialize → **gzip** → 4-byte little-endian length prefix + payload.
- Envelope: `netproto::Envelope` (`TYPE_HELLO`, `TYPE_WELCOME`, `TYPE_USER_CMD`, `TYPE_SNAPSHOT`, `TYPE_DISCONNECT`).
- `UserCmd.tick` = client command sequence; `Player.last_cmd_seq` = last sequence the server processed (prediction ack).
- Snapshot: `WorldSnapshot` = tick, tick rate, `repeated Entity` (`net_id` + optional component messages). Encode/decode: `protocol::encode` / `protocol::decode`.

## Runtime flow

- Server: `INetwork::pump` (the `serveNetwork` coroutine handles connect/disconnect/command events) → `world::tick` → `replication::capture` + `broadcastSnapshot` (UDP) → sleep until the next fixed tick.
- Client: platform events → sample input → `sendCommand` → `prediction::predict` → `consumeSnapshots` → `replication::apply` (newest) + buffer all → `prediction::reconcile` → `prediction::decay` → `interpolation::advance` + `interpolation::apply` → client systems (`attachVisuals`, `spin`, `projectileTransforms`) → camera (+ `errorOffset`) → `scene::draw`.
- Server containers never run in the client and vice versa (except the planned offline mode, where `client.dll` loads `server.dll` through `IServerApp`). They talk over `network.dll`; shared rules come from `game_shared`.

## Camera and rendering

- `CCamera` (engine): yaw/pitch clamped by `clampViewAngles` (pitch ±89°). Projection uses `perspectiveRH_ZO` (Metal depth range 0..1); view uses `lookAtRH`.
- Direction convention: yaw=0 looks down -Z, right is +X. Simulation, camera and input all use this convention.
- Client camera: normally at the server-side player's eye height (`kEyeHeight`). Pressing **F** toggles free-cam: movement is local only, and neither movement nor buttons are sent to the server; only the view angles are sent.
- **Renderer format (the only one):** resources are **handles** (`MeshHandle_t`, `TextureHandle_t`, `MaterialHandle_t`; index + generation, invalid = default-constructed). Colour and shading live in the **material** (`MaterialDesc_t`: `kLit` / `kRimLit` / `kUnlit`, `kOpaque` / `kAlphaBlend`, `baseColor` sRGB, `emissive`, optional `baseColorTexture`), not in the draw call. Setup: `initialize(RendererDesc_t{.surface = {kSdlWindow, nativeWindow}})` → `createMesh` (`makeMeshDesc<Vertex_t, uint32_t>`) / `createTexture` / `createMaterial` at load time.
- Per frame: `beginFrame(FrameDesc_t)` → `setView({view, projection})` → `setEnvironment` / `setPostProcess` → `submit(DrawItem_t{mesh, material, transform, castShadow})`* / `submitLight` / `submitDebugLine` → `endFrame()`. Everything is recorded and drawn in `endFrame`. Invalid or destroyed handles are silently skipped. `lastError()` explains a failed `initialize`/`create*`.
- Client side: `scene::Resources_t` holds the loaded meshes and shared materials (`player` = `kRimLit`, `projectile`/`particle` = `kUnlit` + emissive); `Visual_t{mesh, material, local}` is the per-entity component; `scene::draw` submits every `Transform_t` + `Visual_t` entity.
- World-space text (name tags etc.): `engine/engine/core/font.hpp` (`font::load` → R8 atlas, `font::buildTextMesh`, `font::billboardModel`) + a `kUnlit` / `kAlphaBlend` material with `baseColorTexture` = the atlas. Alpha-blend draws are depth-tested but do not write depth, so walls hide the text.
- Shaders: MSL source lives in `render/shaders/msl_source.cpp` and is compiled at runtime. Vertex layout is `Vertex_t` (32 bytes: position, normal, uv). Passes: shadow (D32 depth only) → scene (RGBA16F HDR + D32 z-buffer, LESS test, sky / opaque / alpha) → post (bloom, ACES, vignette, gamma) → overlay (`setOverlay` callback, Dear ImGui onto the swapchain).
- Culling is disabled (so the result does not depend on triangle winding). The depth/HDR targets are recreated when the swapchain size changes.
- Models: `assets/models/` has `spot.glb` (CC0, Keenan Crane) and `suzanne.glb` (Blender). Load through `IAssets::loadMesh("models/<file>")`; `findAssetPath` walks up from the working directory to find `assets/`.

## Libraries and toolchain

- xmake, SDL3 (`libsdl3`), GLM, EnTT (ECS, stays), Jolt Physics (`joltphysics`), miniaudio, Assimp, protobuf-cpp, spdlog (logging, only inside `log.dll`), zlib (system).
- No ready library exists for prediction/interpolation (ENet, yojimbo and GameNetworkingSockets are transport only), so they are our own small code on top of glm.
- **protobuf version: 36.2** (`add_requires("protobuf-cpp 36.2")`). The local `protoc` must also be 36.2: `protoc --version` → `libprotoc 36.2`. With Homebrew: `brew link --overwrite protobuf`. When changing the version, update xmake and the local protoc together.
- **Assimp**: the xmake recipe pulls in minizip, which fails on the zlib CMake config lookup, so Assimp comes from Homebrew: `brew install assimp`. `add_requires("assimp", {system = true})` finds it through pkg-config.
- **zlib**: `add_requires("zlib", {system = true})` (macOS SDK).
- Apple: SDL_GPU driver `"metal"`, shader format MSL.

## Build

Output directory: `out/<plat>-<arch>-<mode>/` (e.g. `out/macosx-arm64-debug/`) with `engine/` (engine DLLs) and `game/` (executables + game DLLs) subfolders, plus `logs/`. Intermediate files stay in xmake's `build/<plat>/<arch>/<mode>/` layout. `.gitignore` ignores `out/` and binary extensions.

```
xmake f -m debug        # or -m release
xmake build
xmake run dedicated_server --tick-rate=64
xmake run game --host=127.0.0.1 --port=7777
xmake run ecs_test      # headless integration test, must print ALL TESTS PASSED
xmake run anim_test     # skinned model load + pose math, must print ALL TESTS PASSED
xmake run network_test  # network.dll: TCP+UDP same port, handshake, reliable/unreliable, coroutine
xmake run dummy_client
xmake run dummy_server
```
