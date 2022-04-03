/*
  ==============================================================================

    EmulatorController.h
    Created: 3 Apr 2022 5:48:11pm
    Author:  Max Walley

  ==============================================================================
*/

#pragma once

#include "Chip8Emulator.h"
#include <fstream>

class EmulatorController  : public juce::Component
{
public:
    EmulatorController();
    ~EmulatorController();
    
    void resized() override;
    void paint(juce::Graphics& g) override;
    
private:
    void initRefreshRateSlider();
    void initStartButton();
    void initLoadButton();
    
    juce::TextButton loadButton;
    juce::TextButton startStopButton;
    Chip8Emulator emulator;
    juce::Slider refreshRateSlider;
};
