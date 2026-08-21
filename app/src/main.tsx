import React from "react";
import ReactDOM from "react-dom/client";
import { App } from "./App";
import { AppBoundary } from "./AppBoundary";

// AppBoundary wraps EVERYTHING. Without it a render error in any UI panel takes
// the whole console to a white screen with no way back — f8b3081 ("✕-to-desktop
// crashed the console") was exactly that. The in-scene ErrorBoundary only covers
// asset subtrees and deliberately falls back to null, which is right there and
// useless here.
ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <AppBoundary>
      <App />
    </AppBoundary>
  </React.StrictMode>
);
