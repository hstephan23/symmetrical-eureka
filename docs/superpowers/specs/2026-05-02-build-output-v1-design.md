# Build Output V1 Design

## Purpose

Start the Build IDE milestone by adding a command that runs a build command as a child process and shows captured output in the workbench.

## Scope

- Add a `tasks` module that runs a shell command as a child process.
- Capture stdout and stderr together.
- Preserve the child exit code.
- Add a `build [command]` command.
- Store the captured output in a workspace buffer named `*build-output*`.
- Use `cmake --build build` when `build` is invoked without arguments.

## Non-Goals

- No async process polling yet.
- No diagnostics parsing yet.
- No jump-to-error yet.
- No configurable project settings yet.

## Architecture

`tasks` owns child process execution and output capture. `app` owns command dispatch and formatting build output into a temporary workspace buffer. `workspace` gets a small text-buffer helper for opening or replacing an in-memory buffer from text.

## Error Handling

Child process launch/read failures return status codes. Nonzero child exits are recoverable and produce a visible output buffer plus a status message.

## Testing

Unit tests cover task output capture and nonzero exit status. App tests cover `build <command>` opening a build output buffer for both success and failure cases.
