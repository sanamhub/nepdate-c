# Overlay port for the core. When the port runs from this repository tree, NEPDATE_C_SOURCE points
# at the repository root (the CI package job sets it); the GitHub archive is used otherwise.

vcpkg_minimum_required(VERSION 2024-05-24)

if(DEFINED ENV{NEPDATE_C_SOURCE})
    get_filename_component(SOURCE_PATH "$ENV{NEPDATE_C_SOURCE}" ABSOLUTE)
else()
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO sanamhub/nepdate-c
        REF "v${VERSION}"
        SHA512 45c3da3a2160e9c0eb4d75286a12483ec61b2e285c043944ef1c2752d262d2f1d329c0086630c4b7d1b12a9654b67bdbd0c43ae57ab8003484392e81ac3485df
        HEAD_REF main
    )
endif()

set(options
    -DNEPDATE_C_BUILD_TESTS=OFF
    -DNEPDATE_C_BUILD_TOOLS=OFF
    -DNEPDATE_C_BUILD_PATRO=OFF
    -DNEPDATE_C_BUILD_BENCH=OFF
)
if("without-tm" IN_LIST FEATURES)
    list(APPEND options -DNEPDATE_C_WITH_TM=OFF)
endif()
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS ${options}
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/nepdate-c)

# Headers are shared between configurations; the debug tree keeps only its library.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
