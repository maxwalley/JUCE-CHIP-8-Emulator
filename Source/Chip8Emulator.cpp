/*
  ==============================================================================

    Chip8Emulator.cpp
    Created: 15 Mar 2022 8:50:07pm
    Author:  Max Walley

  ==============================================================================
*/

#include "Chip8Emulator.h"

Chip8Emulator::Chip8Emulator()  : display(juce::Image::RGB, numWidthPixels, numHeightPixels, true)
{
    keyPairings = getDefaultKeyPairings();
    
    audioPlaying = false;
    audioGenerator.setFreq(2000.0);
}

Chip8Emulator::~Chip8Emulator()
{
    
}

void Chip8Emulator::load(std::istream& programData)
{
    //Reset System State
    programCounter = 0x200;
    currentOpcode = 0;
    indexRegister = 0;
    stackPointer = 0;
    
    std::fill(memory.begin(), memory.end(), 0);
    std::fill(vRegisters.begin(), vRegisters.end(), 0);
    std::fill(stack.begin(), stack.end(), 0);
    
    //Load the fontset
    const auto fontset = getFontset();
    std::copy(fontset.cbegin(), fontset.cend(), memory.begin());
    
    delayTimer = 0;
    soundTimer = 0;
    
    programData.unsetf(std::ios_base::skipws);
    
    //Load program into memory
    std::copy(std::istream_iterator<uint8_t>(programData), std::istream_iterator<uint8_t>(), memory.begin() + 512);
    
    clearScreen();
}

void Chip8Emulator::setRefreshRate(int newRefreshRateHz)
{
    refreshRate = newRefreshRateHz;
    
    if(isPlaying)
    {
        startTimerHz(refreshRate);
    }
}

void Chip8Emulator::setPlayState(bool play)
{
    isPlaying = play;
    
    isPlaying ? startTimerHz(refreshRate) : stopTimer();
}

void Chip8Emulator::paint(juce::Graphics& g)
{
    g.drawImage(display, getLocalBounds().toFloat());
}

void Chip8Emulator::timerCallback()
{
    runCycle();
}

bool Chip8Emulator::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    int keyCode = key.getKeyCode();
    
    const auto foundPairing = std::find_if(keyPairings.cbegin(), keyPairings.cend(), [keyCode](const auto& pairing)
    {
        return pairing.second == keyCode;
    });
    
    if(foundPairing != keyPairings.cend())
    {
        currentInputKey = foundPairing->first;
        keyPressWaitFlag = true;
    }
    
    return false;
}

void Chip8Emulator::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples)
{
    if(!audioPlaying)
    {
        //Zero all output audio
        for(int channel = 0; channel < numOutputChannels; ++channel)
        {
            for(int sample = 0; sample < numSamples; ++sample)
            {
                outputChannelData[channel][sample] = 0.0f;
            }
        }
        
        return;
    }
    
    for(int sample = 0; sample < numSamples; ++sample)
    {
        const double sampleToPlay = audioGenerator.getNextSample();
        
        for(int channel = 0; channel < numOutputChannels; ++channel)
        {
            outputChannelData[channel][sample] = sampleToPlay;
        }
    }
}

void Chip8Emulator::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    audioGenerator.setSampleRate(device->getCurrentSampleRate());
}

void Chip8Emulator::audioDeviceStopped()
{
    
}

void Chip8Emulator::runCycle()
{
    fetchOpcode();

    decodeAndExecuteOpcode();
    
    updateTimers();
}

void Chip8Emulator::fetchOpcode()
{
    const uint8_t firstByte = memory[programCounter];
    const uint8_t secondByte = memory[programCounter + 1];
    
    //Shift the first byte to the start
    currentOpcode = firstByte << 8;
    currentOpcode |= secondByte;
}

