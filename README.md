# IMP

## Examples

- https://youtu.be/lbRJJb_heDc
- https://youtu.be/XslSMH4532w

## Compiling

- Download FMOD Programmers API (you'll need a license, but it's free) and put in the `deps` folder
- Make sure you have CMake >= 3.8
- From inside the `build` folder, run `cmake .. && make` to populate the root `bin` folder.

Now you can run it at `../bin/imp`

## Milestones

1. Proof of concept
    - [x] basic design
    - [x] cross compilation
1. Find the right abstractions
    - [ ] synth
    - [ ] composition
    - [ ] external use
    - [ ] runtime-configurable
1. Bindings
    - [ ] C++
    - [ ] C#
    - [ ] JavaScript
    - [ ] Rust
    - [ ] Python
1. Composition
    - [ ] event format
    - [ ] idea pool
    - [ ] dynamics curves
    - [ ] chord sequences
    - [ ] analysis and embellishments
1. Synth
    - [ ] DSP, FFTs etc
    - [ ] various generators
    - [ ] audio graph
1. Runtime configuration
    - [ ] CLI tool
    - [ ] HTTP server + web gui
1. No dependencies
    - [ ] Windows
    - [ ] OSX
    - [ ] various Unix

## License

You are bound to whatever the FMOD license says.
In addition, all responsibility is on you, the library user, to obey said licenses.
The author can in no way be held responsible for license violation by others than himself.
You can do whatever you want with my code except taking credit for it.
