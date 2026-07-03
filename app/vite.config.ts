/// <reference types="vitest/config" />
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import { VitePWA } from "vite-plugin-pwa";

export default defineConfig({
  // LAN access: serve on all interfaces so Ben (or any laptop/iPad on the same
  // network) can open the twin at http://<this-machine's-ip>:5173 — no deploy needed.
  server: { host: true },
  preview: { host: true },
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
        display: "standalone",
        icons: [],
      },
      workbox: {
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
