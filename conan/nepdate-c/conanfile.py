import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get


class NepdateCConan(ConanFile):
    name = "nepdate-c"
    version = "0.1.0-beta.1"
    license = "MIT"
    description = "Bikram Sambat (Nepali calendar) dates for C11: BS to AD and back, arithmetic, formatting, parsing and time interop."
    homepage = "https://github.com/sanamhub/nepdate-c"
    topics = ("nepali", "calendar", "bikram-sambat", "date", "c")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_tm": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_tm": True,
    }
    no_copy_source = True

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        # A C library: the C++ standard library and standard setting do not apply.
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        cmake_layout(self)

    # Conan's check_min_cstd is a C++ standard helper; this is a C library and the CMake project
    # enforces C11 with C_STANDARD 11.

    def source(self):
        local = os.environ.get("NEPDATE_C_SOURCE")
        if local:
            copy(self, pattern="*", src=local, dst=self.source_folder)
        else:
            get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.variables["NEPDATE_C_WITH_TM"] = self.options.with_tm
        tc.variables["NEPDATE_C_BUILD_PATRO"] = "OFF"
        tc.variables["NEPDATE_C_BUILD_TESTS"] = "OFF"
        tc.variables["NEPDATE_C_BUILD_TOOLS"] = "OFF"
        tc.variables["NEPDATE_C_BUILD_BENCH"] = "OFF"
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "nepdate-c")
        self.cpp_info.set_property("cmake_target_name", "nepdate-c::nepdate")
        self.cpp_info.set_property("pkg_config_name", "nepdate-c")
        self.cpp_info.libs = ["nepdate-c"]
        if not self.options.shared:
            self.cpp_info.defines.append("NEPDATE_STATIC")
        if not self.options.with_tm:
            self.cpp_info.defines.append("NEPDATE_C_WITH_TM=0")
