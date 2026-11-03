#!/usr/bin/env python3
"""Generate a LibOrbisPkg-compatible .gp4 project from a PS4 PKG staging tree.

OpenOrbis's create-gp4 only emits <file> entries and omits matching <dir>
entries, which trips PkgTool.Core's PfsProperties.FindDir.  This helper walks
the staging directory and writes both the file list and the full directory
tree, as the OpenOrbis sample gp4 projects do.

Usage:
    python3 make_gp4.py <staging_dir> <output.gp4> <content_id>
"""
from __future__ import annotations

import argparse
import datetime
import os
from pathlib import Path
from xml.sax.saxutils import quoteattr

# LibOrbisPkg (and avan06's fork) auto-injects these entries.  Listing them
# twice makes pkg_build fail with duplicate paths.
AUTO_INJECTED = {
    "sce_sys/about/right.sprx",
    "sce_sys/icon0.dds",
    "sce_sys/pic1.dds",
    "sce_discmap.plt",
}
SKIP_SUFFIXES = (".pkg", ".gp4")


def collect(staging: Path, output: Path):
    dirs: dict = {}
    files: list[str] = []
    output = output.resolve()
    for current, subdirs, filenames in os.walk(staging):
        subdirs.sort()
        rel_dir = Path(current).relative_to(staging)
        if rel_dir != Path("."):
            node = dirs
            for part in rel_dir.parts:
                node = node.setdefault(part, {})
        for name in sorted(filenames):
            path = Path(current) / name
            rel = path.relative_to(staging).as_posix()
            if path.resolve() == output or rel in AUTO_INJECTED:
                continue
            if rel.endswith(SKIP_SUFFIXES) or name in {".DS_Store"}:
                continue
            if path.is_file():
                files.append(rel)
    return dirs, sorted(files)


def emit_dirs(node: dict, indent: str, lines: list[str]) -> None:
    for name in sorted(node):
        child = node[name]
        attr = quoteattr(name)
        if child:
            lines.append(f"{indent}<dir targ_name={attr}>")
            emit_dirs(child, indent + "\t", lines)
            lines.append(f"{indent}</dir>")
        else:
            lines.append(f"{indent}<dir targ_name={attr} />")


def build_gp4(staging: Path, output: Path, content_id: str) -> str:
    dirs, files = collect(staging, output)
    timestamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
    lines = [
        '<?xml version="1.0"?>',
        '<psproject xmlns:xsd="http://www.w3.org/2001/XMLSchema" '
        'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" fmt="gp4" version="1000">',
        "\t<volume>",
        "\t\t<volume_type>pkg_ps4_app</volume_type>",
        "\t\t<volume_id>PS4VOLUME</volume_id>",
        f"\t\t<volume_ts>{timestamp}</volume_ts>",
        f'\t\t<package content_id={quoteattr(content_id)} '
        'passcode="00000000000000000000000000000000" '
        'storage_type="digital50" app_type="full" />',
        '\t\t<chunk_info chunk_count="1" scenario_count="1">',
        "\t\t\t<chunks>",
        '\t\t\t\t<chunk id="0" layer_no="0" label="Chunk #0" />',
        "\t\t\t</chunks>",
        '\t\t\t<scenarios default_id="0">',
        '\t\t\t\t<scenario id="0" type="sp" initial_chunk_count="1" label="Scenario #0">0</scenario>',
        "\t\t\t</scenarios>",
        "\t\t</chunk_info>",
        "\t</volume>",
        '\t<files img_no="0">',
    ]
    for rel in files:
        attr = quoteattr(rel)
        lines.append(f"\t\t<file targ_path={attr} orig_path={attr} />")
    lines.append("\t</files>")
    lines.append("\t<rootdir>")
    emit_dirs(dirs, "\t\t", lines)
    lines.append("\t</rootdir>")
    lines.append("</psproject>")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("staging", type=Path, help="PKG staging directory")
    parser.add_argument("output", type=Path, help="path of the .gp4 to write")
    parser.add_argument("content_id", help="PS4 CONTENT_ID, e.g. EP0000-LBTY00001_00-...")
    args = parser.parse_args()
    staging = args.staging.resolve()
    output = args.output.resolve()
    if not staging.is_dir():
        parser.error(f"staging directory does not exist: {staging}")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(build_gp4(staging, output, args.content_id), encoding="utf-8")
    print(f"wrote {output} for CONTENT_ID {args.content_id}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
