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

## How to Cite
If our work is useful for your research, please cite the following paper:
```
@article{DBLP:journals/ijpp/RinaldiTSMD20,
  author    = {Luca Rinaldi and Massimo Torquati and Daniele De Sensi and Gabriele Mencagli and Marco Danelutto},
  title     = {Improving the Performance of Actors on Multi-cores with Parallel Patterns},
  journal   = {Int. J. Parallel Program.},
  volume    = {48},
  number    = {4},
  pages     = {692--712},
  year      = {2020},
  url       = {https://doi.org/10.1007/s10766-020-00663-1},
  doi       = {10.1007/s10766-020-00663-1},
  timestamp = {Tue, 29 Dec 2020 00:00:00 +0100},
  biburl    = {https://dblp.org/rec/journals/ijpp/RinaldiTSMD20.bib},
  bibsource = {dblp computer science bibliography, https://dblp.org}
}
```
## Contributors
This work has been developed by [Massimo Torquati](mailto:massimo.torquati@unipi.it) and Luca Rinaldi.