void Chip8Emulator::decodeAndExecuteOpcode()
{
    //Look at the first digit of the opcode
    switch(0xF000 & currentOpcode)
    {
        case 0x0000:
        {
            switch(0x000F & currentOpcode)
            {
                case 0x0000:
                {
                    clearScreen();
                    programCounter += 2;
                    return;
                }
                    
                case 0x000E:
                {
                    programCounter = stack[--stackPointer];
                    programCounter += 2;
                    return;
                }

                default:
                {
                    reportUnrecognisedOpcode();
                    
                    programCounter += 2;
                    
                    return;
                }
            }
            return;
        }
            
        case 0x1000:
        {
            programCounter = 0x0FFF & currentOpcode;
            return;
        }
            
        case 0x2000:
        {
            //CALL SUBROUTINE
            stack[stackPointer++] = programCounter;
            programCounter = 0x0FFF & currentOpcode;
            return;
        }
            
        case 0x3000:
        {
            uint8_t registerIndex = (0x0F00 & currentOpcode) >> 8;
            uint8_t val = 0x00FF & currentOpcode;
            
            if(vRegisters[registerIndex] == val)
            {
                //Skip Next Instruction
                programCounter += 2;
            }
            
            programCounter += 2;
            return;
        }
            
        case 0x4000:
        {
            uint8_t registerIndex = (0x0F00 & currentOpcode) >> 8;
            uint8_t val = 0x00FF & currentOpcode;
            
            if(vRegisters[registerIndex] != val)
            {
                //Skip Next Instruction
                programCounter += 2;
            }
            
            programCounter += 2;
            return;
        }
            
        case 0x5000:
        {
            uint8_t firstRegisterIndex = (0x0F00 & currentOpcode) >> 8;
            uint8_t secondRegisterIndex = (0x00F0 & currentOpcode) >> 4;
            
            if(vRegisters[firstRegisterIndex] == vRegisters[secondRegisterIndex])
            {
                //Skip Next Instrution
                programCounter += 2;
            }
            
            programCounter += 2;
            return;
        };
            
        case 0x6000:
        {
            uint8_t registerIndex = (0x0F00 & currentOpcode) >> 8;
            uint8_t val = 0x00FF & currentOpcode;
            
            vRegisters[registerIndex] = val;
            
            programCounter += 2;
            return;
        }
            
        case 0x7000:
        {
            uint8_t registerIndex = (0x0F00 & currentOpcode) >> 8;
            uint8_t val = 0x00FF & currentOpcode;
            
            vRegisters[registerIndex] += val;
            
            programCounter += 2;
            return;
        }
            
        case 0x8000:
        {
            uint8_t firstRegisterIndex = (0x0F00 & currentOpcode) >> 8;
            uint8_t secondRegisterIndex = (0x00F0 & currentOpcode) >> 4;
            
            switch(0x000F & currentOpcode)
            {
                case 0x0000:
                {
                    vRegisters[firstRegisterIndex] = vRegisters[secondRegisterIndex];
                    programCounter += 2;
                    return;
                }
                
                case 0x0001:
                {
                    vRegisters[firstRegisterIndex] |= vRegisters[secondRegisterIndex];
                    programCounter += 2;
                    return;
                }
                    
                case 0x0002:
                {
                    vRegisters[firstRegisterIndex] &= vRegisters[secondRegisterIndex];
                    programCounter += 2;
                    return;
                }
                    
                case 0x0003:
                {
                    vRegisters[firstRegisterIndex] ^= vRegisters[secondRegisterIndex];
                    programCounter += 2;
                    return;
                }
                    
                case 0x0004:
                {
                    //Set the carry flag
                    vRegisters.back() = checkForCarry(vRegisters[firstRegisterIndex], vRegisters[secondRegisterIndex]);
                    
                    vRegisters[firstRegisterIndex] += vRegisters[secondRegisterIndex];
                    
                    programCounter += 2;
                    return;
                }
                    
                case 0x0005:
                {
                    //Set the borrow flag
                    vRegisters.back() = !checkForBorrow(vRegisters[firstRegisterIndex], vRegisters[secondRegisterIndex]);
                    
                    vRegisters[firstRegisterIndex] -= vRegisters[secondRegisterIndex];
                    
                    programCounter += 2;
                    return;
                }
                    
                case 0x0006:
                {
                    //Store the least significant bit in the carry flag
                    vRegisters.back() = 0x1 & vRegisters[firstRegisterIndex];
                    
                    vRegisters[firstRegisterIndex] >>= 1;
                    
                    programCounter += 2;
                    return;
                }
                    
                case 0x0007:
                {
                    //Set the borrow flag
                    vRegisters.back() = !checkForBorrow(vRegisters[secondRegisterIndex], vRegisters[firstRegisterIndex]);
                    
                    vRegisters[firstRegisterIndex] = vRegisters[secondRegisterIndex] - vRegisters[firstRegisterIndex];
                    
                    programCounter += 2;
                    return;
                }
                    
                case 0x000E:
                {
                    //Store the most significant bit in the carry flag
                    vRegisters.back() = (0x80 & vRegisters[firstRegisterIndex]) >> 7;
                    
                    vRegisters[firstRegisterIndex] <<= 1;
                    
                    programCounter += 2;
                    return;
                }
                    
                default:
                {
                    reportUnrecognisedOpcode();
                    programCounter += 2;
                    return;
                }
            }
        }
            
        case 0x9000:
        {
            uint8_t firstRegisterIndex = (0x0F00 & currentOpcode) >> 8;
            uint8_t secondRegisterIndex = (0x00F0 & currentOpcode) >> 4;
            
            if(vRegisters[firstRegisterIndex] != vRegisters[secondRegisterIndex])
            {
                //Skip Next Instrution
                programCounter += 2;
            }
            
            programCounter +=2;
            return;
        }
            
        case 0xA000:
        {
            indexRegister = 0x0FFF & currentOpcode;
            programCounter += 2;
            return;
        }
            
        case 0xB000:
        {
            uint8_t offset = vRegisters[0];
            uint16_t newMemoryLocation = 0x0FFF & currentOpcode;
            
            programCounter = newMemoryLocation + offset;
            
            return;
        }
            
        case 0xC000:
        {
            uint8_t registerIndex = (0x0F00 & currentOpcode) >> 8;
            uint8_t value = 0x00FF & currentOpcode;
            
            std::random_device dev;
            std::mt19937 generator(dev());
            std::uniform_int_distribution<uint8_t> distributer(0, 255);;
            
            uint8_t randomVal = distributer(generator);
            
            vRegisters[registerIndex] = value & randomVal;
            
            programCounter += 2;
            
            return;
        }
            
        case 0xD000:
        {
            vRegisters.back() = 0;
            
            uint8_t firstRegisterIndex = (0x0F00 & currentOpcode) >> 8;
            uint8_t spriteXPos = vRegisters[firstRegisterIndex];
            
            uint8_t secondRegisterIndex = (0x00F0 & currentOpcode) >> 4;
            uint8_t spriteYPos = vRegisters[secondRegisterIndex];
            
            uint8_t spriteHeight = 0x000F & currentOpcode;
            
            //Go through each vertical line of pixels
            for(int y = 0; y < spriteHeight; ++y)
            {
                uint8_t horizontalPixels = memory[indexRegister + y];
                
                for(int xOffset = 0; xOffset < 8; ++xOffset)
                {
                    //Check if the pixel here is on
                    if((horizontalPixels & (0x80 >> xOffset)) != 0)
                    {
                        int x = spriteXPos + xOffset;
                        
                        //If the pixel has already been set as on
                        if(display.getPixelAt(x, y + spriteYPos) == juce::Colours::white)
                        {
                            display.setPixelAt(x, y + spriteYPos, juce::Colours::black);
                            
                            //Set the carry flag to true
                            vRegisters.back() = 1;
                        }
                        else
                        {
                            display.setPixelAt(x, y + spriteYPos, juce::Colours::white);
                        }
                    }
                }
            }
            
            repaint();
            programCounter += 2;
            
            return;
        }
            
        case 0xE000:
        {
            uint8_t registerIndex = (0x0F00 & currentOpcode) >> 8;
            
            switch(0x00FF & currentOpcode)
            {
                case 0x009E:
                {
                    uint8_t keyToCheck = vRegisters[registerIndex];
                    
                    const auto foundIt = std::find_if(keyPairings.cbegin(), keyPairings.cend(), [keyToCheck](const auto& keyPair)
                    {
                        return keyToCheck == keyPair.first;
                    });
                    
                    if(foundIt != keyPairings.end() && juce::KeyPress::isKeyCurrentlyDown(foundIt->second))
                    {
                        //Skip next instruction
                        programCounter += 2;
                    }
                    
                    programCounter += 2;
                    return;
                }
                    
                case 0x00A1:
                {
                    uint8_t keyToCheck = vRegisters[registerIndex];
                    
                    const auto foundIt = std::find_if(keyPairings.cbegin(), keyPairings.cend(), [keyToCheck](const auto& keyPair)
                    {
                        return keyToCheck == keyPair.first;
                    });
                    
                    if(foundIt != keyPairings.end() && !juce::KeyPress::isKeyCurrentlyDown(foundIt->second))
                    {
                        //Skip next instruction
                        programCounter += 2;
                    }
                    
                    programCounter += 2;
                    return;
                }
                    
                default:
                {
                    reportUnrecognisedOpcode();
                    programCounter += 2;
                    return;
                }
            }
        }
            
        case 0xF000:
        {
            uint8_t registerIndex = (0x0F00 & currentOpcode) >> 8;
            
            switch(0x00FF & currentOpcode)
            {
                case 0x0007:
                {
                    vRegisters[registerIndex] = delayTimer;
                    programCounter += 2;
                    return;
                }
                    
                case 0x000A:
                {
                    keyPressWaitFlag = false;
                    addKeyListener(this);
                    
                    std::cout << "Waiting for key " << std::endl;
                    
                    //Wait for a key press
                    while(!keyPressWaitFlag)
                    {
                        ;
                    }
                    
                    vRegisters[registerIndex] = currentInputKey;
                    
                    currentInputKey = 0;
                    removeKeyListener(this);
                    
                    programCounter += 2;
                    return;
                }
                    
                case 0x0015:
                {
                    delayTimer = vRegisters[registerIndex];
                    programCounter += 2;
                    return;
                }
                    
                case 0x0018:
                {
                    soundTimer = vRegisters[registerIndex];
                    programCounter += 2;
                    return;
                }
                    
                case 0x001E:
                {
                    indexRegister = indexRegister += vRegisters[registerIndex];
                    programCounter += 2;
                    return;
                }
                    
                case 0x0029:
                {
                    indexRegister = vRegisters[registerIndex] * 0x5;
                    programCounter += 2;
                    return;
                }
                    
                case 0x0033:
                {
                    uint8_t registerValue = vRegisters[registerIndex];
                    
                    memory[indexRegister]     = registerValue / 100;
                    memory[indexRegister + 1] = (registerValue / 10) % 10;
                    memory[indexRegister + 2] = (registerValue % 100) % 10;
                    
                    programCounter += 2;
                    return;
                }
                    
                case 0x0055:
                {
                    uint16_t currentLocation = indexRegister;
                    
                    std::for_each(vRegisters.cbegin(), vRegisters.cbegin() + registerIndex + 1, [&currentLocation, this](uint8_t registerValue)
                    {
                        memory[currentLocation++] = registerValue;
                    });
                    
                    programCounter += 2;
                    return;
                }
                    
                case 0x0065:
                {
                    uint16_t currentLocation = indexRegister;
                    
                    std::for_each(vRegisters.begin(), vRegisters.begin() + registerIndex + 1, [&currentLocation, this](uint8_t& registerValue)
                    {
                        registerValue = memory[currentLocation++];
                    });
                    
                    
                    programCounter += 2;
                    return;
                }
                    
                default:
                {
                    reportUnrecognisedOpcode();
                    programCounter += 2;
                    return;
                }
            }
        }
            
        default:
        {
            reportUnrecognisedOpcode();
            programCounter += 2;
            return;
        }
    }
}

