# Docs

Design artifacts and decision records. Read alongside `BACKGROUND.md` at repo root.

```
docs/
|-- block-diagram/    System block diagram + power budget.
|-- decisions/        ADR-style decision records -- short, dated, one decision per file.
`-- tests/            Test plans and measured results from bench / field validation.
```

## Decision Record (ADR) format

One file per architectural decision, named `NNNN-short-slug.md`. Keep short. Format:

```
# NNNN -- [decision title]

**Date:** YYYY-MM-DD
**Status:** Proposed / Accepted / Superseded by [link]
**Owners:** Ben, Steve, Elliot, etc.

## Context

What's the situation that's forcing a decision?

## Options considered

- A: ...
- B: ...

## Decision

What we picked, and why.

## Consequences

Downstream implications. What this enables, what this forecloses.
```

ADRs are append-only. If a decision changes, write a new ADR that supersedes the old one and link both ways.
