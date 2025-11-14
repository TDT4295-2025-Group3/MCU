"""Creates a txt file with cpp code defining vertices and triangles for a 3D model.
This can be used to hardcode simple models into the firmware.

Usage: python3 tools/hardcode_models.py 
"""

import os
from pathlib import Path

# ---------------------------------------------------------
# CONFIG
# ---------------------------------------------------------
SCRIPT_DIR = Path(__file__).resolve().parent
OBJ_FOLDER = SCRIPT_DIR.parent / "build" / "models"
OUTPUT_FILE = SCRIPT_DIR / "player_model.txt"

# ---------------------------------------------------------
# Helper Functions
# ---------------------------------------------------------

def clamp_color(value: float) -> int:
    """Convert float RGB (0-1) to 4-bit integer and clamp to [0, 15]."""

    return max(0, min(15, round(value * 15)))


def parse_vertex(parts):
    """Parse custom OBJ vertex: v x y z r g b."""
    x, y, z = map(float, parts[1:4])
    r, g, b = map(float, parts[4:7])

    # convert float RGB (0-1) -> 0-15 (3x4-bit)
    R = clamp_color(r)
    G = clamp_color(g)
    B = clamp_color(b)
    return (x, y, z, R, G, B)


def parse_face(parts):
    """Parse OBJ face like: f v/t/n v/t/n v/t/n."""
    idx = []
    for token in parts:
        vertex_index = token.split("/")[0]  # only use vertex indices
        idx.append(int(vertex_index) - 1)  # convert to 0-based
    return idx


def triangulate(face):
    """Triangulate an n-gon face using a simple fan."""

    for i in range(1, len(face) - 1):
        yield (face[0], face[i], face[i + 1])


# ---------------------------------------------------------
# MAIN SCRIPT
# ---------------------------------------------------------

def load_obj(filename):
    verts = []
    tris = []

    with open(filename, "r", encoding="utf-8") as f:
        for line in f:
            if line.startswith("v "):
                parts = line.split()
                if len(parts) >= 7:
                    verts.append(parse_vertex(parts))

            elif line.startswith("f "):
                parts = line.split()[1:]
                if len(parts) >= 3:
                    face = parse_face(parts)
                    tris.extend(triangulate(face))

    return verts, tris


def write_output(verts, tris):
    with open(OUTPUT_FILE, "w", encoding="utf-8") as out:
        out.write("Rasterizer::Vertex playerVerts[] = {\n")
        for x, y, z, r, g, b in verts:
            out.write(f"    {{{x}f, {y}f, {z}f, {r}, {g}, {b}}},\n")
        out.write("};\n\n")

        out.write("Rasterizer::Triangle playerTris[] = {\n")
        for a, b, c in tris:
            out.write(f"    {{{a}, {b}, {c}}},\n")
        out.write("};\n")

    print(f"Output written to {OUTPUT_FILE}")


def main():
    # List .obj files
    try:
        files = sorted(
            name for name in os.listdir(OBJ_FOLDER) if name.lower().endswith(".obj")
        )
    except FileNotFoundError:
        print(f"OBJ folder not found: {OBJ_FOLDER}")
        return

    if not files:
        print(f"No .obj files found in {OBJ_FOLDER}")
        return

    print("Choose OBJ file:")
    for idx, fname in enumerate(files):
        print(f"{idx}. {fname}")

    try:
        choice = int(input("Select file number: "))
        obj_path = OBJ_FOLDER / files[choice]
    except (ValueError, IndexError):
        print("Invalid selection.")
        return

    verts, tris = load_obj(obj_path)
    write_output(verts, tris)


if __name__ == "__main__":
    main()