void Chip8Emulator::reportUnrecognisedOpcode()
{
    std::cout << "Unknown Opcode Encountered: " << std::hex << currentOpcode << std::endl;
}

void Chip8Emulator::updateTimers()
{
    if(delayTimer > 0)
    {
        --delayTimer;
    }
    
    if(soundTimer != 0)
    {
        audioPlaying = true;
        --soundTimer;
    }
    else
    {
        audioPlaying = false;
    }
}

bool Chip8Emulator::checkForCarry(uint8_t first, uint8_t second) const
{
    uint16_t result = first + second;
    return result > std::numeric_limits<uint8_t>::max();
}

bool Chip8Emulator::checkForBorrow(uint8_t first, uint8_t second) const
{
    return second > first;
}

std::array<uint8_t, 80> Chip8Emulator::getFontset() const
{
    return {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
      };
}

void Chip8Emulator::clearScreen()
{
    //Clear the screen
    for(int x = 0; x < numWidthPixels; ++x)
    {
        for(int y = 0; y < numHeightPixels; ++y)
        {
            display.setPixelAt(x, y, juce::Colours::black);
        }
    }
    repaint();
}

std::array<std::pair<uint8_t, int>, 16> Chip8Emulator::getDefaultKeyPairings() const
{
    return {
        std::make_pair(0x00, 88),
        std::make_pair(0x01, 49),
        std::make_pair(0x02, 50),
        std::make_pair(0x03, 51),
        std::make_pair(0x04, 81),
        std::make_pair(0x05, 87),
        std::make_pair(0x06, 69),
        std::make_pair(0x07, 65),
        std::make_pair(0x08, 83),
        std::make_pair(0x09, 68),
        std::make_pair(0x0A, 90),
        std::make_pair(0x0B, 67),
        std::make_pair(0x0C, 52),
        std::make_pair(0x0D, 82),
        std::make_pair(0x0E, 70),
        std::make_pair(0x0F, 86)
    };
}
