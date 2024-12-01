# Naive-Audio-Freeze-Frame

## About

A naïve approach to freeze framing audio in JUCE. A GUI prompts a user to open and play an audio file. Upon pressing <kbd>Freeze</kbd>, a rectangular window of size `freezeDuration` will be looped, centered at the position that the user froze. Upon unfreezing (pressing <kbd>Play</kbd> again), the audio ceases looping and continues playback. This literally conveys what is happening at some small slice in time, but the rectangular windowing and looping leads to clicks every time we loop back.
