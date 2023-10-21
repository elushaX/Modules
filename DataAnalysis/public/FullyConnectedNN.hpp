#pragma once

#include "Buffer.hpp"
#include "DataAnalysisCommon.hpp"

namespace tp {
  class FullyConnectedNN {
  public:
    struct Layer {
      halnf mBias;
      Buffer<halnf> mWeights;
    };

  public:
    FullyConnectedNN() = default;

  private:
    Buffer<Layer> mLayers;
  };
};
