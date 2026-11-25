#!/usr/bin/env python3
"""Generate original ChronoOops 3D assets using Python's standard library."""

import base64
import json
import math
import struct
import sys
from pathlib import Path

out = Path(sys.argv[1])
groups = {}

COLORS = {
    "plaster": (0.90, 0.73, 0.53, 1),
    "roof": (0.57, 0.24, 0.20, 1),
    "roof_trim": (0.39, 0.19, 0.17, 1),
    "stone": (0.73, 0.68, 0.58, 1),
    "wood": (0.30, 0.18, 0.13, 1),
    "glass": (0.17, 0.35, 0.43, 1),
    "frame": (0.94, 0.85, 0.68, 1),
    "clock": (0.98, 0.87, 0.57, 1),
    "clock_hand": (0.18, 0.19, 0.19, 1),
}

def tri(material, a, b, c):
    u = [b[i] - a[i] for i in range(3)]
    v = [c[i] - a[i] for i in range(3)]
    n = (
        u[1]*v[2] - u[2]*v[1],
        u[2]*v[0] - u[0]*v[2],
        u[0]*v[1] - u[1]*v[0],
    )
    length = math.sqrt(sum(x*x for x in n)) or 1
    normal = tuple(x / length for x in n)
    data = groups.setdefault(material, ([], []))
    for p in (a, b, c):
        data[0].extend(p)
        data[1].extend(normal)

def quad(material, a, b, c, d):
    tri(material, a, b, c)
    tri(material, a, c, d)

def box(material, x, y, z, w, h, d):
    x0, x1 = x-w/2, x+w/2
    y0, y1 = y-h/2, y+h/2
    z0, z1 = z-d/2, z+d/2
    quad(material, (x0,y0,z1), (x1,y0,z1), (x1,y1,z1), (x0,y1,z1))
    quad(material, (x1,y0,z0), (x0,y0,z0), (x0,y1,z0), (x1,y1,z0))
    quad(material, (x0,y0,z0), (x0,y0,z1), (x0,y1,z1), (x0,y1,z0))
    quad(material, (x1,y0,z1), (x1,y0,z0), (x1,y1,z0), (x1,y1,z1))
    quad(material, (x0,y1,z1), (x1,y1,z1), (x1,y1,z0), (x0,y1,z0))
    quad(material, (x0,y0,z0), (x1,y0,z0), (x1,y0,z1), (x0,y0,z1))

# Municipality, with a proper two-slope pitched roof.
box("plaster", 0, 1.55, 0, 3.6, 3.1, 2.6)
box("stone", 0, 0.13, 0, 3.85, 0.26, 2.9)

# Roof eaves, ridge and slopes.
xl, xr = -2.05, 2.05
zf, zb = 1.65, -1.65
eave, ridge = 3.10, 4.22

quad("roof", (xl,eave,zf), (0,ridge,zf),
     (0,ridge,zb), (xl,eave,zb))
quad("roof", (0,ridge,zf), (xr,eave,zf),
     (xr,eave,zb), (0,ridge,zb))
quad("roof_trim", (xl,eave,zf), (xr,eave,zf),
     (xr,eave-0.16,zf), (xl,eave-0.16,zf))
quad("roof_trim", (xl,eave,zb), (xl,eave,zf),
     (xl,eave-0.16,zf), (xl,eave-0.16,zb))
quad("roof_trim", (xr,eave,zf), (xr,eave,zb),
     (xr,eave-0.16,zb), (xr,eave-0.16,zf))

# Entrance: cornice, wooden door, stone step and handle.
box("stone", 0, 1.08, 1.335, 1.22, 2.12, 0.13)
box("wood", 0, 1.00, 1.425, 0.98, 1.94, 0.08)
box("clock", 0.34, 1.05, 1.48, 0.07, 0.07, 0.07)
box("stone", 0, 0.10, 1.70, 1.40, 0.20, 0.68)

# Four framed front windows and raised sills.
for x in (-1.19, 1.19):
    for y in (1.12, 2.35):
        box("frame", x, y, 1.345, 0.77, 0.86, 0.12)
        box("glass", x, y, 1.425, 0.57, 0.65, 0.07)
        box("frame", x, y, 1.475, 0.065, 0.70, 0.045)
        box("frame", x, y, 1.475, 0.60, 0.065, 0.045)
        box("stone", x, y-0.46, 1.44, 0.93, 0.12, 0.27)

# Facade clock, with separate dial and hands.
box("stone", 0, 2.55, 1.355, 0.87, 0.87, 0.10)
box("clock", 0, 2.55, 1.425, 0.71, 0.71, 0.06)
box("clock_hand", 0, 2.66, 1.47, 0.055, 0.25, 0.04)
box("clock_hand", 0.11, 2.55, 1.47, 0.25, 0.055, 0.04)

buffer = bytearray()
views = []
accessors = []
primitives = []
materials = [
    {
        "name": name,
        "pbrMetallicRoughness": {
            "baseColorFactor": list(color),
            "metallicFactor": 0,
            "roughnessFactor": 0.95,
        },
        "doubleSided": True,
    }
    for name, color in COLORS.items()
]

def attribute(values, width, minimum=None, maximum=None):
    offset = len(buffer)
    buffer.extend(struct.pack("<" + "f"*len(values), *values))
    while len(buffer) % 4:
        buffer.append(0)
    view = len(views)
    views.append({
        "buffer": 0,
        "byteOffset": offset,
        "byteLength": len(values)*4,
        "target": 34962,
    })
    acc = {
        "bufferView": view,
        "componentType": 5126,
        "count": len(values)//width,
        "type": "VEC3",
    }
    if minimum is not None:
        acc["min"], acc["max"] = minimum, maximum
    accessors.append(acc)
    return len(accessors)-1

for material, (positions, normals) in groups.items():
    vertices = list(zip(*[iter(positions)]*3))
    low = [min(p[i] for p in vertices) for i in range(3)]
    high = [max(p[i] for p in vertices) for i in range(3)]
    position_id = attribute(positions, 3, low, high)
    normal_id = attribute(normals, 3)
    primitives.append({
        "attributes": {"POSITION": position_id, "NORMAL": normal_id},
        "material": list(COLORS).index(material),
        "mode": 4,
    })

document = {
    "asset": {"version": "2.0", "generator": "ChronoOops standard-library generator"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [{"mesh": 0, "name": "Municipality_1985"}],
    "meshes": [{"name": "Municipality_1985", "primitives": primitives}],
    "materials": materials,
    "buffers": [{
        "byteLength": len(buffer),
        "uri": "data:application/octet-stream;base64," +
               base64.b64encode(buffer).decode("ascii"),
    }],
    "bufferViews": views,
    "accessors": accessors,
}

out.write_text(json.dumps(document, separators=(",", ":")) + "\n")
print(f"ASSET={out}")
print(f"TRIANGLES={sum(len(p[0])//9 for p in groups.values())}")
print(f"MATERIALS={len(materials)}")
