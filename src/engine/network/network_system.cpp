#include "engine/network/network_system.hpp"

#include "engine/interface/i_log.hpp"

#include <array>
#include <random>
#include <string>
#include <tuple>
#include <utility>

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using SockLen_t = int;
#else
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <sys/socket.h>
    #include <unistd.h>
    using SockLen_t = socklen_t;
#endif

namespace game {
namespace {

constexpr uint32_t kMaxFrameSize = 16u * 1024u * 1024u;
constexpr std::size_t kMaxOutBuffer = 8u * 1024u * 1024u;
constexpr std::size_t kTokenSize = 4;
constexpr std::size_t kHandshakeSize = 8;  // peer id + token
constexpr auto kBindInterval = std::chrono::milliseconds(100);

#if defined(_WIN32)
void closeFd(int fd) {
    if (fd >= 0) {
        closesocket(fd);
    }
}

bool wouldBlock() {
    const int code = WSAGetLastError();
    return code == WSAEWOULDBLOCK || code == WSAEINPROGRESS;
}

void ensureWsa() {
    static const bool ready = [] {
        WSADATA data;
        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    std::ignore = ready;
}

void setNonBlocking(int fd) {
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
}

void setNoSigpipe(int) {}
constexpr int kSendFlags = 0;
#else
void closeFd(int fd) {
    if (fd >= 0) {
        ::close(fd);
    }
}

bool wouldBlock() {
    return errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS;
}

void ensureWsa() {}

void setNonBlocking(int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

// Avoid dying from SIGPIPE when writing to a closed connection: macOS socket option, Linux send flag.
void setNoSigpipe([[maybe_unused]] int fd) {
    #if defined(SO_NOSIGPIPE)
    const int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
    #endif
}
    #if defined(MSG_NOSIGNAL)
constexpr int kSendFlags = MSG_NOSIGNAL;
    #else
constexpr int kSendFlags = 0;
    #endif
#endif

// Don't let small packets wait because of Nagle's algorithm.
void setNoDelay(int fd) {
    const int yes = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&yes), sizeof(yes));
}

[[nodiscard]] CSocketFd openSocket(int type, int protocol) {
    ensureWsa();
    return CSocketFd(static_cast<int>(::socket(AF_INET, type, protocol)));
}

[[nodiscard]] bool bindPort(int fd, uint16_t port) {
    const int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    return ::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

void putU32(std::vector<uint8_t>& out, uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<uint8_t>((value >> shift) & 0xFFu));
    }
}

[[nodiscard]] uint32_t getU32(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) | (static_cast<uint32_t>(bytes[2]) << 16) |
           (static_cast<uint32_t>(bytes[3]) << 24);
}

}  // namespace

// --- CSocketFd ---

CSocketFd& CSocketFd::operator=(CSocketFd&& other) noexcept {
    if (this != &other) {
        reset();
        m_fd = other.release();
    }
    return *this;
}

CSocketFd::~CSocketFd() {
    reset();
}

int CSocketFd::release() {
    return std::exchange(m_fd, -1);
}

void CSocketFd::reset() {
    closeFd(m_fd);
    m_fd = -1;
}

// --- Start / shutdown ---

CNetworkSystem::~CNetworkSystem() = default;

bool CNetworkSystem::listen(uint16_t port) {
    shutdown();
    CSocketFd tcp = openSocket(SOCK_STREAM, IPPROTO_TCP);
    CSocketFd udp = openSocket(SOCK_DGRAM, IPPROTO_UDP);
    const bool ok = tcp.valid() && udp.valid() && bindPort(tcp.get(), port) && bindPort(udp.get(), port) && ::listen(tcp.get(), 64) == 0;
    if (!ok) {
        GAME_LOG_ERROR("net", "could not open port {} (TCP+UDP)", port);
        return false;
    }
    setNonBlocking(tcp.get());
    setNonBlocking(udp.get());
    m_listener = std::move(tcp);
    m_udp = std::move(udp);
    GAME_LOG_INFO("net", "listening: port {} (TCP+UDP)", port);
    return true;
}

