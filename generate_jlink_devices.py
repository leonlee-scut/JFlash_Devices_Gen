"""Generate a SEGGER J-Link Device Support Kit directory from a CMSIS PDSC."""

from __future__ import annotations

import argparse
import os
import shutil
import sys
import zipfile
from pathlib import Path
from typing import Iterable
import xml.etree.ElementTree as ET


CORE_MAP = {
    "Cortex-M0": "JLINK_CORE_CORTEX_M0",
    "Cortex-M0+": "JLINK_CORE_CORTEX_M0",
    "Cortex-M0Plus": "JLINK_CORE_CORTEX_M0",
    "Cortex-M1": "JLINK_CORE_CORTEX_M1",
    "Cortex-M3": "JLINK_CORE_CORTEX_M3",
    "Cortex-M4": "JLINK_CORE_CORTEX_M4",
    "Cortex-M7": "JLINK_CORE_CORTEX_M7",
    "Cortex-M23": "JLINK_CORE_CORTEX_M23",
    "Cortex-M33": "JLINK_CORE_CORTEX_M33",
    "Cortex-M55": "JLINK_CORE_CORTEX_M55",
}


def children(element: ET.Element, name: str) -> list[ET.Element]:
    return list(element.findall(name))


def hex_value(value: str, attribute: str, context: str) -> int:
    try:
        return int(value.strip(), 0)
    except ValueError as error:
        raise ValueError(f"{context}: invalid {attribute} value {value!r}") from error


def unique_by_name(elements: Iterable[ET.Element]) -> list[ET.Element]:
    result: list[ET.Element] = []
    seen: set[tuple[str, str, str]] = set()
    for element in elements:
        key = (
            element.get("name", ""),
            element.get("start", ""),
            element.get("size", ""),
        )
        if key not in seen:
            seen.add(key)
            result.append(element)
    return result


def safe_pack_path(name: str) -> str:
    normalized = name.replace("\\", "/").lstrip("/")
    if not normalized or any(part == ".." for part in normalized.split("/")):
        raise ValueError(f"unsafe algorithm path in PDSC: {name!r}")
    return normalized


def find_pack(download_dir: Path, vendor: str, name: str, version: str) -> Path:
    expected = f"{vendor}.{name}.{version}.pack".casefold()
    candidates = [path for path in download_dir.iterdir() if path.is_file()]
    for candidate in candidates:
        if candidate.name.casefold() == expected:
            return candidate
    raise FileNotFoundError(
        f"CMSIS pack not found: {download_dir / (vendor + '.' + name + '.' + version + '.pack')}"
    )


def extract_algorithms(pack: Path, algorithm_names: Iterable[str], destination: Path) -> None:
    with zipfile.ZipFile(pack) as archive:
        entries = {entry.filename.replace("\\", "/"): entry for entry in archive.infolist()}
        for algorithm_name in sorted(set(algorithm_names)):
            pack_name = safe_pack_path(algorithm_name)
            entry = entries.get(pack_name)
            if entry is None:
                raise FileNotFoundError(f"algorithm {pack_name!r} not found in {pack}")
            output = destination / Path(pack_name).name
            output.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(entry) as source, output.open("wb") as target:
                shutil.copyfileobj(source, target)


