# Sample Target

This directory is a tiny stand-in for "some other C++ project" that you want to benchmark with `profilex`.

It is intentionally separate from the `profilex` source tree shape:

- `profilex` lives at the repo root
- the target program lives in `examples/sample_target`
- when you run `profilex` from inside this directory, benchmark data is stored in `examples/sample_target/.perf_runs/`

Build the sample target:

```sh
cmake -S examples/sample_target -B examples/sample_target/.build
cmake --build examples/sample_target/.build
```

Run the target directly:

```sh
./examples/sample_target/.build/sample_bench 400000
```

Run `profilex` against it:

```sh
cd examples/sample_target
../../.build/profilex run --name baseline --runs 6 --warmup 2 -- ./.build/sample_bench 400000
../../.build/profilex run --name candidate --runs 6 --warmup 2 -- ./.build/sample_bench 700000
../../.build/profilex compare baseline candidate
```

Or use the helper script from the repo root:

```sh
./scripts/example_target_demo.sh ./.build/profilex
```
