set shell := ["zsh", "-cu"]

project_root := justfile_directory()
build_dir := project_root + "/cmake-build-release-emscripten"
target := "bjtu_wgpu_renderer"

# Default: one command to configure, build, serve and open in browser.
default: dev-web

web-configure:
    emcmake cmake -S "{{project_root}}" -B "{{build_dir}}" -DCMAKE_BUILD_TYPE=Release

web-build: web-configure
    cmake --build "{{build_dir}}" --target {{target}} -j 8

web-serve port="8000":
    cd "{{build_dir}}"
    python3 -m http.server {{port}}

# Build, start local server, and open the page. Press Ctrl+C to stop.
dev-web port="8000": web-build
    #!/usr/bin/env zsh
    set -eu
    cd "{{build_dir}}"
    python3 -m http.server {{port}} &
    server_pid=$!
    trap 'kill ${server_pid} >/dev/null 2>&1 || true' EXIT INT TERM
    sleep 1
    open "http://localhost:{{port}}/{{target}}.html"
    wait ${server_pid}
