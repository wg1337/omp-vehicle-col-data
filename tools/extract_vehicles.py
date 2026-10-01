#!/usr/bin/env python3

import sys
from pathlib import Path
from rwfury import Img


def load_vehicle_models(ide_path):
    vehicles = []

    with open(ide_path, "r", encoding="latin-1") as f:
        for line in f:
            line = line.strip()

            if not line or line.startswith("#"):
                continue

            parts = [p.strip() for p in line.split(",")]

            if len(parts) < 2:
                continue

            try:
                model_id = int(parts[0])
            except ValueError:
                continue

            # GTA SA vehicle IDs
            if 400 <= model_id <= 611:
                model_name = parts[1].split()[0]
                vehicles.append((model_id, model_name))

    return vehicles


def main():
    if len(sys.argv) != 4:
        print(
            f"Usage: {sys.argv[0]} "
            "gta3.img vehicles.ide output-directory"
        )
        raise SystemExit(1)

    img_path = Path(sys.argv[1])
    ide_path = Path(sys.argv[2])
    output_dir = Path(sys.argv[3])

    output_dir.mkdir(parents=True, exist_ok=True)

    img = Img.from_file(img_path)

    vehicles = load_vehicle_models(ide_path)

    extracted = 0
    missing = 0

    for model_id, model_name in vehicles:
        filename = f"{model_name}.dff"

        entry = img.find(filename)

        if entry is None:
            print(
                f"[MISSING] {model_id}: {filename}"
            )
            missing += 1
            continue

        data = img.read(filename)

        output_path = output_dir / filename
        output_path.write_bytes(data)

        print(
            f"[OK] {model_id}: {filename} "
            f"({len(data):,} bytes)"
        )

        extracted += 1

    print()
    print(f"Vehicles:  {len(vehicles)}")
    print(f"Extracted: {extracted}")
    print(f"Missing:   {missing}")


if __name__ == "__main__":
    main()
