# Relativistic Starfield

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-Supported-brightgreen.svg)](https://www.opengl.org/)
[![WebAssembly](https://img.shields.io/badge/WebAssembly-Enabled-orange.svg)](https://webassembly.org/)

An interactive C++ / OpenGL / WebAssembly visualisation of what a spacecraft might see while approaching the speed of light. 

**The exact same simulation runs natively on macOS and directly in your browser.**


---

## What it Visualises

The universe is generated deterministically, ensuring that both native and web builds start from the exact same star distribution. As you accelerate towards the speed of light `c`, the engine applies real special-relativistic transformations:

* **Relativistic Doppler Shift:** Stars shift toward the blue/UV spectrum ahead of the ship and red/IR behind.
* **Relativistic Aberration:** The starfield visibly warps and crowds into the forward field of view, relative to the ship.
* **Beaming (Headlight Effect):** Intensity amplifies drastically in the direction of travel and dims behind.
* **Time Dilation:** Watch the stark divergence between the Universe's Coordinate Time and the Spacecraft's Proper Time.
* **Advanced Rendering:** Features visible-spectrum colour mapping with assumed Human Eye ranges, HDR-like tone mapping, optical bloom and a wrapped procedural universe and with a distance fog.

---

## Live Demo

View the simulation in your browser here:

👉 **[Play the Live Demo](https://YOUR_USERNAME.github.io/relativistic-starfield/)**

---

## Flight Computer & Controls

The onboard Spaceship interface exposes the simulation's state and lets tweak physical and visual parameters in real-time.

### Live Telemetry
* Current velocity (as a fraction of Speed of Light `c` and in m/s)
* Lorentz factor (`γ`) and Time-dilation factor
* Forward and backward Doppler/Beaming factors
* Distance travelled
* Coordinate time vs. Ship proper time

### Controls

| Action | Input |
| :--- | :--- |
| **Look around** | Right mouse drag |
| **Increase velocity** | `↑` (Up Arrow) |
| **Decrease velocity** | `↓` (Down Arrow) |
| **Pause / Resume** | `Spacebar` |
| **Reset flight** | `R` |

---

## Building & Running

### macOS build

Install dependencies with Homebrew:

```bash
brew install glfw glm cmake
```

Then:

```bash
make
make run
```

The native build uses GLFW, Apple's OpenGL framework, GLM and Dear ImGui for the flight computer.

### Web build

The browser version is compiled from the same C++ source with Emscripten and targets WebGL 2.

With Emscripten installed:

```bash
make web
```

The generated site is written to `build-web/`.

## Project structure

```text
relativistic-starfield/
├── .github/workflows/deploy-web.yml
├── src/main.cpp
├── shaders/star.vert
├── shaders/star.frag
├── web/shell.html
├── Makefile
├── CMakeLists.txt
├── README.md
├── LICENSE
└── docs/
```

## Physics note

This is better seen an interactive visualisation rather than a precision astrophysics package. The rendering pipeline applies special-relativistic transformations to the starfield, while the flight computer interface exposes the simulation state and key derived quantities.

## License

MIT License.