# Overlay port for the patro library. It finds the installed nepdate-c package and builds the
# standalone patro project.

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

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}/patro")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/nepdate-c-patro)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
