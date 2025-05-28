# SDF Collision Simulation

A real-time particle collision simulation using Signed Distance Fields (SDF) and OpenGL rendering.

## Overview

This project simulates particle collision with 3D mesh objects using SDF for efficient collision detection. Particles bounce around in a bounded box environment and interact with loaded mesh geometry.

## Features

- SDF generation from 3D mesh files (.obj format)
- Real-time particle physics simulation
- OpenGL-based 3D rendering
- BVH (Bounding Volume Hierarchy) optimization for performance
- Interactive collision detection

## Setup

1. Place your .obj mesh file in the `data/` directory as `stanford-bunny.obj`
2. Build and run the simulation

## Usage

```bash
# Build the project
mkdir build && cd build
cmake ..
make

# Run simulation with default resolution (64x64x64)
./sdf_simulation

# Run with custom SDF resolution
./sdf_simulation 128
```

## Project Structure

- `src/` - Source code files
  - `sdf.*` - Signed Distance Field generation and sampling
  - `particle.*` - Particle system implementation
  - `simulation.*` - Physics simulation loop
  - `renderer.*` - OpenGL rendering system
  - `mesh.*` - 3D mesh loading and processing
  - `bvh.*` - Bounding Volume Hierarchy for optimization
- `data/` - Place your mesh files here (requires `stanford-bunny.obj`)
- `build/` - Build output directory

## Controls

- ESC: Exit
- Mouse: Rotate camera
