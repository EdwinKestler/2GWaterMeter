# Enclosure CAD

- Source: `Proyecto EmpaguaV3.skp` (SketchUp **14.0.4900**, ~11 MB OLE compound file, not ZIP).
- Renders of this model: `../renders/`.
- Built prototype photos: `../photos/`.

## CAD vs the box that was built

The SketchUp model (lid-mounted PCB, 90° pipe, stacked valve/meter) does **not** match the July 2016 prototype (straight through-pipe, Microduino in a cup on the box floor, empty proto island). Prefer the **lid-mount** layout for production: electronics off the wet plane, gasketed lid, sealed antenna gland.

Until the `.skp` is edited to the built (or the built is rebuilt to the model), treat photos as the as-built record and the SKP as a concept model.

## STL / STEP export

This tree has no SketchUp, Blender, or FreeCAD importer that can read SketchUp 2014 OLE files, so **STL/STEP were not generated in this pass**.

To export from a machine with SketchUp 2014 or newer:

1. Open `Proyecto EmpaguaV3.skp`.
2. Solid-check each component (box, lid, cups, nipples).
3. File → Export → 3D Model → **STL** (mesh for 3D print of cups/lid inserts) and **STEP** if the exporter plugin is installed (machined adapters).
4. Drop files in `exports/` (filenames like `enclosure-box.stl`, `module-cup.stl`). `exports/*.stl` and `exports/*.step` are gitignored until you choose to commit them.

There is no substitute for opening the SKP; do not scale a screenshot.
