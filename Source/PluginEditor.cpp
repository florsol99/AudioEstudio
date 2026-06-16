#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Panner3DAudioProcessorEditor::Panner3DAudioProcessorEditor(Panner3DAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(700, 600);

    // Crear el slider de profundidad (adelante/atrás)
    depthSlider = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::TextBoxBelow);
    depthSlider->setRange(0.0f, 1.0f, 0.01f);
    depthSlider->setValue(processor.depthParam->get(), juce::dontSendNotification);
    depthSlider->onValueChange = [this] { *processor.depthParam = depthSlider->getValue(); repaint(); };
    addAndMakeVisible(depthSlider.get());

    depthLabel = std::make_unique<juce::Label>("depthLabel", "DEPTH\n(Front/Back)");
    depthLabel->setJustificationType(juce::Justification::centred);
    depthLabel->setFont(juce::Font(12.0f));
    addAndMakeVisible(depthLabel.get());

    // Crear el slider de volumen (respaldo)
    volumeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::TextBoxBelow);
    volumeSlider->setRange(0.0f, 1.0f, 0.01f);
    volumeSlider->setValue(processor.volumeParam->get(), juce::dontSendNotification);
    volumeSlider->onValueChange = [this] { *processor.volumeParam = volumeSlider->getValue(); repaint(); };
    addAndMakeVisible(volumeSlider.get());

    volumeLabel = std::make_unique<juce::Label>("volumeLabel", "VOLUME\n(Backup)");
    volumeLabel->setJustificationType(juce::Justification::centred);
    volumeLabel->setFont(juce::Font(12.0f));
    addAndMakeVisible(volumeLabel.get());

    // Inicializar posición del drag (X = pan, Y = volume)
     // Inicializar posición del drag en el CENTRO (0.5, 0.5)
    dragPosition = { 0.5f, 0.5f };

    // Opcional: Sincronizar los parámetros del procesador con esta posición
    *processor.panXParam = 0.0f;      // Centro (0 = centro)
    *processor.volumeParam = 0.5f;    // Volumen medio (0.5 = 50%)
}

Panner3DAudioProcessorEditor::~Panner3DAudioProcessorEditor()
{
}

