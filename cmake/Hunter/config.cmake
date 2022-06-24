## Template for add custom hunter config
#hunter_config(
#    package-name
#    VERSION 0.0.0-package-version
#    CMAKE_ARGS "CMAKE_VARIABLE=value"
#)

hunter_config(
    fmt
    URL  https://github.com/fmtlib/fmt/archive/refs/tags/8.1.1.tar.gz
    SHA1 9577d6de8f4e268690b099976810ade9ebef5fb5
    CMAKE_ARGS
        FMT_DOC=OFF
        FMT_TEST=OFF
        FMT_USE_CPP14=OFF # don't force c++14
    KEEP_PACKAGE_SOURCES
)
