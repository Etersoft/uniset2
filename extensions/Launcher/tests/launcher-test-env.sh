#!/bin/sh

resolve_launcher() {
    _lte_launcher_dir=$1

    # Prefer the libtool wrapper: it sets the build-tree library paths needed
    # by uninstalled shared libraries. The .libs binary is only safe when the
    # caller has already prepared the dynamic loader environment.
    if [ -x "$_lte_launcher_dir/uniset2-launcher" ]; then
        printf '%s\n' "$_lte_launcher_dir/uniset2-launcher"
        return 0
    fi

    if [ -x "$_lte_launcher_dir/.libs/uniset2-launcher" ]; then
        printf '%s\n' "$_lte_launcher_dir/.libs/uniset2-launcher"
        return 0
    fi

    echo "ERROR: launcher binary not found at $_lte_launcher_dir" >&2
    return 1
}