//==============================================================================
void Panner3DAudioProcessorEditor::paint(juce::Graphics& g)
{
    int w = getWidth();
    int h = getHeight();
    if (w < 100 || h < 100)
        return;

    // ========== COLORES PERSONALIZADOS ==========
    juce::Colour backgroundColor = juce::Colour(20, 18, 25);
    juce::Colour padBackground = juce::Colour(30, 28, 38);

    juce::Colour panColor = juce::Colour(255, 105, 180);      // Rosa
    juce::Colour volumeColor = juce::Colour(180, 100, 255);   // Violeta
    juce::Colour depthColor = juce::Colour(100, 200, 255);    // Celeste
    juce::Colour ballColor = juce::Colour(255, 215, 0);       // Dorado
    juce::Colour ballBorderColor = juce::Colour(255, 235, 100);
    juce::Colour textColor = juce::Colour(200, 200, 210);
    // ==========================================

    g.fillAll(backgroundColor);

    // Área del pad XY
    int padX = 50;
    int padY = 50;
    int padWidth = w - 150;
    int padHeight = h - 150;

    if (padWidth <= 0) padWidth = 100;
    if (padHeight <= 0) padHeight = 100;

    juce::Rectangle<int> padArea(padX, padY, padWidth, padHeight);

    // Borde del pad
    g.setColour(ballColor.withAlpha(0.5f));
    g.drawRect(padArea, 2.0f);

    // Fondo del pad
    g.setColour(padBackground);
    g.fillRect(padArea);

    // Línea HORIZONTAL (PANEO - ROSA)
    g.setColour(panColor);
    float centerY = (float)padArea.getY() + (float)padArea.getHeight() * 0.5f;
    g.drawLine((float)padArea.getX(), centerY, (float)padArea.getRight(), centerY, 2.5f);

    // Línea VERTICAL (VOLUMEN - VIOLETA)
    g.setColour(volumeColor);
    float centerX = (float)padArea.getX() + (float)padArea.getWidth() * 0.5f;
    g.drawLine(centerX, (float)padArea.getY(), centerX, (float)padArea.getBottom(), 2.5f);

    // ========== ETIQUETAS DEL PAD ==========
    g.setFont(juce::Font(12.0f, juce::Font::bold));

    // Paneo (Rosa)
    g.setColour(panColor);
    g.drawText("LEFT", padArea.getX() + 5, (int)centerY - 15, 45, 15, juce::Justification::centred);
    g.drawText("RIGHT", padArea.getRight() - 50, (int)centerY - 15, 45, 15, juce::Justification::centred);

    // Volumen (Violeta)
    g.setColour(volumeColor);
    g.drawText("LOUD", (int)centerX - 50, padArea.getY() + 5, 50, 15, juce::Justification::centred);
    g.drawText("SOFT", (int)centerX - 50, padArea.getBottom() - 20, 50, 15, juce::Justification::centred);

    // ========== PELOTITA ==========
    float ballX = (float)padArea.getX() + dragPosition.x * (float)padArea.getWidth();
    float ballY = (float)padArea.getY() + dragPosition.y * (float)padArea.getHeight();

    float depthValue = processor.depthParam->get();
    float minSize = 14.0f;
    float maxSize = 42.0f;
    float ballSize = maxSize - (depthValue * (maxSize - minSize));

    if (isDragging)
        ballSize += 6.0f;

    // Sombra
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillEllipse(ballX - ballSize / 2 + 3, ballY - ballSize / 2 + 3, ballSize, ballSize);

    // Pelotita
    g.setColour(ballColor);
    g.fillEllipse(ballX - ballSize / 2, ballY - ballSize / 2, ballSize, ballSize);

    // Borde
    g.setColour(ballBorderColor);
    g.drawEllipse(ballX - ballSize / 2, ballY - ballSize / 2, ballSize, ballSize, 2.0f);

    // Brillo
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.fillEllipse(ballX - ballSize / 4, ballY - ballSize / 4, ballSize / 2, ballSize / 2);

    // ========== TEXTO INFORMATIVO (una sola línea) ==========
    g.setFont(juce::Font(13.0f, juce::Font::bold));

    int textY = h - 60;

    // PAN (Izquierda)
    g.setColour(panColor);
    float panValue = processor.panXParam->get();
    juce::String panText;
    if (std::abs(panValue) < 0.05f)
        panText = "CENTER";
    else if (panValue > 0)
        panText = juce::String(panValue * 100, 0) + "% RIGHT";
    else
        panText = juce::String(-panValue * 100, 0) + "% LEFT";
    g.drawText("PAN: " + panText, 40, textY, 150, 20, juce::Justification::centredLeft);

    // VOL (Centro)
    g.setColour(volumeColor);
    float volumeValue = processor.volumeParam->get();
    int volumeX = (w / 2) - 60;  // Centro menos la mitad del ancho del texto
    g.drawText("VOL: " + juce::String(volumeValue * 100, 0) + "%", volumeX, textY, 80, 20, juce::Justification::centred);

    // DEPTH (Derecha)
    g.setColour(depthColor);
    float depthPercent = processor.depthParam->get() * 100;
    juce::String depthStatus;
    if (depthPercent < 15)
        depthStatus = "CLOSE";
    else if (depthPercent > 85)
        depthStatus = "FAR";
    else
        depthStatus = juce::String(depthPercent, 0) + "%";
    g.drawText("DEPTH: " + depthStatus, getWidth() - 220, textY, 150, 20, juce::Justification::centredRight);

    // ========== INSTRUCCIONES (debajo de DEPTH, a la derecha) ==========
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.setColour(depthColor);
    g.drawText("MOUSE WHEEL for DEPTH", getWidth() - 220, textY + 22, 160, 16, juce::Justification::centredRight);


    // ========== CRÉDITOS (arriba a la derecha) ==========
    g.setColour(ballColor);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("Florchis.io", getWidth() - 130, 15, 90, 15, juce::Justification::centredRight);
}


