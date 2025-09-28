//
//  ImpulseResponse.h
//  NeuralAmpModeler-macOS
//
//  Created by Steven Atkinson on 12/30/22.
//
// Impulse response processing

#pragma once

#include <Eigen/Dense>
#include "dspx.h"
#include "wav.h"

namespace dsp
{
class ImpulseResponse : public History
{
public:
  struct IRData;
  ImpulseResponse(const unsigned char* data, size_t dataSize, double sampleRate);
  //double** Process(double** inputs, const size_t numChannels, const size_t numFrames) override;
  double* Process(double* inputs, const size_t numFrames) override;
  IRData GetData();
  double GetSampleRate() const { return mSampleRate; };
  // TODO states for the IR class
  dsp::wav::LoadReturnCode GetWavState() const { return this->mWavState; };

private:
  // Set the weights, given that the plugin is running at the provided sample
  // rate.
  void _SetWeights();

  // State of audio
  dsp::wav::LoadReturnCode mWavState;
  // Keep a copy of the raw audio that was loaded so that it can be resampled
  std::vector<float> mRawAudio;
  double mRawAudioSampleRate;
  // Resampled to the required sample rate.
  std::vector<float> mResampled;
  double mSampleRate;

  const size_t mMaxLength = 8192;
  // The weights
  Eigen::VectorXf mWeight;
};

struct ImpulseResponse::IRData
{
  std::vector<float> mRawAudio;
  double mRawAudioSampleRate;
};

}; // namespace dsp
