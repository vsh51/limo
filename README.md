# LIMO (Linear Method Optimizer)

C++ project for solving linear programming problems with simplex method.

## Build

This project uses CMake (C++20).

## Structure

- `src/` — internal libraries (core + math/LP algorithms)
- `libs/` — third-party or external libraries
- `doc/` — Doxygen config and additional documentation files

## Commit conventions

We're using Conventional Commits for commit messages. Please refer to https://www.conventionalcommits.org/en/v1.0.0/ for details.

Here're some examples of commit messages in case you're intereseted in contributing:
```
feat: add new feature
fix: fix a bug
feat(core): improve performance of the core module
fix(matrix)!: change API of the matrix module
docs: update documentation
build: update build scripts
chore: general maintenance tasks
```