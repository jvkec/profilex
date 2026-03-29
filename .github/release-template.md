## Summary

ProfileX `0.1.0` is the first MVP-usable release of a local, compare-first CLI for performance investigation in C++ workflows.

## Highlights

- Repeatedly benchmark any command and persist raw samples locally.
- Compare saved baseline and candidate runs with conservative verdicts.
- Install from source today or use packaged release artifacts from this release.

## Install

From source:

```sh
git clone <repo-url>
cd profilex
./scripts/install_from_source.sh
```

From a release archive:

```sh
./scripts/install_from_package.sh <artifact.tar.gz> "$HOME/.local"
```

## Verification

- `ctest --test-dir .build --output-on-failure`
- `./scripts/clean_release_validate.sh`

## Notes

- Current target platforms are macOS and Linux.
- This release is MVP-usable, but still early in overall distribution maturity.
- The verdict heuristic intentionally prefers `inconclusive` over overclaiming.
