/*
  ==============================================================================

    Chip8Emulator.h
    Created: 15 Mar 2022 8:50:07pm
    Author:  Max Walley

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <random>

class Chip8Emulator  : public juce::Component,
                       public juce::Timer,
                       public juce::KeyListener
{
public:
    Chip8Emulator();
    ~Chip8Emulator();
    
    void load(std::istream& programData);
    
    void setRefreshRate(int newRefreshRateHz);
    
    void setPlayState(bool play);
    bool getIsPlaying() const {return isPlaying;}
    
private:
    void paint(juce::Graphics& g) override;
    
    void timerCallback() override;
    
    bool keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) override;
    
    void runCycle();
    
    void fetchOpcode();
    void decodeAndExecuteOpcode();
    
    void reportUnrecognisedOpcode();
    
    void updateTimers();
    
    bool checkForCarry(uint8_t first, uint8_t second) const;
    bool checkForBorrow(uint8_t first, uint8_t second) const;
    
    std::array<uint8_t, 80> getFontset() const;
    
    void clearScreen();
    
    std::array<std::pair<uint8_t, int>, 16> getDefaultKeyPairings() const;
    
    uint16_t currentOpcode;
    std::array<uint8_t, 4096> memory;
    
    //Registers
    std::array<uint8_t, 16> vRegisters;
    
    uint16_t indexRegister;
    uint16_t programCounter;
    
    std::array<uint16_t, 16> stack;
    uint16_t stackPointer;
    
    uint8_t delayTimer;
    uint8_t soundTimer;
    
    const int numWidthPixels = 64;
    const int numHeightPixels = 32;
    juce::Image display;
    
    bool keyPressWaitFlag = false;
    uint8_t currentInputKey = 0;
    
    std::array<std::pair<uint8_t, int>, 16> keyPairings;
    
    int refreshRate = 60;
    bool isPlaying = false;
};
