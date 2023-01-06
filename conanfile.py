
import os

from conan import ConanFile
from conan.tools.gnu import AutotoolsToolchain, Autotools
from conan.tools.layout import basic_layout
from conan.tools.apple import fix_apple_shared_install_name
from conans import tools

required_conan_version = ">=1.56.0"

class Uniset2Conan(ConanFile):
    name = "uniset2"
    version = "2.24.2"
    buildver = "alt1"

    # Optional metadata
    license = "LGPLv2.1"
    author = "Pavel Vainerman pv@etersoft.ru"
    url = "https://github.com/Etersoft/uniset2"
    description = "library for building distributed industrial control systems"
    topics = ("industrial control system", "c++")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": True}
    generators = "PkgConfigDeps", "AutotoolsToolchain"
    requires = "poco/1.12.4", "libxml2/2.10.3", "libev/4.33", "libpqxx/7.7.4", "sqlite3/3.40.1", "mosquitto/2.0.15", "openssl/1.1.1q", "libmysqlclient/8.0.30", "omniorb/4.2.4"
    # "sigc++2/2.10.8", "librrd/1.8.0"

    exports_sources = "*.*", "src/*", "lib/*", "contrib/*", "include/*", "tests/*", "docs/*", "wrappers/*", "Utilities/*", "extensions/*", "IDL/*", "testsuite/*", "conf/*", "COPYING", "README*", "AUTHORS", "Changelog", "NEWS"

    # def source(self):
    #     source_url = "https://github.com/Etersoft/uniset2/archive/refs/tags/%s-%s.tar.gz" % ( self.version, self.buildver )
    #     tools.get(source_url)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        basic_layout(self)

    def generate(self):
        at_toolchain = AutotoolsToolchain(self, prefix="/usr")
        at_toolchain.generate()

    def build(self):
        autotools = Autotools(self)
        autotools.autoreconf(args=["-fiv"])
        autotools.configure(args=["--enable-maintainer-mode","--disable-dependency-tracking","--libdir=${prefix}/lib64"])
        autotools.make()

    def package(self):
        autotools = Autotools(self)
        autotools.install()
        fix_apple_shared_install_name(self)

    def package_info(self):
        self.cpp_info.libs = ["uniset2"]
