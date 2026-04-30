# Search v1 Design

## Purpose

Search v1 adds literal single-buffer text search to `tide`. It extends the command-centric editor flow by letting users search from the command prompt and jump between matches without adding a separate UI panel.

## Scope

Search is command-driven:

- `find <text>` searches for literal text starting at the current cursor position and wraps to the top if needed.
- `next` moves to the next match for the current query and wraps to the top.
- `prev` moves to the previous match for the current query and wraps to the bottom.

When a match is found, the editor moves the cursor to the match, updates the viewport through existing cursor visibility logic, stores the active query, and highlights the current match in the editor surface. When no match is found, the cursor stays in place, the active match is cleared, and the status line shows `no match: <query>`.

## Architecture

Search state belongs to `TideEditor` alongside cursor and viewport state. The editor owns the query text, current match position, and match length. Search functions work on the existing `TideBuffer` line array and expose small APIs for first, next, and previous match movement.

The app layer remains responsible for command parsing. It maps `find <text>`, `next`, and `prev` command prompt entries to editor search APIs. The renderer checks the editor search state and applies reverse style to cells that overlap the current visible match.

## Matching Rules

Search v1 is deliberately simple:

- Matching is literal and case-sensitive.
- Empty queries are invalid and show `search query required`.
- `next` and `prev` without an active query show `no active search`.
- Tabs are still rendered as spaces, but matching operates on the underlying buffer bytes.

## Testing

Tests cover first-match search, wraparound next/previous movement, no-match status, app command dispatch, and reverse-style rendering of the current match. Existing command prompt, editing, save, and render tests must continue to pass.

## Out Of Scope

Search v1 does not implement regex, case-insensitive search, search history, incremental search while typing, all-match highlighting, replacement, project-wide search, fuzzy file search, or search result panels.
