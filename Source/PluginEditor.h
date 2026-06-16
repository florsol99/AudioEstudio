#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class Panner3DAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit Panner3DAudioProcessorEditor(Panner3DAudioProcessor&);
    ~Panner3DAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    // Manejadores del mouse
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    // Referencia al procesador
    Panner3DAudioProcessor& processor;

    // Componentes de la UI
    std::unique_ptr<juce::Slider> depthSlider;
    std::unique_ptr<juce::Slider> volumeSlider;
    std::unique_ptr<juce::Label> depthLabel;
    std::unique_ptr<juce::Label> volumeLabel;

    // Variables para el drag del pad XY
    juce::Point<float> dragPosition;
    bool isDragging = false;

    // Helper para actualizar parámetros desde la UI
    void updateParametersFromMousePosition(const juce::Point<float>& position);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Panner3DAudioProcessorEditor)
};