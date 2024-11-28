#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() 
    : state(Unprimed),
      openButton("Open"),
      playButton("Play"),
      stopButton("Stop"),
      freezeButton("Freeze"),
      circularBufferSize(0),
      currentBufferReadIndex(0),
      currentBufferWriteIndex(0),
      freezeDuration(0.1)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (200, 200);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (0, 2);
    }
    
    openButton.onClick = [this] { openButtonClicked(); };
    addAndMakeVisible(&openButton);
    
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setEnabled(false);
    addAndMakeVisible(&playButton);
    
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled(false);
    addAndMakeVisible(&stopButton);
    
    freezeButton.onClick = [this] { freezeButtonClicked(); };
    freezeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::lightblue);
    freezeButton.setEnabled(false);
    addAndMakeVisible(&freezeButton);
    
    formatManager.registerBasicFormats();
    
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    circularBufferSize = static_cast<int>(freezeDuration * sampleRate);
    circularBuffer.setSize(2, circularBufferSize); // Stereo (2 channels)
    circularBuffer.clear();
    currentBufferWriteIndex = 0;
    currentBufferReadIndex = 0;
    transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::openButtonClicked()
{
    // Choose a file
    juce::FileChooser chooser ("Choose a wav or aiff file", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.wav; *.aiff; *.mp3");
    // If the user chooses a file
    if (chooser.browseForFileToOpen()) {
        juce::File myFile = chooser.getResult();
        // Read file
        juce::AudioFormatReader* reader = formatManager.createReaderFor(myFile);
        
        if (reader)
        {
            std::unique_ptr<juce::AudioFormatReaderSource> tempSource (new juce::AudioFormatReaderSource (reader, true));
            
            transport.setSource(tempSource.get());
            transportStateChanged(Stopped);
            
            playSource.reset(tempSource.release());
        }
    }
}

void MainComponent::playButtonClicked()
{
    transportStateChanged(Starting);
}

void MainComponent::stopButtonClicked()
{
    transportStateChanged(Stopping);
}

void MainComponent::freezeButtonClicked()
{
    transportStateChanged(Freezing);
}

void MainComponent::transportStateChanged(TransportState newState)
{
    if (newState == state) return;
    
    state = newState;
    
    switch (state) {
        case Unprimed:
            stopButton.setEnabled(false);
            playButton.setEnabled(false);
            freezeButton.setEnabled(false);
        case Stopped:
            stopButton.setEnabled(false);
            playButton.setEnabled(true);
            freezeButton.setEnabled(false);
            transport.setPosition(0);
            break;
        case Starting:
            stopButton.setEnabled(true);
            freezeButton.setEnabled(true);
            playButton.setEnabled(false);
            transport.start();
            break;
        case Stopping:
            stopButton.setEnabled(false);
            freezeButton.setEnabled(false);
            playButton.setEnabled(true);
            transport.stop();
            break;
        case Freezing:
            stopButton.setEnabled(true);
            playButton.setEnabled(true);
            freezeButton.setEnabled(false);
            break;
    }
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;
    int numSamples = bufferToFill.numSamples;

    if (state == Freezing)
    {
        for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                int readIndex = (currentBufferReadIndex + sample) % circularBufferSize;
                float sampleValue = circularBuffer.getSample(channel, readIndex);
                buffer->setSample(channel, sample, sampleValue);
            }
        }
        currentBufferReadIndex = (currentBufferReadIndex + numSamples) % circularBufferSize;
    }
    else
    {
        transport.getNextAudioBlock(bufferToFill);

        for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                int writeIndex = (currentBufferWriteIndex + sample) % circularBufferSize;
                circularBuffer.setSample(channel, writeIndex, buffer->getSample(channel, sample));
            }
        }
        currentBufferWriteIndex = (currentBufferWriteIndex + numSamples) % circularBufferSize;
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    openButton.setBounds(10, 10, getWidth() - 20, 30);
    playButton.setBounds(10, 50, getWidth() - 20, 30);
    stopButton.setBounds(10, 90, getWidth() - 20, 30);
    freezeButton.setBounds(10, 130, getWidth() - 20, 30);
}
