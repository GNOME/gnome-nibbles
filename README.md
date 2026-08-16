# GNOME Nibbles
A snake game for GNOME.

# Building

Whenever possible, you should use the official binary packages approved by the
supplier of your operating system, such as your Linux distribution.

## Building from a release tarball

Download the tarball from https://gitlab.gnome.org/GNOME/gtkmm/-/releases

Extract the tarball and go to the extracted directory:
```
  $ tar xjf gnome-nibbles-x.x.x.tar.bz2
  $ cd gnome-nibbles-x.x.x
```

## Building from the git repository

Clone the repository, specifying the release you want to build:
```
  $ git clone https://gitlab.gnome.org/GNOME/gnome-nibbles.git -b release-x-x gnome-nibbles
  $ cd gnome-nibbles
```

## Installing build dependencies

The easiest way to install the build dependencies is to use your operating
system’s package manager, provided it provides Nibbles 4.6 or later.
Otherwise, you will need to install the following dependencies by hand:
```
pkg-config (also known as pkgconf)
update-desktop-database
itstool
meson version >= 1.1
gsound version >= 1.0.2
C++ compiler that supports C++26 with GNU-specific language extensions (-std=gnu++26)
gtkmm version >= 4.23.1 (along with a corresponding version of gtk4)
glibmm-2.68 version >= 2.88.1 (along with a corresponding version of glib)
```

### Debian/Ubuntu using the distribution's build dependencies
Use the root user:
```
  # apt update
  # apt build-dep gnome-nibbles
```
### Fedora using the distribution's build dependencies
Use the root user:
```
  # dnf builddep gnome-nibbles
```

## Compiling & Installing
You need to use a C++ compiler that supports C++26 with GNU-specific language
extensions, such as GCC 16.
```
  $ meson setup --prefix ~/my-nibbles ../nibbles.build
```
If your default compiler does not support C++26, the meson setup stage will
fail. You can specify a compiler that supports C++26 using the CXX
environment variable:
```
  $ CXX=g++-16 meson setup --prefix ~/my-nibbles ../nibbles.build
```
To build a release version of the software instead of a debug version, add
the `--buildtype=release` option to the `meson setup` command:
```
  $ meson setup --prefix ~/my-nibbles --buildtype=release ../nibbles.build
```
or
```
  $ CXX=g++-16 meson setup --prefix ~/my-nibbles --buildtype=release ../nibbles.build
```

If Meson complains about a missing dependency, you will need to install the
dependency by hand and then repeat the appropriate command above.

Once the meson setup command completes successfully, you can compile Nibbles:
```
  $ meson compile -C ../nibbles.build
```

Build and run the tests with:
```
  $ meson test -C ../nibbles.build
```

If you have previously installed Nibbles, you can run the tests and Nibbles
directly from the command line:
```
  $ ../nibbles.build/src/nibbles_tests
  $ ../nibbles.build/src/gnome-nibbles
```

If Nibbles has not previously been installed, install it with:
```
  $ meson install -C ../nibbles.build
```
Nibbles will be installed below the directory specified by the --prefix option
above.
