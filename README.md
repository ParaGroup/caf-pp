# CAF-PP: C++ Actor Framework - Parallel Patterns

> Parallel pattern library for the C++ Actor Framework (CAF)

This repository is an early stage implement of efficient and optimized parallel patterns for the Actor Model.
The library especially targets multi-cores and it exploits shared-memory to efficient implement Parallel patterns.

The Parallel patterns provided so far are:
- Pipeline
- Farm
- Map
- Divide&Conquer

In the `/examples` folder, there are some base examples on how to use the provided parallel pattern inside an Actor-based application.

## Build from source

Download the repository and do the following:
```
mkdir build
cd build
../configure --prefix-path=<path-to-caf-folder>
make -j
```

To compile with the release optimizations pass the `--release` parameter to the configure script. For other configurations see the configure help.

## Dependency
- [C++ Actor Framework](https://github.com/actor-framework/actor-framework)
- [range-v3](https://github.com/ericniebler/range-v3) (automatically downloaded if not founded)

## License

MIT License