def make_database(vendor: str, family: ET.Element) -> tuple[ET.Element, list[str]]:
    database = ET.Element("Database")
    algorithm_names: list[str] = []

    for subfamily in children(family, "subFamily"):
        processor = subfamily.find("processor")
        core_name = CORE_MAP.get(processor.get("Dcore", "") if processor is not None else "")
        if core_name is None:
            raise ValueError(f"unsupported or missing processor core in {subfamily.get('DsubFamily')!r}")
        inherited_algorithms = children(subfamily, "algorithm")
        for device in children(subfamily, "device"):
            context = device.get("Dname", "device")
            device_algorithms = unique_by_name(inherited_algorithms + children(device, "algorithm"))
            memories = children(device, "memory") + children(subfamily, "memory")
            ram = next(
                (memory for memory in memories if "w" in memory.get("access", "") and
                 ("sram" in memory.get("name", "").casefold() or
                  memory.get("start", "").casefold().startswith("0x2"))),
                None,
            )

            chip_attributes = {
                "Vendor": vendor,
                "Name": context,
                "Core": core_name,
            }
            if ram is not None:
                chip_attributes["WorkRAMAddr"] = f"0x{hex_value(ram.get('start', ''), 'start', context):X}"
                chip_attributes["WorkRAMSize"] = f"0x{hex_value(ram.get('size', ''), 'size', context):X}"

            device_element = ET.SubElement(database, "Device")
            ET.SubElement(device_element, "ChipInfo", chip_attributes)
            ordered_algorithms = sorted(
                device_algorithms,
                key=lambda algorithm: 0 if hex_value(algorithm.get("start", ""), "start", context) < 0x10000000 else 1,
            )
            for algorithm in ordered_algorithms:
                algorithm_name = safe_pack_path(algorithm.get("name", ""))
                if not algorithm_name.lower().endswith(".flm"):
                    raise ValueError(f"{context}: algorithm is not an FLM file: {algorithm_name}")
                base = hex_value(algorithm.get("start", ""), "start", context)
                size = hex_value(algorithm.get("size", ""), "size", context)
                loader_name = Path(algorithm_name).name
                bank_name = "Internal Flash" if base < 0x10000000 else "Configuration"
                bank = ET.SubElement(
                    device_element,
                    "FlashBankInfo",
                    {"Name": bank_name, "BaseAddr": f"0x{base:X}", "AlwaysPresent": "1" if bank_name == "Internal Flash" else "0"},
                )
                ET.SubElement(
                    bank,
                    "LoaderInfo",
                    {
                        "Name": loader_name,
                        "MaxSize": f"0x{size:X}",
                        "Loader": f"Flash/{loader_name}",
                        "LoaderType": "FLASH_ALGO_TYPE_OPEN",
                    },
                )
                algorithm_names.append(algorithm_name)
    return database, algorithm_names


def generate(pdsc_path: Path, output_root: Path, pack_root: Path) -> list[Path]:
    root = ET.parse(pdsc_path).getroot()
    vendor = root.findtext("vendor", "").strip()
    package_name = root.findtext("name", "").strip()
    releases = children(root, "releases/release")
    version = releases[0].get("version", "") if releases else ""
    if not vendor or not package_name or not version:
        raise ValueError("PDSC must contain vendor, name, and at least one release version")

    pack = find_pack(pack_root / ".Download", vendor, package_name, version)
    generated: list[Path] = []
    families = root.findall("devices/family")
    for family in families:
        family_name = family.get("Dfamily", "").strip()
        directory_name = family_name.split(" Series", 1)[0].rstrip()
        directory = output_root / vendor.split(":", 1)[0] / directory_name
        database, algorithms = make_database(vendor.split(":", 1)[0], family)
        extract_algorithms(pack, algorithms, directory / "Flash")
        tree = ET.ElementTree(database)
        ET.indent(tree, space="  ")
        directory.mkdir(parents=True, exist_ok=True)
        xml_path = directory / "Devices.xml"
        tree.write(xml_path, encoding="utf-8", xml_declaration=True)
        generated.append(xml_path)
    return generated


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate J-Link Devices.xml and FLM files from a CMSIS PDSC.")
    parser.add_argument("pdsc", type=Path, help="input CMSIS .pdsc file")
    parser.add_argument("--pack-root", type=Path, default=None, help="CMSIS_PACK_ROOT override")
    parser.add_argument("--output", type=Path, default=None, help="output JLinkDevices directory")
    args = parser.parse_args()
    pack_root = args.pack_root or (Path(os.environ["CMSIS_PACK_ROOT"]) if "CMSIS_PACK_ROOT" in os.environ else None)
    if pack_root is None:
        parser.error("CMSIS_PACK_ROOT is not set; use --pack-root to specify it")
    try:
        output_root = args.output or Path(__file__).resolve().parent / "JLinkDevices"
        generated = generate(args.pdsc.resolve(), output_root.resolve(), pack_root.resolve())
    except (ET.ParseError, OSError, ValueError, zipfile.BadZipFile) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    for path in generated:
        print(path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
