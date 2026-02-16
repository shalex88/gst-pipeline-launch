---
applyTo: "**/*.cpp, **/*.h, **/CMakeLists.txt, **/*.cmake"
description: C++ guidelines
---

### I. Language & Platform Discipline
- Language: C++20, modern idioms only.
- Compiler: GCC (Linux-only builds and runtime).
- Code must be self-explanatory; avoid comments that explain what good code can express.

### II. Coding Style & Conventions
- Stroustrup style.
- Naming: PascalCase for classes/types; camelCase for functions/methods; snake_case for variables; private members use trailing underscore (_).
- Headers use `#pragma once`.
- No static functions in headers; use anonymous namespaces in .cpp for internal linkage.
- Members initialized with default member initializers; all variables initialized.
- Static functions in anonymous namespaces in cpp files.
- Enforce these via clang-format and clang-tidy; CI checks required.

### III. Build, Dependencies, Packaging
- Single source of truth: CMake for build, test, packaging, and installation.
- Dependency management: vcpkg in manifest mode.
- vcpkg is brought in via CMake FetchContent; no global installs required.
- Reproducibility: lock compatible versions via vcpkg.json manifest; CI uses the same flow as developers.
- Provide install and package targets suitable for Linux distributions.

### IV. Observability & Logging
- Use spdlog for structured logging.
- Log levels: trace, debug, info, warn, err, critical; never log secrets.

### V. Simplicity & Safety
- Prefer simplicity over cleverness; prioritize readability and maintainability.
- Follow the Rule of Zero/Five as applicable.
- Fail fast on invariant violations; validate inputs at module boundaries.
- No raw owning pointers; prefer smart pointers (unique_ptr/shared_ptr).
- Prefer value semantics; reserve shared ownership only when necessary.
- Error handling: prefer Result<T, E> over exceptions for recoverable errors.
- Use const, constexpr, and noexcept where applicable.
- Methods that don’t mutate state must be const.
- Always initialize variables; use default member initializers for class members.

### VI. Performance & Concurrency
- Prefer non-blocking and RAII-managed resources; timeouts for I/O.
- Avoid premature optimization; measure with benchmarks when performance-sensitive.
- Thread-safe designs where concurrency is needed; document ownership and lifetimes in APIs (via types, not comments).
- Validate inputs at module boundaries.
- Never log secrets or PII; redact at source.
- Use safe defaults: fail-fast on invariant violations, explicit error returns for recoverable conditions.

### VII. Test-First (NON-NEGOTIABLE)
- Framework: GoogleTest via vcpkg.
- TDD cycle enforced: write tests → see them fail → implement → refactor with tests green.
- Unit tests for libraries; integration tests for composed behavior and contracts; system tests for end-to-end scenarios.
- Code coverage: minimum 80% for all new code; enforced via CI.
- All new code lands with tests; regressions require a test.