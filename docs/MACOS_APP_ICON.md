macOS App Icon Setup

This project can build the integrated app as a macOS bundle with a Dock icon.

Quick steps
- Prepare an ICNS file: `assets/AppIcon.icns` (1024×1024 recommended with proper iconset).
- Build the app: `./build.sh`
- The bundle is at `build/AIMusicHardwareIntegrated.app` and will show the icon in the Dock.

Creating the ICNS
1) Start from a 1024×1024 PNG (transparent background preferred).
2) Create an iconset folder and scale variants:
   
   mkdir -p AppIcon.iconset
   sips -z 16 16     icon.png --out AppIcon.iconset/icon_16x16.png
   sips -z 32 32     icon.png --out AppIcon.iconset/icon_16x16@2x.png
   sips -z 32 32     icon.png --out AppIcon.iconset/icon_32x32.png
   sips -z 64 64     icon.png --out AppIcon.iconset/icon_32x32@2x.png
   sips -z 128 128   icon.png --out AppIcon.iconset/icon_128x128.png
   sips -z 256 256   icon.png --out AppIcon.iconset/icon_128x128@2x.png
   sips -z 256 256   icon.png --out AppIcon.iconset/icon_256x256.png
   sips -z 512 512   icon.png --out AppIcon.iconset/icon_256x256@2x.png
   sips -z 512 512   icon.png --out AppIcon.iconset/icon_512x512.png
   cp icon.png AppIcon.iconset/icon_512x512@2x.png
   
3) Convert to ICNS:
   
   iconutil -c icns AppIcon.iconset
   
4) Move the ICNS into the repo:
   
   mv AppIcon.icns assets/AppIcon.icns
   
5) Rebuild with `./build.sh`.

Notes
- The CMake target `AIMusicHardwareIntegrated` is built as a MACOSX_BUNDLE and will include `assets/AppIcon.icns` if present.
- The bundle is built in `build/AIMusicHardwareIntegrated.app`. You can `open` it or drag to Dock.