bool CNetworkSystem::connect(const std::string& host, uint16_t port) {
    shutdown();
    ensureWsa();

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result = nullptr;
    const std::string portText = std::to_string(port);
    if (getaddrinfo(host.c_str(), portText.c_str(), &hints, &result) != 0 || result == nullptr) {
        GAME_LOG_ERROR("net", "could not resolve address: {}", host);
        return false;
    }

    // UDP is connected too: only datagrams from the server are read, and send() does not need an address.
    CSocketFd tcp = openSocket(SOCK_STREAM, IPPROTO_TCP);
    CSocketFd udp = openSocket(SOCK_DGRAM, IPPROTO_UDP);
    const auto length = static_cast<SockLen_t>(result->ai_addrlen);
    const bool ok = tcp.valid() && udp.valid() && ::connect(tcp.get(), result->ai_addr, length) == 0 &&
                    ::connect(udp.get(), result->ai_addr, length) == 0;
    freeaddrinfo(result);
    if (!ok) {
        GAME_LOG_ERROR("net", "could not connect to {}:{}", host, port);
        return false;
    }

    setNoDelay(tcp.get());
    setNoSigpipe(tcp.get());
    setNonBlocking(tcp.get());
    setNonBlocking(udp.get());
    m_connection.emplace();
    m_connection->tcp = std::move(tcp);
    m_udp = std::move(udp);
    GAME_LOG_INFO("net", "connected: {}:{} (waiting for handshake)", host, port);
    return true;
}

void CNetworkSystem::shutdown() {
    m_connection.reset();
    m_clients.clear();
    m_tokens.clear();
    m_listener.reset();
    m_udp.reset();
    m_events.clear();
    m_waiter = {};
    m_localPeer = 0;
    m_nextPeer = 1;
}

void CNetworkSystem::disconnect(PeerId_t peer) {
    if (Connection_t* connection = findConnection(peer)) {
        std::ignore = flush(*connection);  // try to send pending data
        peer == kServerPeer ? dropConnection() : dropClient(peer);
    }
}

Connection_t* CNetworkSystem::findConnection(PeerId_t peer) {
    if (peer == kServerPeer) {
        return m_connection ? &*m_connection : nullptr;
    }
    const auto it = m_clients.find(peer);
    return it != m_clients.end() ? &it->second.connection : nullptr;
}

// --- Sending ---

bool CNetworkSystem::send(PeerId_t peer, std::span<const uint8_t> data, EDeliver deliver) {
    Connection_t* connection = findConnection(peer);
    if (connection == nullptr || connection->broken) {
        return false;
    }
    return deliver == EDeliver::kReliable ? sendReliable(*connection, data) : sendUnreliable(*connection, data);
}

void CNetworkSystem::broadcast(std::span<const uint8_t> data, EDeliver deliver) {
    for (const auto& [id, client] : m_clients) {
        std::ignore = send(id, data, deliver);
    }
}

bool CNetworkSystem::sendReliable(Connection_t& connection, std::span<const uint8_t> data) {
    if (data.size() > kMaxFrameSize) {
        return false;
    }
    if (connection.out.size() + kTokenSize + data.size() > kMaxOutBuffer) {
        connection.broken = true;  // the remote end is not reading
        return false;
    }
    putU32(connection.out, static_cast<uint32_t>(data.size()));
    connection.out.insert(connection.out.end(), data.begin(), data.end());
    connection.broken = !flush(connection);
    return !connection.broken;
}

bool CNetworkSystem::sendUnreliable(const Connection_t& connection, std::span<const uint8_t> data) {
    if (!connection.udpBound || data.size() > kMaxUnreliableSize) {
        return false;
    }
    return writeDatagram(connection, data);
}

bool CNetworkSystem::writeDatagram(const Connection_t& connection, std::span<const uint8_t> payload) {
    if (!m_udp.valid()) {
        return false;
    }
    std::vector<uint8_t> packet;
    packet.reserve(kTokenSize + payload.size());
    putU32(packet, connection.token);
    packet.insert(packet.end(), payload.begin(), payload.end());

    const char* bytes = reinterpret_cast<const char*>(packet.data());
    if (m_listener.valid()) {
        sockaddr_in to{};
        to.sin_family = AF_INET;
        to.sin_addr.s_addr = connection.udpAddress;
        to.sin_port = connection.udpPort;
        return ::sendto(m_udp.get(), bytes, packet.size(), 0, reinterpret_cast<sockaddr*>(&to), sizeof(to)) >= 0;
    }
    return ::send(m_udp.get(), bytes, packet.size(), 0) >= 0;
}

