/*
  ==============================================================================

    SineWaveGenerator.h
    Created: 3 Apr 2022 6:51:31pm
    Author:  Max Walley

  ==============================================================================
*/

#pragma once

#include <cmath>

class SineWaveGenerator
{
public:
    SineWaveGenerator() {};
    
    void setFreq(double newFreq);
    void setSampleRate(double sampleRate);
    
    double getNextSample();
    
private:
    void updateAngleDelta();
    
    double sr;
    double freq = 4000.0;
    double angleDelta;
    double currentAngle;
};
