/// <reference types="vitest/config" />
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import { VitePWA } from "vite-plugin-pwa";

export default defineConfig({
  // LAN access: serve on all interfaces so Ben (or any laptop/iPad on the same
  // network) can open the twin at http://<this-machine's-ip>:5173 — no deploy needed.
  //
  // /cambium → the local cambium daemon (:8600), proxied INCLUDING the websocket.
  // Why a proxy and not just ?cambium=ws://<host>:8600/ws — measured 2026-08-15:
  // the daemon binds *:8600 correctly, but macOS's application firewall allows
  // per-BINARY, and cambium runs from its venv python (~/code/cambium/.venv/bin/
  // python3.14), which is not the allow-listed /usr/bin/python3. So :8600 answers
  // on localhost and is refused from the LAN, while vite's node binary is already
  // allowed. Proxying puts the daemon behind the port that already works — one
  // origin, one URL, no sudo, no firewall edit on Elliot's machine.
  // Bonus: same-origin ws also sidesteps the ws://-from-https mixed-content block.
  server: {
    host: true,
    proxy: { "/cambium": { target: "http://127.0.0.1:8600", ws: true, rewrite: (p) => p.replace(/^\/cambium/, "") } },
  },
  preview: {
    host: true,
    proxy: { "/cambium": { target: "http://127.0.0.1:8600", ws: true, rewrite: (p) => p.replace(/^\/cambium/, "") } },
  },
  plugins: [
    react(),
    VitePWA({
      registerType: "autoUpdate",
      manifest: {
        name: "Resonance Tree — Mirror Twin",
        short_name: "Resonance Tree",
        description: "Real-time 3D lighting twin + control system for the Resonance Tree",
        theme_color: "#05070a",
        background_color: "#05070a",
        display: "standalone", // installed = no browser top bar / bottom nav (Elliot 08-14)
        orientation: "portrait",
        icons: [
          { src: "icons/icon-192.png", sizes: "192x192", type: "image/png" },
          { src: "icons/icon-512.png", sizes: "512x512", type: "image/png" },
          { src: "icons/icon-512.png", sizes: "512x512", type: "image/png", purpose: "maskable" },
        ],
      },
      workbox: {
        // updates take control on FIRST reopen — Elliot was chronically one
        // version behind ("close and reopen twice"); never again
        skipWaiting: true,
        clientsClaim: true,
        // precache the app shell; runtime-cache the big art assets on first load
        globPatterns: ["**/*.{js,css,html}"],
        maximumFileSizeToCacheInBytes: 4 * 1024 * 1024,
        runtimeCaching: [
          {
            urlPattern: /\.(?:glb|png|json|wav)$/,
            handler: "CacheFirst",
            options: { cacheName: "tree-assets", expiration: { maxEntries: 30 } },
          },
        ],
      },
    }),
  ],
  test: {
    environment: "jsdom",
    globals: true,
    include: ["src/**/*.test.{ts,tsx}"],
  },
});
