# Robotic arm Project
##### this is a project made with samples from kinect v1 sdk windows

The purpose of this project is to extract poses of closed and open hands and it classify.
From the right hand position tracking, to use a robotic claw, and change claw states from open and closed by respectively 
depth image from the tracked hand 

## Build System
this project use cmake for to build itself.
## Dependencies
- C++ dependencies
  - cmake 3.2
  - Kinect SDK v1 windows
  - opencv 3.X
  - SFML2 (socket connections to training)
  - **Observation!**
    - probably will us caffee for deep learning with transfer weigths from kereas
    - samples from DNN in caffee http://caffe.berkeleyvision.org/gathered/examples/cpp_classification.html
    - here is a list of model converters https://github.com/ysh329/deep-learning-model-convertor
- Classifier
  - python 3.5
  - tensorflow 1.X
  - keras 2.X
  - pillow
  - python-opencv
