# Project File Finder V1 Design

## Purpose

Make `open` usable as an IDE command by allowing fuzzy project-file queries. After command palette suggestions, the next Workbench step is to discover project files from the current directory and let `open main` resolve to a matching file such as `src/main.c`.

## Scope

- Add a `project_files` module that scans a project root recursively.
- Store project-relative regular file paths.
- Ignore generated/private directories: `.git`, `build`, `build-asan`, `cmake-build-debug`, `.idea`, and `.superpowers`.
- Reuse the command palette fuzzy scorer for file-path matching.
- Let workspace command execution resolve `open <query>` to the best indexed project file when the literal path does not exist.

## Non-Goals

- No live filesystem watching.
- No file finder overlay yet.
- No project root detection beyond the current working directory.
- No path canonicalization or symlink traversal.

## Architecture

`TideProjectFiles` owns scanned file paths and fuzzy match results. The app keeps a lazily built project file index for workspace commands, resolves open arguments through it, then delegates to the existing `TideWorkspace` file opening path.

## Error Handling

Scanning failures are recoverable. If the index cannot be built or no fuzzy file matches, `open` falls back to the literal path and surfaces the existing load/open status. Empty `open` still reports `path required`.

## Testing

Add focused scanner/filter tests with a small fixture directory. Add app command tests showing fuzzy open resolves a known project file and exact literal paths continue to work.
