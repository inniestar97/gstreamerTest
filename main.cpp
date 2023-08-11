#include <iostream>

#include <gst/gst.h>
#include <gst/gstutils.h>
#include <gst/gstelement.h>

#define WIDTH   1920
#define HEIGHT  920

#define GSTCHECK(x) GstCheck(x, __LINE__);

void GstCheck(gboolean returnCode, int line) {
    if (returnCode != TRUE) {
        throw std::runtime_error("GStreamer function call failed at line " + std::to_string(line));
    }
}

int main(int argc, char* argv[]) {

    gst_init(&argc, &argv);

    GstElement* pipeline = gst_pipeline_new("main-pipeline");

    if (!pipeline) {
        g_printerr("pipeline not constructed");
        return -1;
    }

    GstElement* video_udpsrc = gst_element_factory_make("udpsrc", "video_udpsrc");
    GstElement* audio_udpsrc = gst_element_factory_make("udpsrc", "audio_udpsrc");
    g_object_set(video_udpsrc, "port", 13131, NULL);
    g_object_set(audio_udpsrc, "port", 12121, NULL);

    if (!video_udpsrc || !audio_udpsrc) {
        g_printerr("udpsrc not constructed");
        return -1;
    }

    GstElement* h264parse = gst_element_factory_make("h264parse", "h264parse");
    GstElement* avdec_h264 = gst_element_factory_make("avdec_h264", "avdec_h264");
    GstElement* videoscale = gst_element_factory_make("videoscale", "videoscale");

    if (!h264parse || !avdec_h264 || !videoscale) {
        g_printerr("converting video process not complete");
        return -1;
    }

    GstElement* audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    GstElement* audioresample = gst_element_factory_make("audioresample", "audioresample");

    if (!audioconvert || !audioresample) {
        g_printerr("converting audio process not complete");
        return -1;
    }

    GstCaps* audio_init_caps =
        gst_caps_new_simple("audio/x-raw", "format", G_TYPE_STRING, "S16LE", "channels", G_TYPE_INT, 2, "layout", G_TYPE_STRING, "interleaved", "rate", G_TYPE_INT, 44100, NULL);
    GstCaps* video_scale_caps =
        gst_caps_new_simple("video/x-raw", "width", G_TYPE_INT, WIDTH, "height", G_TYPE_INT, HEIGHT, NULL);

    GstElement* audio_init_filter = gst_element_factory_make("capsfilter", "audio_init_caps_filter");
    g_object_set(audio_init_filter, "caps", audio_init_caps, NULL);
    GstElement* video_scale_filter = gst_element_factory_make("capsfilter", "video_scale_caps_filter");
    g_object_set(video_scale_filter, "caps", video_scale_caps, NULL);

    if (!video_scale_filter) {
        g_printerr("video scale filter is not constructed");
        return -1;
    }
    if (!audio_init_filter) {
        g_printerr("audio scale filter is not constructed");
        return -1;
    }

    GstElement* autovideosink = gst_element_factory_make("autovideosink", "autovideosink");
    GstElement* autoaudiosink = gst_element_factory_make("autoaudiosink", "autoaudiosink");

    if (!autovideosink || !autoaudiosink) {
        g_printerr("videosink and audiosink are not constructed");
        return -1;
    }

    gst_bin_add_many(
        GST_BIN(pipeline),
        video_udpsrc, h264parse, avdec_h264, videoscale, video_scale_filter, autovideosink,
        audio_udpsrc, audio_init_filter, audioconvert, audioresample, autoaudiosink,
        NULL
    );

    GSTCHECK(gst_element_link_many(video_udpsrc, h264parse, avdec_h264, videoscale, video_scale_filter, autovideosink, NULL));
    GSTCHECK(gst_element_link_many(audio_udpsrc, audio_init_filter, audioconvert, audioresample, autoaudiosink, NULL));

    gst_caps_unref(video_scale_caps);
    gst_caps_unref(audio_init_caps);

    auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pipeline);
        return -1;
    }

    GstBus* bus = gst_element_get_bus(pipeline);

    gboolean terminate = FALSE;
    do {
        GstMessage* msg =
            gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        /* Parse message */
        if (msg != NULL) {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE (msg)) {
            case GST_MESSAGE_ERROR:
            gst_message_parse_error (msg, &err, &debug_info);
            g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
            g_clear_error (&err);
            g_free (debug_info);
            terminate = TRUE;
            break;
            case GST_MESSAGE_EOS:
            g_print ("End-Of-Stream reached.\n");
            terminate = TRUE;
            break;
            case GST_MESSAGE_STATE_CHANGED:
            /* We are only interested in state-changed messages from the pipeline */
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                g_print ("Pipeline state changed from %s to %s:\n",
                    gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
            }
            break;
            default:
            /* We should not reach here */
            g_printerr ("Unexpected message received.\n");
            break;
        }
        gst_message_unref (msg);
        }
    } while (!terminate);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