//==============================================================================
void Panner3DAudioProcessorEditor::resized()
{
    int w = getWidth();
    int h = getHeight();
    if (w < 100 || h < 100)
        return;

    int sliderWidth = 60;
    int sliderHeight = 150;
    int rightMargin = 30;

    if (depthSlider)
    {
        depthSlider->setBounds(w - sliderWidth - rightMargin,
            h / 2 - sliderHeight - 20,
            sliderWidth, sliderHeight);
    }

    if (depthLabel)
    {
        depthLabel->setBounds(w - sliderWidth - rightMargin,
            h / 2 + sliderHeight - 40,
            sliderWidth, 40);
    }

    if (volumeSlider)
    {
        volumeSlider->setBounds(w - sliderWidth - rightMargin,
            h / 2 + 20,
            sliderWidth, sliderHeight);
    }

    if (volumeLabel)
    {
        volumeLabel->setBounds(w - sliderWidth - rightMargin,
            h / 2 + sliderHeight - 20,
            sliderWidth, 40);
    }
}

//==============================================================================
void Panner3DAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    int w = getWidth();
    int h = getHeight();
    if (w < 100 || h < 100) return;

    juce::Rectangle<int> padArea(50, 50, w - 150, h - 150);
    if (padArea.contains(event.getPosition()))
    {
        isDragging = true;
        updateParametersFromMousePosition(event.position);
    }
}

void Panner3DAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (isDragging)
        updateParametersFromMousePosition(event.position);
}

void Panner3DAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;
    repaint();
}

void Panner3DAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Obtenemos el valor actual de profundidad
    float currentDepth = processor.depthParam->get();

    // Calculamos el nuevo valor. La rueda hacia arriba (deltaY > 0) acerca la fuente.
    float newDepth = currentDepth + wheel.deltaY * 0.05f; // Ajusta 0.05f para la sensibilidad

    // Limitamos el valor entre 0 (cerca) y 1 (lejos) y lo aplicamos
    *processor.depthParam = juce::jlimit(0.0f, 1.0f, newDepth);

    // Opcional: Actualiza el slider horizontal de respaldo si lo tienes
    if (depthSlider)
        depthSlider->setValue(newDepth, juce::dontSendNotification);

    // Forzamos el repintado para que se actualice la interfaz
    repaint();
}

void Panner3DAudioProcessorEditor::updateParametersFromMousePosition(const juce::Point<float>& position)
{
    int w = getWidth();
    int h = getHeight();
    if (w < 100 || h < 100) return;

    juce::Rectangle<int> padArea(50, 50, w - 150, h - 150);

    float clampedX = juce::jlimit((float)padArea.getX(), (float)padArea.getRight(), position.x);
    float clampedY = juce::jlimit((float)padArea.getY(), (float)padArea.getBottom(), position.y);

    float relativeX = (clampedX - padArea.getX()) / (float)padArea.getWidth();
    float relativeY = (clampedY - padArea.getY()) / (float)padArea.getHeight();

    dragPosition = { relativeX, relativeY };

    // X = Paneo (-1 a 1)
    float panValue = (relativeX * 2.0f) - 1.0f;

    // Y = VOLUMEN (0 a 1) - Invertido: arriba = más volumen
    float volumeValue = 1.0f - relativeY;
    // Aplicar curva exponencial para mejor respuesta auditiva
    // Valores pequeños suben más rápido al principio
    volumeValue = volumeValue * volumeValue;  // Curva cuadrática

    // Asignar directamente a los parámetros
    *processor.panXParam = juce::jlimit(-1.0f, 1.0f, panValue);
    *processor.volumeParam = juce::jlimit(0.0f, 1.0f, volumeValue);

    // Debug
    DBG("Pan: " << panValue << " | Volume: " << volumeValue);

    repaint();
}