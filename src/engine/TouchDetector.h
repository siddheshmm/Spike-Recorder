#ifndef BACKYARDBRAINS_TOUCHDETECTOR_H
#define BACKYARDBRAINS_TOUCHDETECTOR_H

#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>

namespace BackyardBrains {

class TouchDetector {
public:
    TouchDetector();

    // Feed new audio samples into the rolling buffer
    // samples: array of int16_t raw audio samples
    // count: number of samples
    void pushSamples(const int16_t* samples, int count);

    // Run prediction if enough time has elapsed since last prediction
    // sampleRate: current sample rate (e.g., 10000)
    // Returns true if a new prediction was made this call
    bool update(int sampleRate);

    // --- Accessors ---
    bool isTouchDetected() const { return _touchDetected; }
    float touchProbability() const { return _touchProbability; }
    bool isModelLoaded() const { return _modelLoaded; }
    bool hasEnoughData() const { return _bufferFilled; }
    bool isEnabled() const { return _enabled; }
    void setEnabled(bool enabled) { _enabled = enabled; }

    // Configuration
    void setThreshold(float threshold) { _threshold = threshold; }
    float getThreshold() const { return _threshold; }
    void setCooldownSeconds(float seconds) { _cooldownSeconds = seconds; }

private:
    // Rolling sample buffer (circular)
    static const int TOUCH_BUFFER_SIZE = 60000; // 6 seconds at 10kHz
    int16_t _buffer[TOUCH_BUFFER_SIZE];
    int _bufferHead;       // next write position
    int _bufferCount;      // total samples in buffer
    bool _bufferFilled;    // true when we have at least one full window

    // Feature extraction
    struct Features {
        double rms;
        double peak_to_peak;
        double std_dev;
        double percentile_90;
        double mean_abs;
    };

    Features extractFeatures(int windowSamples) const;

    // Model inference
    double runForest(const double scaledFeatures[5]) const;

    // State
    bool _modelLoaded;
    bool _touchDetected;
    float _touchProbability;
    float _threshold;
    float _cooldownSeconds;
    bool _enabled;

    // Timing
    int _samplesSinceLastPrediction;
    int _samplesSinceLastDetection;
    int _hopSamples;  // how many samples between predictions
};

} // namespace BackyardBrains

#endif
