import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get


class NepdateCPatroConan(ConanFile):
    name = "nepdate-c-patro"
    version = "0.1.0-beta.1"
    license = "MIT"
    description = "The patro (Nepali almanac) companion to nepdate-c: tithi, public holidays and events, BS 2077 to 2084."
    homepage = "https://github.com/sanamhub/nepdate-c"
    topics = ("nepali", "calendar", "patro", "tithi", "c")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
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

    def requirements(self):
        self.requires("nepdate-c/0.1.0-beta.1", transitive_headers=True)

    def layout(self):
        cmake_layout(self)

    def source(self):
        local = os.environ.get("NEPDATE_C_SOURCE")
        if local:
            copy(self, pattern="*", src=local, dst=self.source_folder)
        else:
            get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(build_script_folder="patro")
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "nepdate-c-patro")
        self.cpp_info.set_property("cmake_target_name", "nepdate-c::patro")
        self.cpp_info.set_property("pkg_config_name", "nepdate-c-patro")
        self.cpp_info.libs = ["nepdate-c-patro"]
        if not self.options.shared:
            self.cpp_info.defines.append("NEPDATE_PATRO_STATIC")
