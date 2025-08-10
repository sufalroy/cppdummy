#include <chrono>
#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <gst/gst.h>

constexpr std::string_view DEFAULT_URI = "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm";

namespace gst {

struct GObjectDeleter
{
  void operator()(void *obj) const noexcept
  {
    if (obj != nullptr) { gst_object_unref(static_cast<GstObject *>(obj)); }
  }
};

struct GMessageDeleter
{
  void operator()(GstMessage *msg) const noexcept
  {
    if (msg != nullptr) { gst_message_unref(msg); }
  }
};

struct GErrorDeleter
{
  void operator()(GError *err) const noexcept
  {
    if (err != nullptr) { g_error_free(err); }
  }
};

struct GCharDeleter
{
  void operator()(gchar *ptr) const noexcept
  {
    if (ptr != nullptr) { g_free(ptr); }
  }
};

using GstElementPtr = std::unique_ptr<GstElement, GObjectDeleter>;
using GstBusPtr = std::unique_ptr<GstBus, GObjectDeleter>;
using GstMessagePtr = std::unique_ptr<GstMessage, GMessageDeleter>;
using GstErrorPtr = std::unique_ptr<GError, GErrorDeleter>;
using GstCharPtr = std::unique_ptr<gchar, GCharDeleter>;

enum class PlaybackResult : std::uint8_t { Success, Error, EndOfStream };

struct PlaybackError
{
  std::string message;
  std::string debug_info;
};

}// namespace gst

class GStreamerPlayer
{
public:
  explicit GStreamerPlayer(std::string_view uri) : uri_(uri) { initialize_pipeline(); }

  ~GStreamerPlayer() noexcept { cleanup(); }

  GStreamerPlayer(const GStreamerPlayer &) = delete;
  GStreamerPlayer &operator=(const GStreamerPlayer &) = delete;
  GStreamerPlayer(GStreamerPlayer &&) = default;
  GStreamerPlayer &operator=(GStreamerPlayer &&) = default;

  [[nodiscard]] bool play() noexcept
  {
    if (pipeline_ == nullptr) {
      std::cerr << "Pipeline not initialized\n";
      return false;
    }

    if (gst_element_set_state(pipeline_.get(), GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
      std::cerr << "Failed to start playback\n";
      return false;
    }

    std::cout << std::format("Playing: {}\n", uri_);
    return true;
  }

  void stop() noexcept
  {
    if (pipeline_ != nullptr) { gst_element_set_state(pipeline_.get(), GST_STATE_NULL); }
  }

  [[nodiscard]] gst::PlaybackResult wait_for_completion() noexcept
  {
    if (bus_ == nullptr) {
      std::cerr << "Bus not initialized\n";
      return gst::PlaybackResult::Error;
    }

    std::cout << "Waiting for playback to complete...\n";

    while (true) {
      auto message = gst::GstMessagePtr{ gst_bus_timed_pop_filtered(bus_.get(), GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS) };

      if (message == nullptr) {
        std::cerr << "Unexpected null message\n";
        return gst::PlaybackResult::Error;
      }

      auto result = handle_message(*message);
      if (result != gst::PlaybackResult::Success) { return result; }
    }
  }

private:
  void initialize_pipeline()
  {
    const std::string pipeline_desc = std::format("playbin uri={}", uri_);

    GError *error = nullptr;
    GstElement *raw_pipeline = gst_parse_launch(pipeline_desc.c_str(), &error);

    if (error != nullptr) {
      const gst::GstErrorPtr err_ptr(error);
      throw std::runtime_error(std::format("Failed to create pipeline: {}", error->message));
    }

    if (raw_pipeline == nullptr) { throw std::runtime_error("Failed to create pipeline"); }

    pipeline_ = gst::GstElementPtr{ raw_pipeline };

    GstBus *raw_bus = gst_element_get_bus(pipeline_.get());
    if (raw_bus == nullptr) { throw std::runtime_error("Failed to get pipeline bus"); }

    bus_ = gst::GstBusPtr{ raw_bus };
  }

  [[nodiscard]] static gst::PlaybackResult handle_message(GstMessage &msg) noexcept
  {
    switch (GST_MESSAGE_TYPE(&msg)) {
    case GST_MESSAGE_ERROR: {
      auto error = parse_error_message(&msg);
      std::cerr << std::format("Error: {} ({})\n", error.message, error.debug_info);
      return gst::PlaybackResult::Error;
    }

    case GST_MESSAGE_EOS:
      std::cout << "End of stream reached\n";
      return gst::PlaybackResult::EndOfStream;

    default:
      break;
    }
    return gst::PlaybackResult::Success;
  }

  [[nodiscard]] static gst::PlaybackError parse_error_message(GstMessage *msg) noexcept
  {
    GError *raw_error = nullptr;
    gchar *raw_debug = nullptr;
    gst_message_parse_error(msg, &raw_error, &raw_debug);

    const gst::GstErrorPtr error{ raw_error };
    const gst::GstCharPtr debug{ raw_debug };

    return gst::PlaybackError{ .message = error != nullptr ? error->message : "Unknown error",
      .debug_info = debug != nullptr ? debug.get() : "" };
  }

  void cleanup() noexcept { stop(); }

  std::string uri_;
  gst::GstElementPtr pipeline_;
  gst::GstBusPtr bus_;
};

int main(int argc, char *argv[])
{
  try {
    gst_init(&argc, &argv);

    guint major = 0;
    guint minor = 0;
    guint micro = 0;
    guint nano = 0;
    gst_version(&major, &minor, &micro, &nano);
    std::cout << std::format("GStreamer version: {}.{}.{}.{}\n", major, minor, micro, nano);

    GStreamerPlayer player(DEFAULT_URI);

    if (!player.play()) {
      std::cerr << "Failed to start playback\n";
      return EXIT_FAILURE;
    }

    auto result = player.wait_for_completion();

    switch (result) {
    case gst::PlaybackResult::EndOfStream:
      std::cout << "Playback finished successfully\n";
      break;
    case gst::PlaybackResult::Error:
      std::cout << "Playback ended with error\n";
      return EXIT_FAILURE;
    case gst::PlaybackResult::Success:
      std::cout << "Unexpected success result\n";
      break;
    }

    return EXIT_SUCCESS;
  } catch (const std::exception &ex) {
    std::cerr << std::format("Exception: {}\n", ex.what());
    return EXIT_FAILURE;
  }
}