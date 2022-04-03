/*
  ==============================================================================

    EmulatorController.cpp
    Created: 3 Apr 2022 5:48:11pm
    Author:  Max Walley

  ==============================================================================
*/

#include "EmulatorController.h"

EmulatorController::EmulatorController()
{
    addAndMakeVisible(emulator);
    
    initRefreshRateSlider();
    initStartButton();
    initLoadButton();
    
    setSize(1024, 662);
}

EmulatorController::~EmulatorController()
{
    
}

void EmulatorController::resized()
{
    loadButton.setBounds(10, 10, 150, 30);
    startStopButton.setBounds(getWidth() - 160, 10, 150, 30);
    
    emulator.setBounds(0, 50, getWidth(), getHeight() - 100);
    
    refreshRateSlider.setBounds(100, getHeight() - 40, getWidth() - 100, 30);
}

void EmulatorController::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16));
    g.drawText("Refresh Rate", 0, getHeight() - 40, 100, 30, juce::Justification::centredRight);
}

void EmulatorController::initRefreshRateSlider()
{
    refreshRateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    refreshRateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 30);
    refreshRateSlider.textFromValueFunction = [](double value)
    {
        return juce::String(int(value)) + "Hz";
    };
    refreshRateSlider.setRange(60.0, 1000.0);
    refreshRateSlider.onValueChange = [this]()
    {
        emulator.setRefreshRate(refreshRateSlider.getValue());
    };
    refreshRateSlider.setValue(300);
    addAndMakeVisible(refreshRateSlider);
}

void EmulatorController::initStartButton()
{
    startStopButton.setButtonText("Play");
    
    startStopButton.onClick = [this]()
    {
        bool newState = !emulator.getIsPlaying();
        juce::String newButtonText = newState ? "Pause" : "Play";
        
        emulator.setPlayState(newState);
        startStopButton.setButtonText(newButtonText);
    };
    
    addAndMakeVisible(startStopButton);
}

void EmulatorController::initLoadButton()
{
    loadButton.setButtonText("Load App");
    
    loadButton.onClick = [this]()
    {
        juce::FileChooser loader("Load Application", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory));
        
        if(loader.browseForFileToOpen())
        {
            juce::File fileToOpen = loader.getResult();
            
            std::ifstream fileStream(fileToOpen.getFullPathName().toStdString());
            
            if(fileStream.is_open())
            {
                emulator.load(fileStream);
            }
        }
    };
    
    addAndMakeVisible(loadButton);
}
