#pragma once

#include "juce_audio_basics/juce_AudioSampleBuffer.h"
#include <atomic>
#include <random>
#include <vector>

/**
 * @struct Grain
 * @brief A plain data structure representing a single sonic event.
 *
 * This structure holds all distinct properties for one grain of sound,
 * assigned at the moment of its creation. The AudioEngine will manage a
 * collection of these.
 */
struct Grain
{
    bool isAlive = true; // Flag to mark for cleanup when the grain is finished.
    int id = 0; // Unique identifier for visualization purposes.

    // Core sonic properties
    float pitch = 60.0f; // MIDI note number.
    float pan = 0.0f; // Stereo position from -1.0 (L) to 1.0 (R).
    float amplitude = 0.0f; // Amplitude from 0.0 to 1.0.
    int durationInSamples = 0; // Total lifetime of the grain in audio samples.

    // Playback state
    int ageInSamples = 0; // How many samples this grain has been playing.
    double sourceSamplePosition = 0.0; // The starting position within the source audio file.
};

/**
 * @class StochasticModel
 * @brief Manages the probability distributions that govern grain creation.
 *
 * This class is the core of the "pointillistic" concept. It uses the C++
 * <random> library  to generate properties for new grains based on user-defined
 * parameters. The UI thread will call the 'set' methods, and the audio thread
 * will call the 'generate' methods. Parameters are atomic to ensure thread safety.
 */
class StochasticModel
{
public:
    // This enum will be used by the UI to select the temporal distribution model.
    enum class TemporalDistribution {
        Uniform,
        Poisson // Random
    };

    //==============================================================================
    // Parameter Setters (called by the UI thread)
    //==============================================================================
    void setPitchAndDispersion (float centralPitch, float dispersion) { /* ... */ } //
    void setDurationAndVariation (float averageDurationMs, float variation) { /* ... */ } //
    void setPanAndSpread (float centralPan, float spread) { /* ... */ } //
    void setDensity (float grainsPerSecond) { /* ... */ } //
    void setTemporalDistribution (TemporalDistribution model) { /* ... */ } //

    //==============================================================================
    // Value Generators (called by the AudioEngine thread)
    //==============================================================================

    /** Generates the number of samples to wait before triggering the next grain. */
    int getSamplesUntilNextEvent();

    /** Fills a Grain struct with new, randomized properties based on the current model. */
    void generateNewGrain (Grain& newGrain);

private:
    // Private members would include std::mt19937 for random number generation,
    // and various std::distribution objects (e.g., normal_distribution,
    // uniform_real_distribution, poisson_distribution) to model the parameters.
    std::mt19937 randomEngine;

    // Parameters controlled by the UI (using std::atomic for thread-safety)
    std::atomic<float> pitch { 60.0f };
    std::atomic<float> dispersion { 12.0f };
    std::atomic<float> density { 100.0f };
    std::atomic<float> duration { 100.0f };
};

/**
 * @class AudioEngine
 * @brief The high-performance heart of the instrument.
 *
 * This class is responsible for managing and rendering all active grains. It will
 * be driven by the main JUCE AudioProcessor's processBlock. Its goal is to maintain
 * high CPU efficiency while managing up to the target number of grains.
 */
class AudioEngine
{
public:
    AudioEngine() = default;

    /** Called by the host to prepare the engine for playback. */
    void prepareToPlay (double sampleRate, int samplesPerBlock);

    /**
     * Processes a block of audio. This is where all DSP happens.
     * This method will be called repeatedly on the real-time audio thread.
     */
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    /** Loads a user-provided audio file to be used as a grain source. */
    void loadAudioSample (const juce::File& audioFile);

    /** Selects an internal waveform to be used as a grain source. */
    void setGrainSource (int internalWaveformId);

    /** Provides a non-owning pointer to the model for the UI to control. */
    StochasticModel* getStochasticModel() { return &stochasticModel; }

private:
    double currentSampleRate = 44100.0;
    int grainIdCounter = 0;

    // The model that dictates the properties of new grains.
    StochasticModel stochasticModel;

    // A pre-allocated vector to hold all active grains. We reserve to avoid
    // real-time memory allocation.
    std::vector<Grain> grains;

    // A counter to determine when to ask the StochasticModel for a new grain.
    int samplesUntilNextGrain = 0;

    // Placeholder for the loaded audio file data.
    juce::AudioBuffer<float> sourceAudio;
};
