#include "TouchDetector.h"
#include "TouchModelData.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace BackyardBrains {

TouchDetector::TouchDetector()
    : _bufferHead(0)
    , _bufferCount(0)
    , _bufferFilled(false)
    , _modelLoaded(true)  // model is embedded in header
    , _touchDetected(false)
    , _touchProbability(0.0f)
    , _threshold(0.7f)
    , _cooldownSeconds(2.0f)
    , _enabled(false)
    , _samplesSinceLastPrediction(0)
    , _samplesSinceLastDetection(0)
    , _hopSamples(0)
{
    std::memset(_buffer, 0, sizeof(_buffer));
}

void TouchDetector::pushSamples(const int16_t* samples, int count) {
    for (int i = 0; i < count; i++) {
        _buffer[_bufferHead] = samples[i];
        _bufferHead = (_bufferHead + 1) % TOUCH_BUFFER_SIZE;
        if (_bufferCount < TOUCH_BUFFER_SIZE) {
            _bufferCount++;
        }
    }
    _samplesSinceLastPrediction += count;
    _samplesSinceLastDetection += count;
}

bool TouchDetector::update(int sampleRate) {
    if (sampleRate <= 0) return false;

    int windowSamples = (int)(TouchModelData::WINDOW_SIZE_SECONDS * sampleRate);
    if (windowSamples > TOUCH_BUFFER_SIZE) windowSamples = TOUCH_BUFFER_SIZE;

    // Check if we have enough data
    if (_bufferCount < windowSamples) {
        _bufferFilled = false;
        return false;
    }
    _bufferFilled = true;

    // Calculate hop in samples (0.5 seconds)
    _hopSamples = sampleRate / 2;

    // Only run prediction every hop interval
    if (_samplesSinceLastPrediction < _hopSamples) {
        return false;
    }
    _samplesSinceLastPrediction = 0;

    // Extract features from the latest window
    Features feat = extractFeatures(windowSamples);

    // Scale features using the embedded scaler parameters
    double scaled[5];
    scaled[0] = (feat.rms - TouchModelData::SCALER_MEAN[0]) / TouchModelData::SCALER_SCALE[0];
    scaled[1] = (feat.peak_to_peak - TouchModelData::SCALER_MEAN[1]) / TouchModelData::SCALER_SCALE[1];
    scaled[2] = (feat.std_dev - TouchModelData::SCALER_MEAN[2]) / TouchModelData::SCALER_SCALE[2];
    scaled[3] = (feat.percentile_90 - TouchModelData::SCALER_MEAN[3]) / TouchModelData::SCALER_SCALE[3];
    scaled[4] = (feat.mean_abs - TouchModelData::SCALER_MEAN[4]) / TouchModelData::SCALER_SCALE[4];

    // Run Random Forest
    double probability = runForest(scaled);
    _touchProbability = (float)probability;

    // Apply threshold with cooldown
    int cooldownSamples = (int)(_cooldownSeconds * sampleRate);
    if (probability > _threshold && _samplesSinceLastDetection > cooldownSamples) {
        _touchDetected = true;
        _samplesSinceLastDetection = 0;
    } else if (probability <= _threshold) {
        _touchDetected = false;
    }
    // If within cooldown and was detected, keep showing detected briefly
    // then fade off after cooldown
    if (_samplesSinceLastDetection > cooldownSamples && _touchDetected) {
        if (probability <= _threshold) {
            _touchDetected = false;
        }
    }

    return true;
}

TouchDetector::Features TouchDetector::extractFeatures(int windowSamples) const {
    Features feat;
    feat.rms = 0;
    feat.peak_to_peak = 0;
    feat.std_dev = 0;
    feat.percentile_90 = 0;
    feat.mean_abs = 0;

    // Copy the latest windowSamples into a temporary array as float
    // The buffer is circular, so we read backwards from bufferHead
    std::vector<double> window(windowSamples);
    int readPos = (_bufferHead - windowSamples + TOUCH_BUFFER_SIZE) % TOUCH_BUFFER_SIZE;

    double sum = 0.0;
    for (int i = 0; i < windowSamples; i++) {
        int idx = (readPos + i) % TOUCH_BUFFER_SIZE;
        window[i] = (double)_buffer[idx] / 32768.0;
        sum += window[i];
    }

    // DC offset removal (same as Python: signal - mean(signal))
    double mean = sum / windowSamples;
    for (int i = 0; i < windowSamples; i++) {
        window[i] -= mean;
    }

    // 1. RMS
    double sumSq = 0.0;
    for (int i = 0; i < windowSamples; i++) {
        sumSq += window[i] * window[i];
    }
    feat.rms = std::sqrt(sumSq / windowSamples);

    // 2. Peak-to-Peak
    double minVal = window[0], maxVal = window[0];
    for (int i = 1; i < windowSamples; i++) {
        if (window[i] < minVal) minVal = window[i];
        if (window[i] > maxVal) maxVal = window[i];
    }
    feat.peak_to_peak = maxVal - minVal;

    // 3. Standard Deviation
    double sumForStd = 0.0;
    double meanForStd = 0.0;
    for (int i = 0; i < windowSamples; i++) {
        meanForStd += window[i];
    }
    meanForStd /= windowSamples;
    for (int i = 0; i < windowSamples; i++) {
        double diff = window[i] - meanForStd;
        sumForStd += diff * diff;
    }
    feat.std_dev = std::sqrt(sumForStd / windowSamples);

    // 4. 90th Percentile of absolute values
    std::vector<double> absValues(windowSamples);
    for (int i = 0; i < windowSamples; i++) {
        absValues[i] = std::fabs(window[i]);
    }
    std::sort(absValues.begin(), absValues.end());
    int p90Index = (int)(0.9 * windowSamples);
    if (p90Index >= windowSamples) p90Index = windowSamples - 1;
    feat.percentile_90 = absValues[p90Index];

    // 5. Mean Absolute Value
    double sumAbs = 0.0;
    for (int i = 0; i < windowSamples; i++) {
        sumAbs += std::fabs(window[i]);
    }
    feat.mean_abs = sumAbs / windowSamples;

    return feat;
}

double TouchDetector::runForest(const double scaledFeatures[5]) const {
    double sumProb = 0.0;

    for (int t = 0; t < TouchModelData::N_TREES; t++) {
        const TouchModelData::TreeNode* nodes = TouchModelData::ALL_TREES[t];
        int nodeIdx = 0;

        // Traverse tree until we reach a leaf (children_left == -1)
        while (nodes[nodeIdx].left != -1) {
            int featureIdx = nodes[nodeIdx].feature;
            if (scaledFeatures[featureIdx] <= nodes[nodeIdx].threshold) {
                nodeIdx = nodes[nodeIdx].left;
            } else {
                nodeIdx = nodes[nodeIdx].right;
            }
        }

        // Leaf node: accumulate probability of class 1 (touch)
        sumProb += nodes[nodeIdx].prob;
    }

    return sumProb / TouchModelData::N_TREES;
}

} // namespace BackyardBrains
