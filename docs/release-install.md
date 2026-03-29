# Release And Install Validation

This project is still a local CLI tool first, so the release/install path is intentionally small.

For release history, see [CHANGELOG.md](../CHANGELOG.md).

## Install

Build and install with CMake:

```sh
cmake -S . -B .build
cmake --build .build
cmake --install .build --prefix "$HOME/.local"
```

If `$HOME/.local/bin` is on your `PATH`, the installed binary is:

```sh
profilex help
```

## Package

The CMake build also enables CPack, so you can generate a release archive from a built tree:

```sh
cmake --build .build --target package
```

The generated package is a `TGZ` archive in the build directory.

For tagged GitHub releases, the workflow in `.github/workflows/release.yml` is intended to build and publish these archives automatically.

## Clean Source Validation

Use the release validation script to verify a fresh build, test run, install, and package generation in a clean temporary workspace:

```sh
./scripts/clean_release_validate.sh
```

Or point it at a source tree explicitly:

```sh
./scripts/clean_release_validate.sh /path/to/profilex
```

The script copies the source tree into a temporary workspace, runs configure/build/tests, installs the binary, checks the installed `profilex`, and generates a package artifact.

## Release Checklist

- `CHANGELOG.md` should reflect the release.
- `README.md` should explain build, install, test, and usage.
- `.github/release-template.md` should be updated if the generic text is no longer accurate.
- `profilex help` should work from an installed prefix.
- `profilex --version` should report the release version.
- `cmake --build .build --target package` should succeed.
- The clean validation script should pass from a source tree with no manual setup.
