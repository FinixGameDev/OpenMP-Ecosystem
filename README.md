# OpenMP-Ecosystem

> A grid-based predator-prey simulation in C where rabbits and foxes move, breed, eat and starve generation by generation, written to explore parallelising the simulation with **OpenMP**.

**Status: work in progress.** The sequential version runs, and a first parallel attempt ("Paraleize attempt 1") is in place, but the parallel version does not yet produce the same results as the sequential one, and the largest input crashes. See [Known issues](#known-issues).

## Table of contents

- [Overview](#overview)
- [Simulation rules](#simulation-rules)
- [Requirements](#requirements)
- [Build and run](#build-and-run)
- [Input format](#input-format)
- [Output format](#output-format)
- [Example](#example)
- [Included inputs](#included-inputs)
- [How the code works](#how-the-code-works)
- [Known issues](#known-issues)
- [Roadmap](#roadmap)
- [License](#license)

## Overview

The program simulates a small ecosystem on an `R x C` grid. Every cell is either empty, a rock, a rabbit or a fox. In each generation the rabbits move first, then the foxes, and conflicts between animals that land on the same cell are resolved. The program prints the grid before and after the run, writes the final state to a file called `output`, and reports the execution time measured with `omp_get_wtime()`.

The goal of the project is to take a sequential simulation and speed it up with OpenMP, using a configurable number of threads.

## Simulation rules

As implemented in `main.c`:

**Rabbits** move one cell north, south, east or west into a neighbouring cell that is not a rock, a fox or another rabbit. If several moves are possible, one is chosen deterministically from the generation number and the rabbit's position: `(GEN + x + y) mod P`, where `P` is the number of free neighbours. After `GEN_PROC_RABBITS` generations a rabbit reproduces, leaving a new rabbit in the cell it came from.

**Foxes** prefer to move onto an adjacent cell containing a rabbit, which eats the rabbit and resets the fox's hunger counter. If no rabbit is adjacent, the fox moves to a free neighbouring cell, chosen with the same deterministic formula. A fox that goes `GEN_FOOD_FOXES` generations without eating starves. After `GEN_PROC_FOXES` generations a fox reproduces, leaving a new fox in its old cell.

**Conflicts.** If two rabbits (or two foxes) end up on the same cell, one of them is removed, using each animal's age counter and, for foxes, its food counter to decide. Rabbits that end up on a cell occupied by a fox are eaten at the end of the generation.

**Rocks** never move and block both species. Cells outside the grid are treated as blocked.

## Requirements

- A C compiler with OpenMP support (GCC is used by the makefile)
- `make` (optional)

On Debian/Ubuntu: `sudo apt install build-essential`. On macOS the system `clang` does not support `-fopenmp` by default; install GCC with Homebrew and edit the makefile to use it.

## Build and run

```bash
git clone https://github.com/FinixGameDev/OpenMP-Ecosystem.git
cd OpenMP-Ecosystem
make                       # gcc -fopenmp main.c -o ecosystem
./ecosystem <threads> <input-file>
```

For example:

```bash
./ecosystem 4 input10x10   # 4 OpenMP threads on the 10x10 input
```

| Make target | What it does |
| --- | --- |
| `make` | Builds the `ecosystem` executable |
| `make run` | Runs `./ecosystem 4 input100x100` |
| `make clean` | Removes `ecosystem` and `output` |

The first argument is the number of OpenMP threads, the second is the path to the input file. The final state is always written to a file named `output` in the current directory.

## Input format

The first line holds seven integers:

```
GEN_PROC_RABBITS GEN_PROC_FOXES GEN_FOOD_FOXES N_GEN R C N
```

| Field | Meaning |
| --- | --- |
| `GEN_PROC_RABBITS` | Generations before a rabbit reproduces |
| `GEN_PROC_FOXES` | Generations before a fox reproduces |
| `GEN_FOOD_FOXES` | Generations a fox can survive without eating |
| `N_GEN` | Number of generations to simulate |
| `R`, `C` | Grid dimensions |
| `N` | Number of initial objects (rocks, rabbits, foxes) |

Every following line places one object:

```
ROCK x y
RABBIT x y
FOX x y
```

with `0 <= x, y` inside the grid.

## Output format

The `output` file uses the same layout as the input: a header line with the same seven fields (`N_GEN` is the number of generations remaining, so `0` after a complete run, and `N` is the object count at the end), followed by one `ROCK`, `RABBIT` or `FOX` line per object.

## Example

Running the smallest input:

```bash
$ ./ecosystem 1 input5x5
Configuration: 2 4 3 6 5 5 9
-------
|*F RR|
|     |
|R    |
|     |
|FF* F|
-------
Simulating Generation 1...
...
Gen: 6
-------
|*  R |
|     |
|     |
|     |
|  *  |
-------
Execution Time: 0.000047
```

`*` is a rock, `R` a rabbit and `F` a fox. After six generations, only the rocks and a single rabbit are left on this grid.

## Included inputs

| File | Grid | Generations | Initial objects |
| --- | --- | --- | --- |
| `input5x5` | 5 x 5 | 6 | 9 |
| `input10x10` | 10 x 10 | 100 | 50 |
| `input20x20` | 20 x 20 | 1,000 | 100 |
| `input100x100` | 100 x 100 | 10,000 | 700 |
| `input200x200` | 200 x 200 | 10,000 | 1,400 |

For reference, `input100x100` took about 89 seconds with one thread on a single-core test machine. Because that machine had only one core, it can't tell you anything about parallel speed-up.

## How the code works

Everything lives in `main.c`.

**Data structures**

- `Cell { char name; int x, y; }`: one grid cell (`'*'` rock, `'R'`, `'F'` or `'.'` empty)
- `Rabbit` and `Fox`: pointers to their current cell plus age, reproduction and (for foxes) hunger counters
- Two global arrays, `RABBITS` and `FOXES`, each sized `R * C`. A slot with `cell == NULL` is free

**Main loop.** For every generation the program:

1. allocates an empty copy of the grid (`generate_empty_system`)
2. moves all rabbits in parallel (`#pragma omp parallel for`), writing into the copy
3. removes rabbits that collide (`check_rabbit_overlap`)
4. moves all foxes in parallel, again writing into the copy
5. removes foxes that collide (`check_fox_overlap`)
6. removes rabbits that share a cell with a fox (`kill_rabbits`)
7. copies the new grid over the old one

**Functions.** `load_ecosystem` parses the input, `display_ecosystem` prints the grid as ASCII, `output_ecosystem` writes the `output` file, and `move_rabbit`, `move_fox`, `generate_rabbit`, `generate_fox`, `kill_rabbit` and `kill_fox` implement the rules.

**OpenMP usage.** The thread count comes from the first command-line argument via `omp_set_num_threads`. Parallel loops cover rabbit movement, fox movement, both overlap checks and `kill_rabbits`.

## Known issues

These were found by building the project with GCC 13 and running every included input with 1 and 4 threads, and by running it under AddressSanitizer.

- **Crash on large input.** `input200x200` segfaults even with one thread. AddressSanitizer reports a heap-use-after-free in `move_rabbit`: the animals keep pointers to cells in the per-generation grid copy, which is freed at the end of each generation (`free(system_copy)` after the `memcpy`). Small inputs usually survive this by luck.
- **Parallel results differ from sequential ones.** For `input10x10`, `input20x20` and `input100x100`, running with 1 thread and with 4 threads gave different final states. The parallel loops write to shared data without synchronisation (the shared grid copy, the global animal count `N`, and the free-slot searches in `generate_rabbit`/`generate_fox`).
- **Unreliable population counter.** `N` is decremented and incremented from parallel loops without atomics, so the final count is not trustworthy. The `output` file committed in this repository even reports more animals (136,247) than the 100x100 grid has cells.
- **`#pragma omp barrier` has no effect** where it is used, because it sits outside any parallel region (`parallel for` already ends with an implicit barrier).
- **Only square grids are safe.** The code mixes `R` and `C` when indexing (for example `system[y * R + x]` in `move_fox` and `generate_fox` versus `y * C + x` elsewhere). All included inputs are square.
- **Age counter never grows.** `gens` is initialised to 0 and never incremented, so the "older animal wins" tie-break does not distinguish animals.
- **No argument or file checking.** Missing arguments or an unreadable input file crash the program.
- **Build artefacts are committed.** The compiled `ecosystem` binary and a sample `output` file are in the repository, and `.gitignore` does not exclude them.
- **Very verbose output.** Each generation prints several progress lines, which dominates the run time on long simulations. Consider a quiet flag.

## Roadmap

- Fix the use-after-free so large inputs run
- Make the parallel version race-free and verify it against the sequential one on every input
- Add a quiet mode and argument validation
- Benchmark speed-up with different thread counts on a multi-core machine
- Document and enforce the rules (a rules specification, or a link to the assignment this implements)

## License

Released under the [MIT License](LICENSE).
