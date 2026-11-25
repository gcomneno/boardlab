#!/usr/bin/env python3
"""Generate two original low-poly characters as self-contained glTF 2.0."""
import base64
import json
import math
import struct
import sys
from pathlib import Path

output_dir = Path(sys.argv[1])

def generate(kind):
    palettes = {
        "clerk_1985": {
            "skin": (0.91, 0.67, 0.48, 1),
            "hair": (0.22, 0.15, 0.12, 1),
            "coat": (0.17, 0.29, 0.51, 1),
            "shirt": (0.94, 0.91, 0.80, 1),
            "accent": (0.62, 0.19, 0.23, 1),
            "trousers": (0.21, 0.23, 0.30, 1),
            "boots": (0.15, 0.11, 0.10, 1),
            "equipment": (0.43, 0.25, 0.14, 1),
        },
        "explorer_2025": {
            "skin": (0.79, 0.54, 0.39, 1),
            "hair": (0.19, 0.13, 0.12, 1),
            "coat": (0.91, 0.36, 0.17, 1),
            "shirt": (0.22, 0.35, 0.39, 1),
            "accent": (0.94, 0.76, 0.30, 1),
            "trousers": (0.23, 0.32, 0.37, 1),
            "boots": (0.15, 0.19, 0.20, 1),
            "equipment": (0.25, 0.39, 0.28, 1),
        },
    }
    colors = palettes[kind]
    groups = {}

    def triangle(material, a, b, c):
        u = [b[i] - a[i] for i in range(3)]
        v = [c[i] - a[i] for i in range(3)]
        n = (u[1]*v[2]-u[2]*v[1],
             u[2]*v[0]-u[0]*v[2],
             u[0]*v[1]-u[1]*v[0])
        norm = math.sqrt(sum(t*t for t in n)) or 1
        positions, normals = groups.setdefault(material, ([], []))
        for p in (a, b, c):
            positions.extend(p)
            normals.extend(t/norm for t in n)

    def quad(material, a, b, c, d):
        triangle(material, a, b, c)
        triangle(material, a, c, d)

    def box(material, x, y, z, w, h, d):
        x0, x1 = x-w/2, x+w/2
        y0, y1 = y-h/2, y+h/2
        z0, z1 = z-d/2, z+d/2
        quad(material,(x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1))
        quad(material,(x1,y0,z0),(x0,y0,z0),(x0,y1,z0),(x1,y1,z0))
        quad(material,(x0,y0,z0),(x0,y0,z1),(x0,y1,z1),(x0,y1,z0))
        quad(material,(x1,y0,z1),(x1,y0,z0),(x1,y1,z0),(x1,y1,z1))
        quad(material,(x0,y1,z1),(x1,y1,z1),(x1,y1,z0),(x0,y1,z0))
        quad(material,(x0,y0,z0),(x1,y0,z0),(x1,y0,z1),(x0,y0,z1))

    # Feet, separate legs and recognizable jacket silhouette.
    for x in (-0.19, 0.19):
        box("boots", x, 0.12, 0.10, 0.30, 0.24, 0.47)
        box("trousers", x, 0.43, 0, 0.25, 0.54, 0.29)

    box("coat", 0, 1.07, 0, 0.91, 0.83, 0.45)
    box("shirt", 0, 1.27, 0.247, 0.28, 0.34, 0.045)
    box("accent", 0, 1.13, 0.28, 0.10, 0.32, 0.045)

    for x in (-0.57, 0.57):
        box("coat", x, 1.08, 0, 0.25, 0.66, 0.36)
        box("skin", x, 0.68, 0.02, 0.22, 0.18, 0.25)

    box("skin", 0, 1.70, 0, 0.56, 0.57, 0.50)
    box("hair", 0, 2.01, -0.045, 0.60, 0.19, 0.55)
    box("hair", 0, 1.85, -0.26, 0.59, 0.34, 0.09)

    if kind == "clerk_1985":
        # Carried leather case with handle.
        box("equipment", 0.62, 0.47, 0.16, 0.15, 0.45, 0.60)
        box("boots", 0.62, 0.77, 0.16, 0.11, 0.12, 0.27)
    else:
        # Backpack, straps and high-visibility chest detail.
        box("equipment", 0, 1.09, -0.38, 0.72, 0.86, 0.35)
        for x in (-0.31, 0.31):
            box("equipment", x, 1.12, 0.27, 0.14, 0.70, 0.08)
        box("accent", 0, 0.93, 0.285, 0.64, 0.09, 0.055)

    raw = bytearray()
    views, accessors, primitives = [], [], []

    def accessor(values, bounds):
        offset = len(raw)
        raw.extend(struct.pack("<" + "f"*len(values), *values))
        index = len(views)
        views.append({"buffer": 0, "byteOffset": offset,
                      "byteLength": len(values)*4, "target": 34962})
        entry = {"bufferView": index, "componentType": 5126,
                 "count": len(values)//3, "type": "VEC3"}
        if bounds:
            axes = [values[i::3] for i in range(3)]
            entry["min"] = [min(a) for a in axes]
            entry["max"] = [max(a) for a in axes]
        accessors.append(entry)
        return len(accessors)-1

    for material, (positions, normals) in groups.items():
        primitives.append({
            "attributes": {"POSITION": accessor(positions, True),
                           "NORMAL": accessor(normals, False)},
            "material": list(colors).index(material), "mode": 4
        })

    document = {
        "asset": {"version": "2.0", "generator": "ChronoOops P1.1"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": kind}],
        "meshes": [{"name": kind, "primitives": primitives}],
        "materials": [
            {"name": name, "doubleSided": True,
             "pbrMetallicRoughness": {
                 "baseColorFactor": list(color),
                 "metallicFactor": 0, "roughnessFactor": 0.95}}
            for name, color in colors.items()
        ],
        "buffers": [{
            "byteLength": len(raw),
            "uri": "data:application/octet-stream;base64,"
                   + base64.b64encode(raw).decode()
        }],
        "bufferViews": views,
        "accessors": accessors
    }

    target = output_dir / f"{kind}.gltf"
    target.write_text(json.dumps(document, separators=(",", ":")) + "\n")
    assert len(base64.b64decode(
        document["buffers"][0]["uri"].split(",", 1)[1]
    )) == len(raw)
    print(f"CHARACTER={kind}")
    print(f"TRIANGLES={sum(len(p[0])//9 for p in groups.values())}")
    print(f"MATERIALS={len(colors)}")

generate("clerk_1985")
generate("explorer_2025")