bool CNetworkSystem::flush(Connection_t& connection) {
    std::size_t sent = 0;
    bool ok = true;
    while (sent < connection.out.size()) {
        const auto n = ::send(connection.tcp.get(), reinterpret_cast<const char*>(connection.out.data() + sent),
                              connection.out.size() - sent, kSendFlags);
        if (n > 0) {
            sent += static_cast<std::size_t>(n);
        } else {
            ok = wouldBlock();  // full: continue on the next pump()
            break;
        }
    }
    connection.out.erase(connection.out.begin(), connection.out.begin() + static_cast<std::ptrdiff_t>(sent));
    return ok;
}

// --- Every tick ---

void CNetworkSystem::pump() {
    acceptClients();

    std::vector<PeerId_t> dead;
    for (auto& [id, client] : m_clients) {
        if (!service(id, client.connection)) {
            dead.push_back(id);
        }
    }
    if (m_connection && !service(kServerPeer, *m_connection)) {
        dropConnection();
    }

    readUdp();
    sendBindRequest();
    for (const PeerId_t id : dead) {
        dropClient(id);
    }
    wakeWaiter();
}

bool CNetworkSystem::service(PeerId_t peer, Connection_t& connection) {
    return readTcp(peer, connection) && flush(connection) && !connection.broken;
}

void CNetworkSystem::acceptClients() {
    if (!m_listener.valid()) {
        return;
    }
    while (true) {
        const int fd = static_cast<int>(::accept(m_listener.get(), nullptr, nullptr));
        if (fd < 0) {
            break;
        }
        Client_t client;
        client.id = m_nextPeer++;
        client.connection.tcp = CSocketFd(fd);
        client.connection.token = makeToken();
        setNoDelay(fd);
        setNoSigpipe(fd);
        setNonBlocking(fd);

        // First TCP frame: peer id + UDP token.
        std::vector<uint8_t> hello;
        putU32(hello, client.id);
        putU32(hello, client.connection.token);
        std::ignore = sendReliable(client.connection, hello);

        const PeerId_t id = client.id;
        m_tokens[client.connection.token] = id;
        m_clients.emplace(id, std::move(client));
        pushEvent({.type = ENetEvent::kConnected, .peer = id});
        GAME_LOG_INFO("net", "client {} connected", id);
    }
}

bool CNetworkSystem::readTcp(PeerId_t peer, Connection_t& connection) {
    std::array<uint8_t, 4096> chunk{};
    bool open = true;
    while (true) {
        const auto n = ::recv(connection.tcp.get(), reinterpret_cast<char*>(chunk.data()), chunk.size(), 0);
        if (n > 0) {
            connection.in.insert(connection.in.end(), chunk.begin(), chunk.begin() + n);
            continue;
        }
        open = n < 0 && wouldBlock();  // 0: the remote end closed the connection
        break;
    }
    return extractFrames(peer, connection) && open;
}

bool CNetworkSystem::extractFrames(PeerId_t peer, Connection_t& connection) {
    std::size_t offset = 0;
    while (connection.in.size() - offset >= kTokenSize) {
        const uint32_t size = getU32(connection.in.data() + offset);
        if (size > kMaxFrameSize) {
            return false;
        }
        if (connection.in.size() - offset - kTokenSize < size) {
            break;
        }
        onFrame(peer, connection, {connection.in.data() + offset + kTokenSize, size});
        offset += kTokenSize + size;
    }
    connection.in.erase(connection.in.begin(), connection.in.begin() + static_cast<std::ptrdiff_t>(offset));
    return true;
}

void CNetworkSystem::onFrame(PeerId_t peer, Connection_t& connection, std::span<const uint8_t> frame) {
    if (peer == kServerPeer && m_localPeer == 0) {  // client: the server's handshake frame
        if (frame.size() != kHandshakeSize) {
            connection.broken = true;
            return;
        }
        m_localPeer = getU32(frame.data());
        connection.token = getU32(frame.data() + kTokenSize);
        pushEvent({.type = ENetEvent::kConnected, .peer = kServerPeer});
        return;
    }
    pushEvent({.type = ENetEvent::kPacket, .peer = peer, .deliver = EDeliver::kReliable, .data = {frame.begin(), frame.end()}});
}

