# libmipflooding, a Mip Flooding C++ library

C++ implementation of the Mip Flooding algorithm, presented by Sean Feeley at GDC 2019 in the talk 
_Interactive Wind and Vegetation in “God of War”_. [YouTube](https://www.youtube.com/watch?v=MKX45_riWQA&t=2954s)

Python wrapper included, which mainly implements two functions:
* flood_image: mip-floods an image
* generate_mips: Generate mip maps using mip flooding 

The python wrapper is installable using pip:

```
pip install <path_to_libmipflooding>/wrappers/python/
```

This is a pre-release, more documentation and linux binaries to follow. I would love to get your feedback!

### TODO
#### High priority
* Documentation
* input assertions and graceful failure
* Build config and binaries for Linux (and possible Mac?)

#### Medium priority
* Normal map scaling using slerp instead of just re-normalizing
* Tests