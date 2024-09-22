# libmipflooding, a Mip Flooding C++ library

C++ implementation of the Mip Flooding algorithm, presented by Sean Feeley at GDC 2019 in the talk 
_Interactive Wind and Vegetation in “God of War”_. [YouTube](https://www.youtube.com/watch?v=MKX45_riWQA&t=2954s)

Mip Flooding is a high performance alternative to edge padding / dilation and is not biased towards the edge color. It also improves the compressibility of output images, by only retaining a level of detail that is relevant for mip map generation.

Features implemented in this library:
* 8/16/32 bit input images and coverage masks (uint8/uint16/float32), and theoretically up to 8 image channels 
* selective channel processing, e.g. you might want to preserve the original alpha channel
* coverage input either as separate texture, or the alpha channel (or any last channel for that matter)
* can optionally scale the last channel unweighted (regular box filtering)
* can generate weighted mip maps, and optionally re-normalize vectors (for normal maps). Slerp scaling is planned but not yet implemented.
* includes C-style function exports for easier interfacing with non-C++ code (e.g. Python)

This is a pre-release. I would love to get your feedback!

### Documentation
The API is documented in-code, and here:
* [Documentation](https://level3manatee.github.io/libmipflooding/)


### Python wrapper

A Python wrapper is included, which processes NumPy arrays and mainly implements two functions:
* flood_image: mip-floods an image
* generate_mips: Generate mip maps using mip flooding 

The python wrapper is installable using pip:

```shell
pip install <path_to_libmipflooding>/wrappers/python/
```


### Building it yourself
(I don't have an ARM64 machine, this is only tested with AMD64 so far)

#### Windows
Use your favorite IDE (Visual Studio, Rider, ...) and open the Solution. Building should just work.

#### Linux (and possibly Mac)
Use CMake and either gcc or Clang

```shell
mkdir Build
cd Build
cmake ../
cmake --build .
```

Use `cmake ../ -DCMAKE_BUILD_TYPE=Debug` to enable printing of debug information. 


### TODO
#### High priority
* input assertions and graceful failure

#### Medium priority
* Normal map scaling using slerp instead of just re-normalizing
* Tests
* building Python package completely from source, without pre-packaged binaries  
