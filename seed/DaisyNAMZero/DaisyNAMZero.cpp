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
#define numFrames 64  // number of samples processed per audio callback


// -------------------------- FUNCTION DECS -------------------------------

void PrepareBuffers();
void StageIR();
void ApplyDSPStaging();
void ProcessInput(const float* const* inputs);
void FallbackDSP();
float clip(float n, float lower, float upper);


// -------------------------- GLOBAL VARIABLES ----------------------------

// Impulse response (IR) buffers: one active, one staged (for safe swapping)
std::unique_ptr<dsp::ImpulseResponse> mIR;
std::unique_ptr<dsp::ImpulseResponse> mStagedIR;

// Input/output sample arrays for block processing
std::vector<double> mInputArray;
std::vector<double> mOutputArray;

// Raw pointers 
double* mInputPointers  = mInputArray.data();
double* mOutputPointers = mOutputArray.data();

//bool enableModel = true;
bool enableIR = false;

// Switches
//Switch toggleModel;
Switch toggleIR;

// ------------------------- MAIN APPLICATION ----------------------------

// Main audio processing callback
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	ProcessInput(in);   // Adjust input gain
    FallbackDSP();      // For now, simply copy input to output

	// Process IR
    double* irPointers = mOutputPointers;
    if(mIR != nullptr && enableIR){
         irPointers = mIR->Process(mInputPointers, size);
	}

	// Output result
	for(size_t i = 0; i < size; i++){
		out[0][i] = clip(irPointers[i], -1.0f, 1.0f);
		//out[0][i] = in[0][i]; // left channel straight through
	}

}

// Converts hardware input into double buffer with gain applied
void ProcessInput(const float* const* inputs)
{
  const double gain = /* adjustable input gain (currently fixed) */ 1.0;

  for (size_t s = 0; s < numFrames; s++)
    mInputArray[s] = gain * inputs[0][s]; // only first channel used
}

// "Fallback" DSP: simply copies input buffer to output buffer
void FallbackDSP()
{
  for (size_t s = 0; s < numFrames; s++)
    mOutputArray[s] = mInputArray[s];
}

// Prepare a staged impulse response from static IR data
void StageIR()
{
    const double sampleRate = hw.AudioSampleRate();
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
    }
}

void PrepareBuffers()
{
    mInputArray.assign(numFrames, 0.0);
    mOutputArray.assign(numFrames, 0.0);

    mInputPointers  = mInputArray.data();
    mOutputPointers = mOutputArray.data();
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

float clip(float n, float lower, float upper) {
  return std::max(lower, std::min(n, upper));
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

	//toggleIR.Init(seed::D7, 1000);

	//hw.StartAudio(AudioCallback);
	while(1) {

        //toggleIR.Debounce();
        //enableIR = toggleIR.Pressed();
        //hw.SetLed(enableIR); // Led is on when IR is on
    }
}
