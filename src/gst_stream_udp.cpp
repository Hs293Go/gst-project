#include <csignal>

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include "boost/program_options.hpp"

// gst-launch-1.0 v4l2src device=/dev/video2 !
// 'video/x-raw(memory:NVMM),width=640, height=480, framerate=30/1' !
// videoconvert ! omxh264enc ! rtph264pay config-interval=5 name=pay0 pt=96 !
// udpsink host=192.168.1.31 port=8786 sync=true
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;

        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            g_printerr("Error: %s\n", error->message);
            g_error_free(error);

            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }

    return TRUE;
}

GMainLoop *loop;

void handle_ctrl_c(int s) {
    g_main_loop_quit(loop);
    g_print("Quitting...\n");
}

int main(int argc, const char *argv[]) {
    namespace po = boost::program_options;
    
    po::options_description options(
        "Simple gstreamer launcher for Jetson Nano");

    options.add_options()("help", "Display help message")(
        "use-usb-camera", po::bool_switch(),
        "Set this option to use USB camera")(
        "use-nvmm", po::bool_switch(),
        "Set this option to use DMA buffer on Jetson Nano")(
        "width,w", po::value<int>()->default_value(640),
        "The width of the image in pixels")(
        "height,h", po::value<int>()->default_value(480),
        "The height of the image in pixels")(
        "device", po::value<std::string>()->default_value("/dev/video0"),
        "Device location")(
        "host", po::value<std::string>()->default_value("127.0.0.1"),
        "The host/IP/Multicast group to send the packets to")(
        "port", po::value<guint>()->default_value(8786),
        "The port to send the packets to");

    po::variables_map result;
    po::store(po::parse_command_line(argc, argv, options), result);
    if (result.count("help")) {
        fmt::print("{}", options);
        return 0;
    }

    bool use_usb_camera = result["use-usb-camera"].as<bool>();
    bool use_nvmm = result["use-nvmm"].as<bool>();
    const int width = result["width"].as<int>();
    const int height = result["height"].as<int>();
    const gchar *source = use_usb_camera ? "v4l2src" : "nvarguscamerasrc";
    const gchar *device = result["device"].as<std::string>().c_str();
    const gchar *hostname = result["host"].as<std::string>().c_str();
    const guint port = result["port"].as<guint>();

    gst_init(nullptr, nullptr);
    loop = g_main_loop_new(NULL, FALSE);

    GstElement *pipe = nullptr;
    GstElement *src = nullptr;
    GstElement *conv = nullptr;
    GstElement *enc = nullptr;
    GstElement *pay = nullptr;
    GstElement *sink = nullptr;

    GstBus *bus;
    guint bus_watch_id;

    pipe = gst_pipeline_new("camera_node");
    src = gst_element_factory_make(source, "src");
    conv = gst_element_factory_make("videoconvert", "conv");
    enc = gst_element_factory_make("omxh264enc", "enc");
    pay = gst_element_factory_make("rtph264pay", "pay");
    sink = gst_element_factory_make("udpsink", "sink");

    if (!pipe || !src || !conv || !enc || !pay || !sink) {
        g_printerr("One element could not be created. Exiting.\n");
        return -1;
    }

    if (use_usb_camera) {
        g_object_set(G_OBJECT(src), "device", device, nullptr);
    }

    const int interval = 5;
    g_object_set(G_OBJECT(pay), "config-interval", interval, "pt", 96, nullptr);

    g_object_set(G_OBJECT(sink), "host", hostname, "port", port, "sync", true,
                 nullptr);

    gst_bin_add_many(GST_BIN(pipe), src, conv, enc, pay, sink, nullptr);
    GstCaps *cap = nullptr;

    const gchar *nvmm = use_nvmm ? "(memory:NVMM)" : "";
    cap = gst_caps_from_string(
        fmt::format("video/x-raw{}, width={}, height={}, framerate=30/1", nvmm,
                    width, height)
            .c_str());

    if (!gst_element_link_filtered(src, conv, cap)) {
        g_printerr("Link failed\n");
        return 1;
    }
    gst_caps_unref(cap);

    if (!gst_element_link_many(conv, enc, pay, sink, nullptr)) {
        g_printerr("Link failed\n");
        return 1;
    }

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = handle_ctrl_c;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;

    sigaction(SIGINT, &sig_int_handler, NULL);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    /* Iterate */
    g_print("Running...\n");
    g_main_loop_run(loop);

    gst_element_set_state(pipe, GST_STATE_NULL);

    g_print("Deleting pipeline\n");
    gst_object_unref(GST_OBJECT(pipe));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);

    return 0;
}