void CNetworkSystem::readUdp() {
    if (!m_udp.valid()) {
        return;
    }
    std::array<uint8_t, 2048> buffer{};
    while (true) {
        sockaddr_in from{};
        SockLen_t length = sizeof(from);
        const auto n = ::recvfrom(m_udp.get(), reinterpret_cast<char*>(buffer.data()), buffer.size(), 0,
                                  reinterpret_cast<sockaddr*>(&from), &length);
        if (n < 0) {
            break;
        }
        if (static_cast<std::size_t>(n) >= kTokenSize) {
            onDatagram(getU32(buffer.data()), {buffer.data() + kTokenSize, static_cast<std::size_t>(n) - kTokenSize},
                       from.sin_addr.s_addr, from.sin_port);
        }
    }
}

void CNetworkSystem::onDatagram(uint32_t token, std::span<const uint8_t> payload, uint32_t fromAddress, uint16_t fromPort) {
    const bool isServer = m_listener.valid();
    PeerId_t peer = kServerPeer;
    Connection_t* connection = nullptr;
    if (isServer) {
        const auto tokenIt = m_tokens.find(token);
        const auto clientIt = tokenIt != m_tokens.end() ? m_clients.find(tokenIt->second) : m_clients.end();
        if (clientIt != m_clients.end()) {
            peer = clientIt->first;
            connection = &clientIt->second.connection;
        }
    } else if (m_connection && m_localPeer != 0 && m_connection->token == token) {
        connection = &*m_connection;
    }
    if (connection == nullptr) {
        return;  // unknown token: ignore
    }

    if (payload.empty()) {  // bind request (to the server) or acknowledgement (to the client)
        if (isServer) {
            connection->udpAddress = fromAddress;
            connection->udpPort = fromPort;
            std::ignore = writeDatagram(*connection, {});
        }
        connection->udpBound = true;
        return;
    }
    const bool fromBoundAddress = !isServer || (fromAddress == connection->udpAddress && fromPort == connection->udpPort);
    if (connection->udpBound && fromBoundAddress) {
        pushEvent({.type = ENetEvent::kPacket, .peer = peer, .deliver = EDeliver::kUnreliable, .data = {payload.begin(), payload.end()}});
    }
}

void CNetworkSystem::sendBindRequest() {
    if (!m_connection || m_localPeer == 0 || m_connection->udpBound) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (now - m_lastBindRequest < kBindInterval) {
        return;  // UDP can get lost: repeat periodically until the acknowledgement arrives
    }
    m_lastBindRequest = now;
    std::ignore = writeDatagram(*m_connection, {});
}

// --- Events ---

void CNetworkSystem::dropClient(PeerId_t id) {
    const auto it = m_clients.find(id);
    if (it == m_clients.end()) {
        return;
    }
    m_tokens.erase(it->second.connection.token);
    m_clients.erase(it);
    pushEvent({.type = ENetEvent::kDisconnected, .peer = id});
    GAME_LOG_INFO("net", "client {} left", id);
}

void CNetworkSystem::dropConnection() {
    m_connection.reset();
    m_udp.reset();
    m_localPeer = 0;
    pushEvent({.type = ENetEvent::kDisconnected, .peer = kServerPeer});
    GAME_LOG_INFO("net", "server connection lost");
}

void CNetworkSystem::pushEvent(NetEvent_t event) {
    m_events.push_back(std::move(event));
}

std::optional<NetEvent_t> CNetworkSystem::nextEvent() {
    if (m_events.empty()) {
        return std::nullopt;
    }
    NetEvent_t event = std::move(m_events.front());
    m_events.pop_front();
    return event;
}

void CNetworkSystem::wakeWaiter() {
    if (m_waiter && !m_events.empty()) {
        std::exchange(m_waiter, {}).resume();
    }
}

uint32_t CNetworkSystem::makeToken() const {
    std::random_device random;
    uint32_t token = 0;
    while (token == 0 || m_tokens.contains(token)) {
        token = random();
    }
    return token;
}

}  // namespace game
