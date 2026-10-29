# Overlay port for the core. When the port runs from this repository tree, NEPDATE_C_SOURCE points
# at the repository root (the CI package job sets it); the GitHub archive is used otherwise.
#
# The archive hash is a placeholder until the release tag exists. Compute it with:
#   curl -L -o nepdate-c.tar.gz https://github.com/sanamhub/nepdate-c/archive/refs/tags/v0.1.0-beta.1.tar.gz
#   sha512sum nepdate-c.tar.gz
# then replace PLACEHOLDER_ARCHIVE_SHA512 with the printed hash and remove this note.

vcpkg_minimum_required(VERSION 2024-05-24)

if(DEFINED ENV{NEPDATE_C_SOURCE})
    get_filename_component(SOURCE_PATH "$ENV{NEPDATE_C_SOURCE}" ABSOLUTE)
else()
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO sanamhub/nepdate-c
        REF "v${VERSION}"
        SHA512 00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 # PLACEHOLDER_ARCHIVE_SHA512
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
