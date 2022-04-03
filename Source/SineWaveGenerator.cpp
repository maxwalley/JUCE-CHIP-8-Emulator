/*
  ==============================================================================

    SineWaveGenerator.cpp
    Created: 3 Apr 2022 6:51:31pm
    Author:  Max Walley

  ==============================================================================
*/

#include "SineWaveGenerator.h"

void SineWaveGenerator::setFreq(double newFreq)
{
    freq = newFreq;
    updateAngleDelta();
}

void SineWaveGenerator::setSampleRate(double sampleRate)
{
    sr = sampleRate;
    updateAngleDelta();
}

void SineWaveGenerator::updateAngleDelta()
{
    angleDelta = (freq / sr) * 2.0 * M_PI;
}

double SineWaveGenerator::getNextSample()
{
    auto sample = (float) std::sin (currentAngle);
    currentAngle += angleDelta;
    return sample;
}
