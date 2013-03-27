gst-plugins-lp
==============

Gstreamer Lightweight Playback Plugins

  * Fake Decoder
    Sometimes stream should be decoded just before reaching renderer. 
    Especially, if using a restricted hardware decoder, playbin's structure 
    is not compatible with this obejctive. In this condition, a Fake Decoder
    can be a solution to send decoder back-side of pipeline even using URIDecodebin.
