from setuptools import setup, find_packages

setup(
    packages=["libmipflooding"],
    package_dir={"libmipflooding": "."},
    package_data={"libmipflooding": [
        "binaries/libmipflooding.dll",
        "binaries/libmipflooding.so",
        "binaries/libmipflooding_extern_c.h",
        "binaries/libmipflooding_enums.h"
    ]}
)
