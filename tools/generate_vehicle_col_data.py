#!/usr/bin/env python3

import argparse
import csv
import sys
from pathlib import Path

from rwfury import Dff


MIN_VEHICLE_MODEL = 400
MAX_VEHICLE_MODEL = 611
EXPECTED_VEHICLE_COUNT = (
    MAX_VEHICLE_MODEL - MIN_VEHICLE_MODEL + 1
)

FORMAT_VERSION = 1


# Does not contain col data
KNOWN_UNSUPPORTED_MODELS = {
    594: "rccam",
}


def load_vehicle_models(ide_path: Path):
    """
    Load GTA:SA vehicle model IDs and DFF names from vehicles.ide.

    Returns:
        List of (model_id, model_name) tuples.
    """

    vehicles = []

    with ide_path.open("r", encoding="latin-1") as f:
        for line in f:
            line = line.strip()

            if not line or line.startswith("#"):
                continue

            parts = [part.strip() for part in line.split(",")]

            if len(parts) < 2:
                continue

            try:
                model_id = int(parts[0])
            except ValueError:
                continue

            if not MIN_VEHICLE_MODEL <= model_id <= MAX_VEHICLE_MODEL:
                continue

            # Fix Rockstar's many typos
            model_name = parts[1].split()[0]

            vehicles.append((model_id, model_name))

    vehicles.sort(key=lambda vehicle: vehicle[0])

    return vehicles


