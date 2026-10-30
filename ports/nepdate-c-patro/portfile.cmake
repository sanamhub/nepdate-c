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
        SHA512 45c3da3a2160e9c0eb4d75286a12483ec61b2e285c043944ef1c2752d262d2f1d329c0086630c4b7d1b12a9654b67bdbd0c43ae57ab8003484392e81ac3485df
        HEAD_REF main
    )
endif()

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}/patro")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/nepdate-c-patro)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
