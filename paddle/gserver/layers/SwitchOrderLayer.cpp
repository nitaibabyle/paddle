/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "SwitchOrderLayer.h"
#include "paddle/utils/Stat.h"

namespace paddle {

REGISTER_LAYER(switch_order, SwitchOrderLayer);

bool SwitchOrderLayer::init(const LayerMap& layerMap,
                            const ParameterMap& parameterMap) {
  /* Initialize the basic parent class */
  Layer::init(layerMap, parameterMap);
  auto& img_conf = config_.inputs(0).image_conf();
  size_t inH =
      img_conf.has_img_size_y() ? img_conf.img_size_y() : img_conf.img_size();
  size_t inW = img_conf.img_size();
  size_t inC = img_conf.channels();
  inDims_ = TensorShape({0, inC, inH, inW});
  outDims_ = TensorShape(4);

  auto& reshape_conf = config_.reshape_conf();
  for (size_t i = 0; i < reshape_conf.heightaxis_size(); i++) {
    LOG(INFO) << "reshape height axis: " << reshape_conf.heightaxis(i);
    heightAxis_.push_back(reshape_conf.heightaxis(i));
  }
  for (size_t i = 0; i < reshape_conf.widthaxis_size(); i++) {
    LOG(INFO) << "reshape width axis: " << reshape_conf.widthaxis(i);
    widthAxis_.push_back(reshape_conf.widthaxis(i));
  }
  createFunction(nchw2nhwc_, "NCHW2NHWC", FuncConfig());
  createFunction(nhwc2nchw_, "NHWC2NCHW", FuncConfig());
  return true;
}

void SwitchOrderLayer::setOutDims() {
  outDims_.setDim(0, inDims_[0]);
  outDims_.setDim(1, inDims_[2]);
  outDims_.setDim(2, inDims_[3]);
  outDims_.setDim(3, inDims_[1]);
  reshapeHeight_ = 1;
  for (size_t i = 0; i < heightAxis_.size(); i++) {
    reshapeHeight_ *= outDims_[heightAxis_[i]];
  }
  output_.setFrameHeight(reshapeHeight_);
  reshapeWidth_ = 1;
  for (size_t i = 0; i < widthAxis_.size(); i++) {
    reshapeWidth_ *= outDims_[widthAxis_[i]];
  }
  output_.setFrameWidth(reshapeWidth_);
  LOG(INFO) << "outDims: " << outDims_[0] << "; " << outDims_[1] << ";"
            << outDims_[2] << ";" << outDims_[3];
}

void SwitchOrderLayer::setInDims() {
  MatrixPtr input = inputLayers_[0]->getOutputValue();
  size_t batchSize = input->getHeight();
  inDims_.setDim(0, batchSize);

  int h = inputLayers_[0]->getOutput().getFrameHeight();
  if (h != 0) inDims_.setDim(2, h);
  int w = inputLayers_[0]->getOutput().getFrameWidth();
  if (w != 0) inDims_.setDim(3, w);
  int totalCount = input->getElementCnt();
  int channels = totalCount / (inDims_[0] * inDims_[2] * inDims_[3]);
  if (channels != 0) inDims_.setDim(1, channels);
  LOG(INFO) << "inDims: " << inDims_[0] << "; " << inDims_[1] << ";"
            << inDims_[2] << ";" << inDims_[3];
}

void SwitchOrderLayer::forward(PassType passType) {
  Layer::forward(passType);
  setInDims();
  setOutDims();
  resetOutput(outDims_[0], outDims_[1] * outDims_[2] * outDims_[3]);
  if (heightAxis_.size() > 0) {
    getOutputValue()->reshape(reshapeHeight_, reshapeWidth_);
  }

  // switch NCHW to NHWC
  BufferArgs inputs;
  BufferArgs outputs;
  inputs.addArg(*getInputValue(0), inDims_);
  outputs.addArg(*getOutputValue(), outDims_);
  nchw2nhwc_[0]->calc(inputs, outputs);
  // forwardActivation();
}

void SwitchOrderLayer::backward(const UpdateCallback& callback) {
  (void)callback;
  // backwardActivation();

  // switch NHWC to NCHW
  BufferArgs inputs;
  BufferArgs outputs;
  inputs.addArg(*getOutputGrad(), outDims_);
  outputs.addArg(*getInputGrad(0), inDims_, ADD_TO);
  nhwc2nchw_[0]->calc(inputs, outputs);
}
}  // namespace paddle