def read_collision_bounds(dff_path: Path):
    """
    Read the embedded collision model from a vehicle DFF.

    Returns a dictionary containing:

        AABB minimum:
            min_x, min_y, min_z

        AABB maximum:
            max_x, max_y, max_z

        Bounding sphere:
            sphere_x, sphere_y, sphere_z
            sphere_radius

    Returns None when the DFF has no usable embedded collision model.
    """

    dff = Dff.from_file(dff_path)

    if not dff.collision:
        return None

    col = dff.collision.parse()

    if col is None:
        return None

    # Collision AABB.
    min_x, min_y, min_z = col.bounds.min
    max_x, max_y, max_z = col.bounds.max

    # COL bounding sphere.
    sphere_x, sphere_y, sphere_z = col.bounds.center
    sphere_radius = col.bounds.radius

    return {
        "min_x": min_x,
        "min_y": min_y,
        "min_z": min_z,
        "max_x": max_x,
        "max_y": max_y,
        "max_z": max_z,
        "sphere_x": sphere_x,
        "sphere_y": sphere_y,
        "sphere_z": sphere_z,
        "sphere_radius": sphere_radius,
    }


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Generate GTA San Andreas vehicle collision model "
            "data for omp-vehicle-col-data."
        )
    )

    parser.add_argument(
        "vehicles_ide",
        type=Path,
        help="Path to GTA San Andreas vehicles.ide",
    )

    parser.add_argument(
        "dff_directory",
        type=Path,
        help="Directory containing extracted vehicle DFF files",
    )

    parser.add_argument(
        "output",
        type=Path,
        help="Output CSV file",
    )

    args = parser.parse_args()

    # Validate input paths
    if not args.vehicles_ide.is_file():
        print(
            f"ERROR: vehicles.ide does not exist or is not a file: "
            f"{args.vehicles_ide}",
            file=sys.stderr,
        )
        return 1

    if not args.dff_directory.is_dir():
        print(
            f"ERROR: DFF directory does not exist or is not a directory: "
            f"{args.dff_directory}",
            file=sys.stderr,
        )
        return 1

    # Read vehicle definitions
    vehicles = load_vehicle_models(args.vehicles_ide)

    if len(vehicles) != EXPECTED_VEHICLE_COUNT:
        print(
            f"ERROR: Expected {EXPECTED_VEHICLE_COUNT} vehicles, "
            f"found {len(vehicles)}.",
            file=sys.stderr,
        )
        return 1

    # CSV format
    fieldnames = [
        "model_id",
        "has_col",
        "min_x",
        "min_y",
        "min_z",
        "max_x",
        "max_y",
        "max_z",
        "sphere_x",
        "sphere_y",
        "sphere_z",
        "sphere_radius",
    ]

    failures = []
    skipped = []
    written = 0

    args.output.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    with args.output.open(
        "w",
        newline="",
        encoding="utf-8",
    ) as output_file:

        # Marker
        output_file.write(
            f"# omp-vehicle-col-data,{FORMAT_VERSION}\n"
        )

        writer = csv.DictWriter(
            output_file,
            fieldnames=fieldnames,
        )

        writer.writeheader()

        # Loop through all vehicle models
        for model_id, model_name in vehicles:
            dff_path = (
                args.dff_directory /
                f"{model_name}.dff"
            )

            if not dff_path.exists():
                print(
                    f"[FAIL] {model_id}: {model_name} "
                    f"(DFF not found)",
                    file=sys.stderr,
                )

                failures.append(
                    (
                        model_id,
                        model_name,
                        "missing DFF",
                    )
                )

                continue

            try:
                bounds = read_collision_bounds(dff_path)

            except Exception as error:
                print(
                    f"[FAIL] {model_id}: {model_name} "
                    f"({error})",
                    file=sys.stderr,
                )

                failures.append(
                    (
                        model_id,
                        model_name,
                        str(error),
                    )
                )

                continue

            if bounds is None:
                if model_id in KNOWN_UNSUPPORTED_MODELS:
                    expected_name = KNOWN_UNSUPPORTED_MODELS[model_id]

                    # Check if model name matches using the name from model id
                    if model_name.lower() != expected_name.lower():
                        print(
                            f"[FAIL] {model_id}: {model_name} "
                            f"(expected known unsupported model "
                            f"{expected_name})",
                            file=sys.stderr,
                        )

                        failures.append(
                            (
                                model_id,
                                model_name,
                                (
                                    "known unsupported model name "
                                    f"mismatch; expected {expected_name}"
                                ),
                            )
                        )

                        continue

                    print(
                        f"[SKIP] {model_id}: {model_name} "
                        f"(known unsupported COL)",
                        file=sys.stderr,
                    )

                    # Fill known bad models with zeros
                    writer.writerow({
                        "model_id": model_id,
                        "has_col": 0,
                        "min_x": "0.000000000",
                        "min_y": "0.000000000",
                        "min_z": "0.000000000",
                        "max_x": "0.000000000",
                        "max_y": "0.000000000",
                        "max_z": "0.000000000",
                        "sphere_x": "0.000000000",
                        "sphere_y": "0.000000000",
                        "sphere_z": "0.000000000",
                        "sphere_radius": "0.000000000",
                    })
                    written += 1

                    skipped.append(
                        (
                            model_id,
                            model_name,
                            "known unsupported COL",
                        )
                    )

                    continue

                print(
                    f"[FAIL] {model_id}: {model_name} "
                    f"(COL parse failed)",
                    file=sys.stderr,
                )

                failures.append(
                    (
                        model_id,
                        model_name,
                        "COL parse failed",
                    )
                )

                continue

            writer.writerow({
                "model_id": model_id,
                "has_col": 1,

                "min_x": (
                    f"{bounds['min_x']:.9f}"
                ),
                "min_y": (
                    f"{bounds['min_y']:.9f}"
                ),
                "min_z": (
                    f"{bounds['min_z']:.9f}"
                ),

                "max_x": (
                    f"{bounds['max_x']:.9f}"
                ),
                "max_y": (
                    f"{bounds['max_y']:.9f}"
                ),
                "max_z": (
                    f"{bounds['max_z']:.9f}"
                ),

                "sphere_x": (
                    f"{bounds['sphere_x']:.9f}"
                ),
                "sphere_y": (
                    f"{bounds['sphere_y']:.9f}"
                ),
                "sphere_z": (
                    f"{bounds['sphere_z']:.9f}"
                ),

                "sphere_radius": (
                    f"{bounds['sphere_radius']:.9f}"
                ),
            })

            written += 1

            print(
                f"[OK] {model_id}: {model_name}",
                file=sys.stderr,
            )

    # Summary.
    print(file=sys.stderr)

    print(
        f"Vehicles: {len(vehicles)}",
        file=sys.stderr,
    )

    print(
        f"Written:  {written}",
        file=sys.stderr,
    )

    print(
        f"Skipped:  {len(skipped)}",
        file=sys.stderr,
    )

    print(
        f"Failed:   {len(failures)}",
        file=sys.stderr,
    )

    if skipped:
        print(file=sys.stderr)

        print(
            "Known unsupported models:",
            file=sys.stderr,
        )

        for model_id, model_name, reason in skipped:
            print(
                f"  {model_id} {model_name}: {reason}",
                file=sys.stderr,
            )

    if failures:
        print(file=sys.stderr)

        print(
            "Unexpected failures:",
            file=sys.stderr,
        )

        for model_id, model_name, reason in failures:
            print(
                f"  {model_id} {model_name}: {reason}",
                file=sys.stderr,
            )

    # Ignore known bad models
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
