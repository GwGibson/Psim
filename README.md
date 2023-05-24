<h1 align="center">psim</h1>

<p align="center">
  <b>Generalized 2D phonon transport using a Monte Carlo method</b>
</p>

<p align="center">
  <img title="v1.0" alt="v1.0" src="https://img.shields.io/badge/version-v1.0-informational?style=flat-square">
  <img title="MIT License" alt="license" src="https://img.shields.io/badge/license-MIT-informational?style=flat-square">
 <img title="C++17" alt="C++17" src="https://img.shields.io/badge/c++-17-informational?style=flat-square"><br/>
 <a href="https://github.com/GwGibson/psim/actions/workflows/ci.yml">
    <img src="https://github.com/GwGibson/psim/actions/workflows/ci.yml/badge.svg"/>
    </a>
 <a href="https://github.com/GwGibson/psim/actions/workflows/clang-format-check.yml">
    <img src="https://github.com/GwGibson/psim/actions/workflows/clang-format-check.yml/badge.svg"/>
    </a>
</p>

---

## Table of Contents

- [Overview](#overview)
- [Feature Set](#feature-set)
- [Requirements](#requirements)
  - [Docker](#docker)
- [Build Instructions](#build-instructions)
  - [(1) Specify the compiler using environment variables](#1-specify-the-compiler-using-environment-variables)
    - [Commands for setting the compilers](#commands-for-setting-the-compilers)
  - [(2) Configure your build](#2-configure-your-build)
    - [(2a) Configuring via cmake](#2a-configuring-via-cmake)
    - [(2b) Configuring via ccmake](#2b-configuring-via-ccmake)
  - [(3) Build the project](#3-build-the-project)
- [Python/JSON examples](#pythonjson-examples)
- [Output Format](#output-format)
  - [Steady-State](#steady-state)
  - [Periodic \& Transient](#periodic--transient)
- [Visualizations](#visualizations)
  - [Steady-State Visualization (kinked\_demo.py)](#steady-state-visualization-kinked_demopy)
  - [Steady-State Visualization (linear\_sides\_demo.py)](#steady-state-visualization-linear_sides_demopy)
  - [Periodic Visualization (linear\_sides\_demo.py)](#periodic-visualization-linear_sides_demopy)
  - [Transient Visualization (linear\_sides\_demo.py)](#transient-visualization-linear_sides_demopy)
  - [Phasor Visualization](#phasor-visualization)
- [Next Steps](#next-steps)
  - [Model Creation](#model-creation)
  - [Material Interfaces](#material-interfaces)
  - [Optical Phonons](#optical-phonons)
  - [Steady-State Detection](#steady-state-detection)
  - [Geometrical Limitations](#geometrical-limitations)
  - [Visualization Enchancements](#visualization-enchancements)
- [References](#references)

## Overview

The program is designed to simulate phonon transport in nanostructures using a Monte Carlo method, aiming to better understand such structures' thermal properties. To speed up computations, the program uses C++17 parallelism features, including parallel algorithms and execution policies.

The Monte Carlo method simulates phonon transport by modelling the movement of individual phonons through the nanostructure. The program tracks the path of each phonon, as well as the amount of energy it carries. It uses this information to calculate the thermal conductivity and temperature of the nanostructure.

The program is intended for researchers and engineers working in nanotechnology and can be customized to simulate various types of nanostructures with different material properties.

---

## Feature Set

- Nanostructures are specified by a `.json` file generated in Python.
- Multiple `.json` files can be provided as input.
- Theoretically, unrestricted 2D geometrical configurations are possible, although there are practical limitations. See [Next Steps: Model Creation](#next-steps).
- The program can be run in "full" simulation mode using the methodology described by [1].
- The program can also be run in "deviational" mode using the methodologies described in [2] and [3].
- The "deviational" mode is recommended unless you are doing low-temperature (below ~100 K) simulations.
- Three different simulation types are available:
  - Steady-state: provides a snapshot of the system once it reaches the steady-state.
  - Periodic: allows the user to visualize how the system reaches the steady-state.
  - Transient: allows the user to model bursts of heat throughout the system.
  
---

## Requirements

The software has no dependencies or third-party requirements, although you may need [TBB](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb.html#gs.tkvwob) if your compiler does not support C++17 parallel algorithms and execution policy. CMake 3.15+ is required.

See [tests](https://github.com/GwGibson/psim/actions/runs/4583903242) for working OS/compiler combinations.

### Docker

A Docker image is set up, but it is tailored toward development. It is not ideal for running simulations.

If you have [Docker](https://www.docker.com/) installed, you can run this
in your terminal, when the Dockerfile is inside the `.devcontainer` directory:

```bash
docker build -f ./.devcontainer/Dockerfile --tag=psim:latest .
```

This command will start building the Docker image, which may take several minutes
if this is your first time creating the image. Once this process completes,
run the following command to create a docker container:

```bash
docker run -it -v absolute_path_on_host_machine:/workspaces/psim psim:latest psim:latest
```

This command will put you in a `bash` session in a Ubuntu 22.04 Docker container with many dependencies and tools at your disposal.
You will be in a directory that contains a copy of the psim, and your local copy will be mounted directly in the Docker image. See Docker volumes documentation for more information. You will also have `g++-11` and `clang++-14` installed as the default versions of `g++` and `clang++`.

Once you exit the container, you can reaccess the container by using the command:

```bash
docker exec -it container_id /bin/bash
```

Most IDEs support containerized development, and Visual Studio Code makes the process painless. You can navigate to the project root directory and enter the command:

```bash
code .
```

## Build Instructions

A full build has different steps:

1) Specifying the compiler using environment variables
2) Configuring the project
3) Building the project

If you change the source code for the subsequent builds, you only need to repeat the last step.

### (1) Specify the compiler using environment variables

By default (if you don't set environment variables `CC` and `CXX`), the system default compiler will be used.

CMake uses the environment variables CC and CXX to decide which compiler to use. So to avoid conflict issues, only specify the compilers using these variables.

#### Commands for setting the compilers

- Debian/Ubuntu/MacOS:

  Set your desired compiler (`clang`, `gcc`, etc.):

  - Temporarily (only for the current shell)

    Run one of the following in the terminal:

    - clang

      CC=clang CXX=clang++

    - gcc

      CC=gcc CXX=g++

  - Permanent:

    Open `~/.bashrc` using your text editor:

    ```bash
    gedit ~/.bashrc
    ```

    Add `CC` and `CXX` to point to the compilers:

    ```bash
    export CC=clang
    export CXX=clang++
    ```

  Save and close the file.

- Windows:

  - Permanent:

    Run one of the following in PowerShell:

    - Visual Studio generator and compiler (cl)

    ```bash
    [Environment]::SetEnvironmentVariable("CC", "cl.exe", "User")
    [Environment]::SetEnvironmentVariable("CXX", "cl.exe", "User")
    refreshenv
    ```

    - Set the architecture using [vcvarsall](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=vs-2019#vcvarsall-syntax):

      ```bash
      vcvarsall.bat x64
      ```

    - clang

      ```bash
      [Environment]::SetEnvironmentVariable("CC", "clang.exe", "User")
      [Environment]::SetEnvironmentVariable("CXX", "clang++.exe", "User")
      refreshenv
      ```

    - gcc

      ```bash
      [Environment]::SetEnvironmentVariable("CC", "gcc.exe", "User")
      [Environment]::SetEnvironmentVariable("CXX", "g++.exe", "User")
      refreshenv
      ```

  - Temporarily (only for the current shell):

    ```bash
    $Env:CC="clang.exe"
    $Env:CXX="clang++.exe"
    ```

### (2) Configure your build

To configure the project, you could use `cmake`, `ccmake` or `cmake-gui`.

#### (2a) Configuring via cmake

With Cmake directly:

```bash
cmake -S . -B ./build
```

Cmake will automatically create the `./build` folder if it does not exist and configure the project.

Instead, if you have CMake version 3.21+, you can use one of the configuration presets listed in the CmakePresets.json file.

```bash
cmake . --preset <configure-preset>
cmake --build
```

#### (2b) Configuring via ccmake

With the Cmake Curses Dialog Command Line tool:

```bash
ccmake -S . -B ./build
```

Once `ccmake` has finished setting up, press 'c' to configure psim,
press 'g' to generate and 'q' to quit.

### (3) Build the project

Once you have selected all the options you would like to use, you can build psim:

```bash
cmake --build ./build
```

For Visual Studio, give the build configuration (Release, RelWithDeb, Debug, etc.) like the following:

```bash
cmake --build ./build -- /p:configuration=Release
```

---

## Python/JSON examples

In the `python_sim` folder, there are 3 `.py` files. Running these files should produce the `.json` files in the `.json` folder. These are the files you can pass into the psim program. For example, if you put the `.json` files in the same directory as the psim executable, then you can run all five simulations using the command:

```bash
./psim.out *.json
```

Some pre-built configurations can be found at `psim_python\psim\pre_builts.py`. This can give an idea of how to construct your systems, but this can be a complicated process depending on the intricacies of your desired configuration.

---

## Output Format

The table below outlines the data found in the output files.

| Quantity  | Description                                 |
|-----------|---------------------------------------------|
| temp      | Temperature              |
| temp std  | Standard deviation of the temperature       |
| x-flux    | Phonon flux in the x-direction              |
| x-flux std| Standard deviation of the phonon flux in x  |
| y-flux    | Phonon flux in the y-direction              |
| y-flux std| Standard deviation of the phonon flux in y  |

### Steady-State

The first line contains details about the run.
The remaining lines follow the format:

| temp | temp std | x-flux | x-flux std | y-flux | y-flux std |
|------|----------|--------|------------|--------|------------|

Each entry corresponds to the associated sensor in the .json file. That is, the first entry corresponds with the first sensor, etc.

### Periodic & Transient

The first line contains details about the run.
The remaining lines follow the format:

|simulation step|
|------|

|number of sensors or number of measurements that follow this line|
|------|

| temp | temp std | x-flux | x-flux std | y-flux | y-flux std |
|------|----------|--------|------------|--------|------------|

This pattern is repeated to give chronological snapshots of the system and these can be strung together to see the system evolution.

---

## Visualizations

In the visualization process, the original JSON file is combined with the output file to generate plots. Typically, the initial temperature and values specified in the JSON file are replaced by the corresponding values from the simulation results file. Linear systems can be handled more simply, as can be seen in `plotly_plotter.py`.

The following visualizations can be built using the code in `matplotlib_plotter.py`. The code to create these visualizations can be found in the corresponding demo files. The `plotly_plotter.py` file generates interactive `HTML` plots, but it is only helpful for simple linear systems.  

### Steady-State Visualization (kinked_demo.py)

<img src="https://raw.githubusercontent.com/GwGibson/psim/main/psim_python/visualizations/kinked_ss_demo.png" alt="kinked">

### Steady-State Visualization (linear_sides_demo.py)

<img src="https://raw.githubusercontent.com/GwGibson/psim/main/psim_python/visualizations/linear_demo_sides.png" alt="steady-state">

### Periodic Visualization (linear_sides_demo.py)

<img src="https://raw.githubusercontent.com/GwGibson/psim/main/psim_python/visualizations/linear_sides_demo_per.gif" alt="periodic">

### Transient Visualization (linear_sides_demo.py)

<img src="https://raw.githubusercontent.com/GwGibson/psim/main/psim_python/visualizations/linear_sides_demo_trans.gif" alt="trans">

### Phasor Visualization

<img src="https://raw.githubusercontent.com/GwGibson/psim/main/psim_python/visualizations/phasor.gif" alt="phasor">

---

## Next Steps

### Model Creation

- Specifying complicated nanostructures using the existing Python code can be tedious.
- The current implementation could be better for modelling complex structures, particularly for users unfamiliar with Python programming.
- Future program versions may include a more user-friendly interface for specifying and visualizing nanostructures.
- This could include a graphical user interface (GUI) that allows users to create and modify nanostructures using a visual editor.
- Other potential improvements include importing models from other software packages.
- These improvements would make the program more accessible to a broader range of users and facilitate modelling more complex and realistic nanostructures.

### Material Interfaces

- The current software allows the straightforward creation of systems consisting of various materials.
- However, the interface interaction between different materials still needs to be handled.
- Currently, the only interface consideration is that backscattering occurs if an incoming phonon is incompatible with the new material. Improvements can be made starting at line 89 in `surface.cpp`.
- Complex interface interactions can be addressed using promising techniques in [4] and [5].

### Optical Phonons

- The model does not consider optical phonon branches.

### Steady-State Detection

- Software currently requires the user to specify simulation duration.
- Implementing a steady-state detection mechanism is a better approach.
- The mechanism should stop the program once it detects that the system has reached a steady state.
- Stopping the simulation can be done when the flux across each cell is constant within some margin of error for a certain amount of time.

### Geometrical Limitations

- Allowing for unrestricted 3D geometries would enhance the existing model.
- A relatively simple intermediate step would allow the user to specify a single measurement that represents the z-axis dimension for the entire system.
- This would let the user set emitting surfaces and alter surface specularity in that dimension.

### Visualization Enchancements

- Improve structure of visualization code
- Refactor and optimize existing code
- Modularize visualization code
  - Separate into reusable functions
  - Create better-designed classes
- Enhance maintainability and extensibility
- Explore alternative visualization libraries and techniques
  - Improve plot quality and interactivity
  - Ensure easy analysis of simulation results
- Improve overall performance of plotting utilities
  - Address long video generation times for periodic and transient simulations

---

## References

[1]: D. Lacroix, K. Joulain, and D. Lemonnier, “Monte Carlo transient phonon transport in silicon and germanium at nanoscales,” Physical Review B, vol. 72, no. 6, p. 064305, 2005.

[2]: J.-P. M. Péraud and N. G. Hadjiconstantinou, “Efficient simulation of multidimensional phonon transport using energy-based variance-reduced Monte Carlo formulations,” Physical Review B, vol. 84, no. 20, p. 205331, 2011.

[3]: J.-P. M. Péraud and N. G. Hadjiconstantinou, “An alternative approach to efficient simulation of micro/nanoscale phonon transport,” Applied Physics Letters, vol. 101, no. 15, p. 153114, 2012.

[4]: A. J. Minnich, G. Chen, S. Mansoor, and B. Yilbas, “Quasiballistic heat transfer
studied using the frequency-dependent Boltzmann transport equation,” Physical
Review B, vol. 84, no. 23, p. 235207, 2011.

[5]: Y.-C. Hua and B.-Y. Cao, “Study of phononic thermal transport across nanostructured
interfaces using phonon Monte Carlo method,” International Journal
of Heat and Mass Transfer, vol. 154, p. 119762, 2020.
