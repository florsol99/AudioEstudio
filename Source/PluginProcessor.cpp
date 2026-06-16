#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Panner3DAudioProcessor::Panner3DAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    panXParam = new juce::AudioParameterFloat("panX", "Pan X (Left/Right)",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
        0.0f);
    depthParam = new juce::AudioParameterFloat("depth", "Depth (Front/Back)",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f);
    volumeParam = new juce::AudioParameterFloat("volume", "Volume (Up/Down)",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f);

    addParameter(panXParam);
    addParameter(depthParam);
    addParameter(volumeParam);
}

Panner3DAudioProcessor::~Panner3DAudioProcessor()
{
}

//==============================================================================
void Panner3DAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
}

void Panner3DAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Panner3DAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

void Panner3DAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    float panX = panXParam->get();
    float depth = depthParam->get();
    float volume = volumeParam->get();

    // Paneo simple
    float leftGain = (1.0f - panX) / 2.0f;
    float rightGain = (1.0f + panX) / 2.0f;

    // Volumen
    float volumeGain = volume;

    // Profundidad: atenuación dramática
    float depthGain = 1.0f - (depth * depth * 0.8f);
    depthGain = juce::jlimit(0.2f, 1.0f, depthGain);

    int numSamples = buffer.getNumSamples();

    // Variable estática para el filtro (una por canal)
    static float prevOutputL = 0.0f;
    static float prevOutputR = 0.0f;

    if (totalNumInputChannels == 1)
    {
        auto* channelDataIn = buffer.getReadPointer(0);
        auto* leftChannel = buffer.getWritePointer(0);
        auto* rightChannel = buffer.getWritePointer(1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float inputSample = channelDataIn[sample];

            // Aplicar atenuación por profundidad
            float processedSample = inputSample * depthGain;

            // Filtro pasa-altos (más profundidad = más corte de graves)
            if (depth > 0.05f)
            {
                float cutoff = 50.0f + depth * depth * 2500.0f;
                float dt = 1.0f / (float)currentSampleRate;
                float RC = 1.0f / (6.283185f * cutoff);
                float alpha = RC / (RC + dt);

                float output = alpha * (prevOutputL + processedSample - prevOutputL);
                prevOutputL = output;
                processedSample = output;
            }

            leftChannel[sample] = processedSample * leftGain * volumeGain;
            rightChannel[sample] = processedSample * rightGain * volumeGain;
        }
    }
    else if (totalNumInputChannels >= 2)
    {
        auto* leftChannelIn = buffer.getReadPointer(0);
        auto* rightChannelIn = buffer.getReadPointer(1);
        auto* leftChannelOut = buffer.getWritePointer(0);
        auto* rightChannelOut = buffer.getWritePointer(1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float inputSample = (leftChannelIn[sample] + rightChannelIn[sample]) * 0.5f;

            float processedSample = inputSample * depthGain;

            if (depth > 0.05f)
            {
                float cutoff = 50.0f + depth * depth * 2500.0f;
                float dt = 1.0f / (float)currentSampleRate;
                float RC = 1.0f / (6.283185f * cutoff);
                float alpha = RC / (RC + dt);

                float output = alpha * (prevOutputR + processedSample - prevOutputR);
                prevOutputR = output;
                processedSample = output;
            }

            leftChannelOut[sample] = processedSample * leftGain * volumeGain;
            rightChannelOut[sample] = processedSample * rightGain * volumeGain;
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* Panner3DAudioProcessor::createEditor()
{
    return new Panner3DAudioProcessorEditor(*this);
}

bool Panner3DAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String Panner3DAudioProcessor::getName() const
{
    return "3D Panner by Florchis.io";
}

bool Panner3DAudioProcessor::acceptsMidi() const { return false; }
bool Panner3DAudioProcessor::producesMidi() const { return false; }
bool Panner3DAudioProcessor::isMidiEffect() const { return false; }
double Panner3DAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int Panner3DAudioProcessor::getNumPrograms() { return 1; }
int Panner3DAudioProcessor::getCurrentProgram() { return 0; }
void Panner3DAudioProcessor::setCurrentProgram(int index) {}
const juce::String Panner3DAudioProcessor::getProgramName(int index) { return {}; }
void Panner3DAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

void Panner3DAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = juce::ValueTree("panner3d");
    state.setProperty("panX", panXParam->get(), nullptr);
    state.setProperty("depth", depthParam->get(), nullptr);
    state.setProperty("volume", volumeParam->get(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Panner3DAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr)
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid())
        {
            float savedPanX = (float)state.getProperty("panX", panXParam->get());
            float savedDepth = (float)state.getProperty("depth", depthParam->get());
            float savedVolume = (float)state.getProperty("volume", volumeParam->get());

            panXParam->setValueNotifyingHost(savedPanX);
            depthParam->setValueNotifyingHost(savedDepth);
            volumeParam->setValueNotifyingHost(savedVolume);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Panner3DAudioProcessor();
}