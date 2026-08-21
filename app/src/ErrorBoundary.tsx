import { Component, type ReactNode } from "react";

/**
 * Isolates an asset-loading subtree (gobo, glb). If it throws (e.g. a 404 on a
 * missing texture/model), only that piece is dropped — the rest of the tree still
 * renders, so a missing asset can never blank the whole app.
 */
export class ErrorBoundary extends Component<
  { children: ReactNode; fallback?: ReactNode },
  { failed: boolean }
> {
  state = { failed: false };
  static getDerivedStateFromError() {
    return { failed: true };
  }
  componentDidCatch(err: unknown) {
    console.warn("[ErrorBoundary] subtree failed (degraded):", err);
  }
  render() {
    return this.state.failed ? this.props.fallback ?? null : this.props.children;
  }
}
