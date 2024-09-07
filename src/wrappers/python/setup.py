from setuptools import setup, find_packages

setup(
    packages=["libmipflooding"],
    package_dir={"libmipflooding": "."},
    package_data={"libmipflooding": ["binaries/libmipflooding.dll", "binaries/libmipflooding.h"]}
)
