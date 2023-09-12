## Template for add custom hunter config
#hunter_config(
#    package-name
#    VERSION 0.0.0-package-version
#    CMAKE_ARGS "CMAKE_VARIABLE=value"
#)

hunter_config(
    fmt
    URL  https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.tar.gz
    SHA1 e94b38a9efe0d696373a3cb1300dd24f12e2dd9c
    CMAKE_ARGS
        FMT_DOC=OFF
        FMT_TEST=OFF
        FMT_USE_CPP14=OFF # don't force c++14
    KEEP_PACKAGE_SOURCES
)
