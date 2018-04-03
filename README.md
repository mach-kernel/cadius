# cadius

BrutalDeluxe's Cadius disk imaging utility, with some maintenance and *nix support.

## Getting Started

It is recommended that you begin by [reading the official documentation](http://brutaldeluxe.fr/products/crossdevtools/cadius/index.html). Prebuilt binaries coming soon, but for now you must build from source.

View the contents of an image by using the `CATALOG` command:

```bash
cadius CATALOG ~/path/to/image.po | less
```

## Building

This tutorial covers building for most *nix flavors, or on Windows with Cygwin. Windows instructions coming soon, but you should be able to add all of the files to a new MSVC project and build it.

Ensure your system has `gcc` or `clang` and associated build tools, then clone the repository, and build it:

```bash
git clone https://github.com/mach-kernel/cadius.git
cd cadius
# GCC is default, but override with CC=clang for clang or your preferred compiler
make
./bin/release/cadius
```

## Contributions

Any and all contributions are welcome. Included is also a `cadius.pro` file you can use with [Qt Creator](http://doc.qt.io/qtcreator/) if you want an IDE with a nice GDB frontend. Be mindful of the following:

- Preserve the existing code style, especially with pointer declarations.
- As you explore the codebase, consider writing a Doxygen docstring for functions you come across / find important.
- Try to test on OS X and Linux if including new headers.
- In your PR, please add a changelog entry.

## Changelog

#### 1.4.1
- `REPLACEFILE`
  - Buffer overflow resolved [#17](https://github.com/mach-kernel/cadius/issues/17). Thanks [@sicklittlemonkey](https://github.com/sicklittlemonkey)!
  - Error on replacing a file that does not exist silenced [#16](https://github.com/mach-kernel/cadius/issues/16).
- Typo fixed. Thanks [@a2sheppy](https://github.com/a2sheppy)

#### 1.4.0
- Adds AppleSingle file format support, with initial support for data and ProDOS file info IDs 1 & 11 ([#7](https://github.com/mach-kernel/cadius/issues/7)).
- Fix path bugs on Windows ([#9](https://github.com/mach-kernel/cadius/issues/9)).
- Fix buffer overflow from [#12](https://github.com/mach-kernel/cadius/issues/12).
- Fix segfault when using `EXTRACTVOLUME`.

A big thank you to [@oliverschmidt](https://github.com/oliverschmidt) for helping test this release!

#### 1.3.2
- Maintenance release / macro cleanup.

#### 1.3.1
- Resolves timestamp bugs in [#10](https://github.com/mach-kernel/cadius/issues/10). Thanks, @a2-4am!

#### 1.3.0
- Adds ability to specify a file's `type` and `auxtype` via the filename, resolving [#4](https://github.com/mach-kernel/cadius/issues/4). A big thank you to @JohnMBrooks for testing this PR!
- `REPLACEFILE` command, `DELETEFILE` support for type/auxtype suffix. 

#### 1.2-b3
- Fix Windows build issues, make some shared OS methods static / remove from `os.c` ([@mach-kernel](https://github.com/mach-kernel)).

#### 1.2-b2
- Clean up OS macros, explicit `win32` and `posix` modules ([@mach-kernel](https://github.com/mach-kernel)).

#### 1.2-b1
- UTF-8 encode all source files.
- Initial POSIX support ([@mach-kernel](https://github.com/mach-kernel)).

#### 1.1
- Initial fork from BrutalDeluxe.

## License

CADIUS was developed by [BrutalDeluxe](http://brutaldeluxe.fr). All contributions licensed under the [GNU/GPL](https://github.com/mach-kernel/cadius/blob/master/LICENSE) and attributed to contributors. All prior / existing code attributed under the original license. [GenericMakefile](https://github.com/mbcrawfo/GenericMakefile) is licensed under the [MIT License](https://github.com/mbcrawfo/GenericMakefile/blob/master/LICENSE).
