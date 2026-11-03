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
        SHA512 a82f946cd8ba7c9e6ea7f4d6289c4004151bc2d17252f100e9d98cd81f1caf1abf71242c4b5a02e3577ec0751be6603b387649f7ba6fbe78bb506574ae718842
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
