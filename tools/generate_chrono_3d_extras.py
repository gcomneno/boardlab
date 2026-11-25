#!/usr/bin/env python3
"""Original low-poly ChronoOops glTF assets; Python standard library only."""
import base64
import json
import math
import struct
import sys
from pathlib import Path

destination = Path(sys.argv[1])
kind = sys.argv[2]

PALETTE = {
    "archive": {
        "wall": (0.76, 0.70, 0.59, 1),
        "roof": (0.32, 0.32, 0.33, 1),
        "trim": (0.91, 0.83, 0.69, 1),
        "wood": (0.32, 0.23, 0.17, 1),
        "glass": (0.19, 0.35, 0.42, 1),
        "foundation": (0.55, 0.52, 0.48, 1),
    },
    "tower": {
        "wall": (0.69, 0.68, 0.63, 1),
        "roof": (0.28, 0.32, 0.37, 1),
        "trim": (0.82, 0.79, 0.70, 1),
        "wood": (0.29, 0.19, 0.15, 1),
        "glass": (0.12, 0.20, 0.26, 1),
        "foundation": (0.42, 0.44, 0.45, 1),
    },
}

if kind not in PALETTE:
    raise SystemExit("Unknown asset kind")

colors = PALETTE[kind]
groups = {}

def triangle(material, a, b, c):
    u = [b[i] - a[i] for i in range(3)]
    v = [c[i] - a[i] for i in range(3)]
    normal = (
        u[1] * v[2] - u[2] * v[1],
        u[2] * v[0] - u[0] * v[2],
        u[0] * v[1] - u[1] * v[0],
    )
    length = math.sqrt(sum(n*n for n in normal)) or 1.0
    normal = [n / length for n in normal]
    positions, normals = groups.setdefault(material, ([], []))
    for vertex in (a, b, c):
        positions.extend(vertex)
        normals.extend(normal)

def quad(material, a, b, c, d):
    triangle(material, a, b, c)
    triangle(material, a, c, d)

def box(material, x, y, z, width, height, depth):
    a, b = x - width/2, x + width/2
    c, d = y - height/2, y + height/2
    e, f = z - depth/2, z + depth/2
    quad(material, (a,c,f), (b,c,f), (b,d,f), (a,d,f))
    quad(material, (b,c,e), (a,c,e), (a,d,e), (b,d,e))
    quad(material, (a,c,e), (a,c,f), (a,d,f), (a,d,e))
    quad(material, (b,c,f), (b,c,e), (b,d,e), (b,d,f))
    quad(material, (a,d,f), (b,d,f), (b,d,e), (a,d,e))
    quad(material, (a,c,e), (b,c,e), (b,c,f), (a,c,f))

def framed_window(x, y, z, width=0.55, height=0.72):
    box("trim", x, y, z, width+0.16, height+0.17, 0.12)
    box("glass", x, y, z+0.09, width, height, 0.055)
    box("trim", x, y, z+0.13, 0.055, height, 0.045)
    box("trim", x, y, z+0.13, width, 0.055, 0.045)
    box("foundation", x, y-height/2-0.10, z+0.11,
        width+0.28, 0.13, 0.28)

if kind == "archive":
    box("foundation", 0, 0.14, 0, 3.38, 0.28, 2.90)
    box("wall", 0, 1.24, 0, 3.05, 2.48, 2.57)

    # Gabled slate roof, visibly distinct from the municipality.
    left, right = -1.78, 1.78
    front, back = 1.52, -1.52
    eave, peak = 2.52, 3.38
    quad("roof", (left,eave,front), (0,peak,front),
         (0,peak,back), (left,eave,back))
    quad("roof", (0,peak,front), (right,eave,front),
         (right,eave,back), (0,peak,back))
    quad("roof", (left,eave-0.13,front), (right,eave-0.13,front),
         (right,eave,front), (left,eave,front))

    box("trim", 0, 1.02, 1.37, 1.06, 2.00, 0.14)
    box("wood", 0, 0.97, 1.46, 0.84, 1.88, 0.075)
    box("trim", 0, 2.23, 1.48, 1.65, 0.26, 0.16)
    for x in (-1.03, 1.03):
        framed_window(x, 1.42, 1.36, 0.54, 0.79)
    box("foundation", 0, 0.09, 1.72, 1.30, 0.18, 0.66)

else:
    # Tower: layered base, narrowing shaft, cornice and battlements.
    box("foundation", 0, 0.30, 0, 3.16, 0.60, 3.16)
    box("wall", 0, 2.72, 0, 2.48, 4.85, 2.48)
    box("trim", 0, 5.20, 0, 2.94, 0.38, 2.94)
    box("roof", 0, 5.48, 0, 2.55, 0.20, 2.55)

    for x in (-1.13, 0, 1.13):
        for z in (-1.15, 1.15):
            box("trim", x, 5.78, z, 0.49, 0.63, 0.46)
    for z in (-0.01,):
        for x in (-1.17, 1.17):
            box("trim", x, 5.78, z, 0.44, 0.63, 0.48)

    box("trim", 0, 1.03, 1.30, 1.15, 2.05, 0.15)
    box("wood", 0, 0.97, 1.40, 0.90, 1.90, 0.07)
    for y in (2.76, 4.12):
        framed_window(0, y, 1.30, 0.38, 0.79)

    for y in (1.15, 3.05, 4.85):
        box("foundation", -1.26, y, 0.25, 0.085, 0.10, 1.8)

buffer = bytearray()
views = []
accessors = []
primitives = []

def accessor(values, with_bounds):
    offset = len(buffer)
    buffer.extend(struct.pack("<" + "f"*len(values), *values))
    view = len(views)
    views.append({
        "buffer": 0, "byteOffset": offset,
        "byteLength": len(values)*4, "target": 34962
    })
    obj = {
        "bufferView": view,
        "componentType": 5126,
        "count": len(values)//3,
        "type": "VEC3",
    }
    if with_bounds:
        coordinates = [values[i::3] for i in range(3)]
        obj["min"] = [min(axis) for axis in coordinates]
        obj["max"] = [max(axis) for axis in coordinates]
    accessors.append(obj)
    return len(accessors)-1

for material, (positions, normals) in groups.items():
    primitives.append({
        "attributes": {
            "POSITION": accessor(positions, True),
            "NORMAL": accessor(normals, False),
        },
        "material": list(colors).index(material),
        "mode": 4,
    })

document = {
    "asset": {"version": "2.0", "generator": "ChronoOops P1.1"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [{"mesh": 0, "name": kind}],
    "meshes": [{"name": kind, "primitives": primitives}],
    "materials": [{
        "name": name,
        "doubleSided": True,
        "pbrMetallicRoughness": {
            "baseColorFactor": list(color),
            "metallicFactor": 0,
            "roughnessFactor": 0.95,
        },
    } for name, color in colors.items()],
    "buffers": [{
        "byteLength": len(buffer),
        "uri": "data:application/octet-stream;base64,"
               + base64.b64encode(buffer).decode("ascii"),
    }],
    "bufferViews": views,
    "accessors": accessors,
}

destination.write_text(json.dumps(document, separators=(",", ":")) + "\n")
print(f"ASSET={destination.name}")
print(f"TRIANGLES={sum(len(p[0])//9 for p in groups.values())}")
print(f"MATERIALS={len(colors)}")
