# Resonance Camera Viewer

Small dependency-free Windows viewer for USB UVC cameras. It runs entirely on
localhost and opens as an Edge app window; video never leaves the computer.

## Start

From PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File ops/camera/Launch-CameraViewer.ps1
```

On first use, allow camera access. The viewer prefers a device whose Windows
label contains `Arducam` or `USB Camera`, then falls back to the first camera.

Controls:

- `Space`: save a PNG snapshot.
- `R`: start or stop WebM recording.
- `M`: mirror the preview.
- `F`: fullscreen the preview.
- Double-click the preview: fullscreen.

Snapshots and recordings use the browser's normal Downloads folder. Resolution
requests are best-effort; the status strip reports the mode the camera actually
accepted. Exposure, focus, zoom, brightness, and similar controls appear only
when the Windows camera driver exposes them to Chromium.

## Stop the local server

Closing the window stops camera capture but leaves the small localhost server
available for the next launch. To stop it explicitly:

```powershell
powershell -ExecutionPolicy Bypass -File ops/camera/Stop-CameraViewer.ps1
```

## IR illumination

Arducam day/night USB modules such as the B0205/B0506 are designed to leave the
IR illuminators connected. Their photoresistor automatically switches both the
850 nm LEDs and the mechanical IR-cut filter between day and night modes.

Disconnect the illuminators only when deliberately testing visible-only images
in a dim room, troubleshooting IR reflection from glass or an enclosure, or
reducing camera power. Unplug USB power before disconnecting either small LED
board. The browser application itself cannot override Arducam's hardware
day/night circuit unless the Windows driver happens to expose a standard
`torch` control.

An IR-sensitive camera without a mechanical IR-cut filter can still show odd
color in daylight after the IR LEDs are disconnected; removing the local IR
source does not add an IR-cut filter.
