# Undo/Redo v1 Design

## Purpose

Undo/Redo v1 gives `tide` the minimum recovery loop expected from an editor. Users can reverse recent text edits from the command prompt and redo them if they have not made a new edit.

## Scope

Undo/Redo v1 tracks primitive single-buffer edit operations:

- insert character
- delete character with Backspace
- insert newline
- delete newline / join lines with Backspace at column 0

The command prompt gains `undo` and `redo`. Normal edits push inverse operations onto the undo stack and clear the redo stack. `undo` applies the last inverse operation and pushes its inverse onto the redo stack. `redo` reapplies the last redo operation and pushes its inverse back onto undo. Editing after undo clears redo.

## Architecture

History state lives in `TideEditor` because the editor already owns cursor movement, edit operations, and search state. The history uses fixed-size arrays of small action records, avoiding heap allocation and grouped transaction complexity. Applying a history action uses internal editor helpers so undo/redo does not recursively record itself.

## Error Handling

History capacity is fixed. When a stack is full, the oldest entry is dropped and the newest entry is kept. Empty `undo` sets status to `nothing to undo`; empty `redo` sets status to `nothing to redo`. Successful edits, undo, and redo clear active search state so highlights do not point at stale positions.

## Testing

Tests cover undo/redo for character insertion, Backspace deletion, newline insertion/join, redo clearing after a new edit, command dispatch for `undo` and `redo`, and search state clearing after edits.

## Out Of Scope

Undo/Redo v1 does not implement grouped transactions, multi-character coalescing, unlimited history, persistent history, file reload history, undo-aware dirty clean points, or keyboard shortcuts.
