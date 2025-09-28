#include "daisy_seed.h"
#include "daisysp.h"
#include <memory>

#include "dsp/ImpulseResponse.h"
#include "resources/ir.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;

// --------------------------- CONSTANTS ---------------------------------- 

#define numChannels 1 // number of input/output audio channels (currently only mono)
#define numFrames 4   // number of samples processed per audio callback


// -------------------------- FUNCTION DECS -------------------------------

void AllocateIOPointers();
void DeallocateIOPointers();
void PrepareIOPointers();
void PrepareBuffers();
void StageIR();
void ApplyDSPStaging();
void ProcessInput(const float* const* inputs);
void FallbackDSP();

// -------------------------- GLOBAL VARIABLES ----------------------------

// Impulse response (IR) buffers: one active, one staged (for safe swapping)
std::unique_ptr<dsp::ImpulseResponse> mIR;
std::unique_ptr<dsp::ImpulseResponse> mStagedIR;

// Input/output sample arrays for block processing
std::vector<double> mInputArray;
std::vector<double> mOutputArray;

// Raw pointers (allocated manually, see PrepareIOPointers)
double* mInputPointers  = nullptr;
double* mOutputPointers = nullptr;


// ------------------------- MAIN APPLICATION ----------------------------

// Main audio processing callback
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	ProcessInput(in);   // Copy samples from hardware input -> mInputArray
    FallbackDSP();      // For now, simply copy input to output

	// Output: pass input channels straight through
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = in[0][i]; // left channel
		out[1][i] = in[1][i]; // right channel
	}
}

// Converts hardware input into double buffer with gain applied
void ProcessInput(const float* const* inputs)
{
  const double gain = /* adjustable input gain (currently fixed) */ 1.0;

  for (size_t s = 0; s < numFrames; s++)
    mInputArray[s] += gain * inputs[0][s]; // only first channel used
}

// "Fallback" DSP: simply copies input buffer to output buffer
void FallbackDSP()
{
  for (auto s = 0; s < numFrames; s++)
    mOutputArray[s] = mInputArray[s];
}

// Prepare a staged impulse response from static IR data
void StageIR()
{
    /*const double sampleRate = hw.AudioSampleRate();
    dsp::wav::LoadReturnCode wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;

    try
    {
        // Load IR data from resources
        mStagedIR = std::make_unique<dsp::ImpulseResponse>(IRData, dataSize, sampleRate);
        wavState  = mStagedIR->GetWavState();
    }
    catch(std::runtime_error& e)
    {
        wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
    }

    if(wavState != dsp::wav::LoadReturnCode::SUCCESS)
    {
        // Cleanup staged IR if load failed
        mStagedIR = nullptr;
    }*/
}

// Initialize arrays and pointer-based IO buffers
void PrepareBuffers()
{
    PrepareIOPointers();

    // Resize and clear sample arrays
    mInputArray.resize(numFrames);
    std::fill(mInputArray.begin(), mInputArray.end(), 0.0);

    mOutputArray.resize(numFrames);
    std::fill(mOutputArray.begin(), mOutputArray.end(), 0.0);

    // Point raw pointers to underlying vector memory
    mInputPointers  = mInputArray.data();
    mOutputPointers = mOutputArray.data();
}

// IO pointer setup: deallocate any old buffers, then allocate new ones
void PrepareIOPointers()
{
    DeallocateIOPointers();
    AllocateIOPointers();
}

// Release old buffers
void DeallocateIOPointers()
{
    if(mInputPointers != nullptr)
    {
        delete[] mInputPointers;
        mInputPointers = nullptr;
    }
    if(mInputPointers != nullptr) // sanity check
        throw std::runtime_error("Failed to deallocate input buffer!");

    if(mOutputPointers != nullptr)
    {
        delete[] mOutputPointers;
        mOutputPointers = nullptr;
    }
    if(mOutputPointers != nullptr)
        throw std::runtime_error("Failed to deallocate output buffer!");
}

// Allocate new buffers (size = numChannels)
void AllocateIOPointers()
{
    if(mInputPointers != nullptr)
        throw std::runtime_error("Input buffer already allocated!");
    mInputPointers = new double[numChannels];
    if(mInputPointers == nullptr)
        throw std::runtime_error("Failed to allocate input buffer!");

    if(mOutputPointers != nullptr)
        throw std::runtime_error("Output buffer already allocated!");
    mOutputPointers = new double[numChannels];
    if(mOutputPointers == nullptr)
        throw std::runtime_error("Failed to allocate output buffer!");
}

// Promote staged IR to active IR
void ApplyDSPStaging()
{
  if (mStagedIR != nullptr)
  {
    mIR       = std::move(mStagedIR);
    mStagedIR = nullptr;
  }
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(numFrames); // number of samples per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

	StageIR();           // load impulse response
	PrepareBuffers();    // allocate sample buffers
	ApplyDSPStaging();   // make IR active

	hw.StartAudio(AudioCallback);

	while(1) {} // main loop (idle)
}
