#include "engine/engine/net/gzip.hpp"

#include <array>

#include <zlib.h>

namespace game::gzip {
namespace {

constexpr int kGzipWindowBits = 15 + 16;
constexpr size_t kChunkSize = 16 * 1024;

}  // namespace

std::expected<std::vector<uint8_t>, std::string> compress(std::span<const uint8_t> input) {
    z_stream stream{};
    if (deflateInit2(&stream, Z_BEST_SPEED, Z_DEFLATED, kGzipWindowBits, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return std::unexpected("gzip deflateInit2 failed");
    }

    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.data()));
    stream.avail_in = static_cast<uInt>(input.size());

    std::vector<uint8_t> output;
    std::array<uint8_t, kChunkSize> chunk{};
    int status = Z_OK;
    do {
        stream.next_out = chunk.data();
        stream.avail_out = static_cast<uInt>(chunk.size());
        status = deflate(&stream, Z_FINISH);
        const size_t produced = chunk.size() - stream.avail_out;
        output.insert(output.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(produced));
    } while (status == Z_OK);

    deflateEnd(&stream);
    if (status != Z_STREAM_END) {
        return std::unexpected("gzip compress failed");
    }
    return output;
}

std::expected<std::vector<uint8_t>, std::string> decompress(std::span<const uint8_t> input) {
    z_stream stream{};
    if (inflateInit2(&stream, kGzipWindowBits) != Z_OK) {
        return std::unexpected("gzip inflateInit2 failed");
    }

    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.data()));
    stream.avail_in = static_cast<uInt>(input.size());

    std::vector<uint8_t> output;
    std::array<uint8_t, kChunkSize> chunk{};
    int status = Z_OK;
    do {
        stream.next_out = chunk.data();
        stream.avail_out = static_cast<uInt>(chunk.size());
        status = inflate(&stream, Z_NO_FLUSH);
        if (status != Z_OK && status != Z_STREAM_END) {
            inflateEnd(&stream);
            return std::unexpected("gzip decompress failed");
        }
        const size_t produced = chunk.size() - stream.avail_out;
        output.insert(output.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(produced));
    } while (status != Z_STREAM_END);

    inflateEnd(&stream);
    return output;
}

}  // namespace game::gzip
