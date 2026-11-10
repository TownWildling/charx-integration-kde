# CHARX Integration for KDE

An independent KDE Frameworks 6 integration that adds thumbnails for
`.charx` Character Card V3 archives to Dolphin and other KIO applications. It displays the main character portrait, preserves aspect ratio and
transparency, and never modifies the input card. It does not implement chat,
lorebook evaluation, or PNG/JSON card thumbnails.

## Fedora build

```sh
sudo dnf install gcc-c++ cmake ninja-build extra-cmake-modules \
  qt6-qtbase-devel kf6-kio-devel kf6-kcoreaddons-devel qt6-qtimageformats python3
cmake -S . -B build-local -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local" \
  -DKDE_INSTALL_PLUGINDIR=lib64/qt6/plugins \
  -DKDE_INSTALL_LIBEXECDIR=libexec \
  -DCHARX_INSTALL_PLASMA_HOOK=ON \
  -DPython3_EXECUTABLE=/usr/bin/python3
cmake --build build-local
ctest --test-dir build-local --output-on-failure
cmake --install build-local
update-mime-database "$HOME/.local/share/mime"
```

The user-local install adds the plugin, its Python helper, the MIME definition,
and a Plasma login hook. Run the commands as your desktop user. CMake's
`install_manifest.txt` lists the installed files. Reconfigure and rebuild if
the installation prefix changes.

Log out of Plasma and back in, then open Dolphin normally. Enable
**Character Card V3 archives (CHARX)** in Dolphin's preview settings and turn
previews on. Dolphin's maximum preview file size must accommodate the archive.

The hook lives under `${XDG_CONFIG_HOME:-$HOME/.config}/plasma-workspace/env`.
It adds the installed Qt plugin directory to the Plasma session's
`QT_PLUGIN_PATH` only when the plugin file is readable. It preserves existing
entries and leaves the path alone if the plugin is removed.
`CHARX_INSTALL_PLASMA_HOOK` defaults to `OFF` for other build configurations.

For distribution-wide discovery, package the project as an RPM using Fedora's
Qt plugin path (`qtpaths6 --plugin-dir`) and a `/usr` prefix. Rebuild against
Fedora's supported Qt 6/KF6 development packages after an incompatible framework
upgrade. No binary plugin can promise compatibility with future major Qt/KF
versions; KF5/Plasma 5 is deliberately unsupported.

## Architecture and policy

* `src/thumbnail.cpp`: small KIO adapter and request validation.
* `src/assetprocess.cpp`: bounded subprocess transport and seven-second deadline.
* `reader/charx_reader.py`: standard-library ZIP access, card parsing, and URI
  resolution; exposed as independently testable functions.
* `src/imagedecoder.cpp`: raster format allowlist, dimension checks, and scaling.
* `data/`: separate MIME registration; does not claim general ZIP files.

The reader recognizes `spec: chara_card_v3`, tolerates newer spec versions and
unknown fields, and selects `type: icon, name: main` before other icons. It never
substitutes a gallery image for a missing main portrait. The spec's intentionally
misspelled `embeded://` is supported with exact case-sensitive archive paths.
Base64 image data URIs are supported. `ccdefault:` (also used when `assets` is
absent) checks `assets/icon/images/main.png`, `.webp`, then `.jpg`; the CHARX spec
does not prescribe a default image path. No icon means a normal thumbnail
failure, allowing the file manager to show its MIME icon.

HTTP(S), local file URIs, SVG, and other formats are deliberately unsupported.
Only PNG, JPEG and WebP are decoded, using installed Qt image plugins. Nothing
is extracted to disk; there is no network client and no asset code execution.
Duplicate ZIP names, symlink assets, encrypted selected entries, unsafe asset
paths, corrupt ZIPs and malformed cards fail cleanly. Limits: 256 MiB archive,
4,096 entries, 4 MiB JSON, 24 MiB encoded image bytes, 32 megapixels, 16,384 pixels
per image dimension, and 4,096 pixels per requested thumbnail dimension. The
reader runs with a 256 MiB address-space limit and a five-second CPU limit.
Qt image decoding stays in the normal KIO thumbnail worker, bounded by image
size checks; this is not a separate image-codec sandbox.

## Verification

The standard-library tests run without KDE development packages:

```sh
/usr/bin/python3 -m unittest discover -s tests -v
```

After installation, exercise the actual plugin without starting Dolphin:

```sh
build-local/charx-plugin-smoke \
  "$HOME/.local/lib64/qt6/plugins/kf6/thumbcreator/libcharxthumbnail.so" \
  "/absolute/path/to/card.charx" /tmp/charx-preview.png
```

The smoke executable is built only when `BUILD_TESTING=ON` (the default).

## Removal

For the user-local layout above, remove only this project's files, then refresh
the MIME database and log out of Plasma and back in:

```sh
rm -- "$HOME/.local/lib64/qt6/plugins/kf6/thumbcreator/libcharxthumbnail.so" \
  "$HOME/.local/libexec/charx-thumbnailer/charx_reader.py" \
  "$HOME/.local/share/mime/packages/application-x-charx.xml" \
  "${XDG_CONFIG_HOME:-$HOME/.config}/plasma-workspace/env/charx-thumbnailer.sh"
update-mime-database "$HOME/.local/share/mime"
```

Do not remove shared Qt/KDE directories. No source cards or global configuration
need to be changed. Source code is available under the MIT license in `LICENSE`.
