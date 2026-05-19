#!/usr/bin/env python3
"""Generate the Rev A starter KiCad PCB.

This is intentionally a starter layout, not an order-ready release.  The stock
KiCad library does not include the exact M5Stack Grove/HY2.0 socket footprint,
so J1 uses a provisional JST-PH 1x04 2.0 mm footprint until the exact connector
part is selected.
"""

from __future__ import annotations

import json
from pathlib import Path

import pcbnew


PROJECT = "neohex-passive-rev-a"
OUT_DIR = Path(__file__).resolve().parent
FP_ROOT = Path("/usr/share/kicad/footprints")


def mm(value: float) -> int:
    return pcbnew.FromMM(value)


def pt(x: float, y: float) -> pcbnew.VECTOR2I:
    return pcbnew.VECTOR2I(mm(x), mm(y))


def load_fp(
    board: pcbnew.BOARD,
    ref: str,
    value: str,
    lib: str,
    name: str,
    x: float,
    y: float,
    rot: float = 0,
) -> pcbnew.FOOTPRINT:
    fp = pcbnew.FootprintLoad(str(FP_ROOT / f"{lib}.pretty"), name)
    if fp is None:
        raise RuntimeError(f"Could not load footprint {lib}:{name}")
    fp.SetReference(ref)
    fp.SetValue(value)
    fp.SetPosition(pt(x, y))
    fp.SetOrientationDegrees(rot)
    fp.Reference().SetVisible(False)
    fp.Value().SetVisible(False)
    board.Add(fp)
    return fp


def net(board: pcbnew.BOARD, name: str) -> pcbnew.NETINFO_ITEM:
    item = pcbnew.NETINFO_ITEM(board, name)
    board.Add(item)
    return item


def assign(fp: pcbnew.FOOTPRINT, nets: dict[str, pcbnew.NETINFO_ITEM], mapping: dict[str, str]) -> None:
    for pad_num, net_name in mapping.items():
        pad = fp.FindPadByNumber(pad_num)
        if pad is None:
            raise RuntimeError(f"{fp.GetReference()} has no pad {pad_num}")
        pad.SetNet(nets[net_name])


def track(
    board: pcbnew.BOARD,
    nets: dict[str, pcbnew.NETINFO_ITEM],
    net_name: str,
    start: tuple[float, float],
    end: tuple[float, float],
    width: float = 0.25,
    layer: int = pcbnew.F_Cu,
) -> None:
    item = pcbnew.PCB_TRACK(board)
    item.SetStart(pt(*start))
    item.SetEnd(pt(*end))
    item.SetLayer(layer)
    item.SetWidth(mm(width))
    item.SetNet(nets[net_name])
    board.Add(item)


def via(
    board: pcbnew.BOARD,
    nets: dict[str, pcbnew.NETINFO_ITEM],
    net_name: str,
    x: float,
    y: float,
    diameter: float = 0.8,
    drill: float = 0.4,
) -> None:
    item = pcbnew.PCB_VIA(board)
    item.SetPosition(pt(x, y))
    item.SetWidth(mm(diameter))
    item.SetDrill(mm(drill))
    item.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
    item.SetViaType(pcbnew.VIATYPE_THROUGH)
    item.SetNet(nets[net_name])
    board.Add(item)


def edge(board: pcbnew.BOARD, start: tuple[float, float], end: tuple[float, float]) -> None:
    item = pcbnew.PCB_SHAPE(board)
    item.SetShape(pcbnew.SHAPE_T_SEGMENT)
    item.SetStart(pt(*start))
    item.SetEnd(pt(*end))
    item.SetLayer(pcbnew.Edge_Cuts)
    item.SetWidth(mm(0.1))
    board.Add(item)


def text(
    board: pcbnew.BOARD,
    value: str,
    x: float,
    y: float,
    size: float = 1.0,
    layer: int = pcbnew.F_SilkS,
    angle_deg: float = 0,
    justify: int = pcbnew.GR_TEXT_H_ALIGN_LEFT,
) -> None:
    item = pcbnew.PCB_TEXT(board)
    item.SetText(value)
    item.SetPosition(pt(x, y))
    item.SetLayer(layer)
    item.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
    item.SetTextThickness(mm(0.15))
    item.SetTextAngleDegrees(angle_deg)
    item.SetHorizJustify(justify)
    board.Add(item)


def write_project_file() -> None:
    project = {
        "board": {
            "3dviewports": [],
            "design_settings": {
                "defaults": {
                    "board_outline_line_width": 0.1,
                    "copper_line_width": 0.25,
                    "copper_text_size_h": 1.5,
                    "copper_text_size_v": 1.5,
                    "copper_text_thickness": 0.3,
                    "other_line_width": 0.15,
                    "silk_line_width": 0.15,
                    "silk_text_size_h": 1.0,
                    "silk_text_size_v": 1.0,
                    "silk_text_thickness": 0.15,
                },
                "diff_pair_dimensions": [],
                "drc_exclusions": [],
                "rules": {
                    "solder_mask_clearance": 0.0,
                    "solder_mask_min_width": 0.0,
                },
                "track_widths": [0.25, 0.5, 1.2],
                "via_dimensions": [
                    {
                        "diameter": 0.8,
                        "drill": 0.4,
                    }
                ],
            },
            "ipc2581": {
                "dist": "",
                "distpn": "",
                "internal_id": "",
                "mfg": "",
                "mpn": "",
            },
            "layer_pairs": [],
            "layer_presets": [],
            "viewports": [],
        },
        "boards": [],
        "cvpcb": {"equivalence_files": []},
        "erc": {
            "erc_exclusions": [],
            "meta": {"version": 0},
            "pin_map": [],
            "rule_severities": {},
        },
        "libraries": {
            "pinned_footprint_libs": [],
            "pinned_symbol_libs": [],
        },
        "meta": {
            "filename": f"{PROJECT}.kicad_pro",
            "version": 3,
        },
        "net_settings": {
            "classes": [
                {
                    "bus_width": 12,
                    "clearance": 0.2,
                    "diff_pair_gap": 0.25,
                    "diff_pair_via_gap": 0.25,
                    "diff_pair_width": 0.2,
                    "line_style": 0,
                    "microvia_diameter": 0.3,
                    "microvia_drill": 0.1,
                    "name": "Default",
                    "pcb_color": "rgba(0, 0, 0, 0.000)",
                    "priority": 2147483647,
                    "schematic_color": "rgba(0, 0, 0, 0.000)",
                    "track_width": 0.25,
                    "via_diameter": 0.8,
                    "via_drill": 0.4,
                    "wire_width": 6,
                },
                {
                    "bus_width": 12,
                    "clearance": 0.2,
                    "diff_pair_gap": 0.25,
                    "diff_pair_via_gap": 0.25,
                    "diff_pair_width": 0.2,
                    "line_style": 0,
                    "microvia_diameter": 0.3,
                    "microvia_drill": 0.1,
                    "name": "LED_POWER",
                    "pcb_color": "rgba(255, 0, 0, 0.500)",
                    "priority": 2147483646,
                    "schematic_color": "rgba(255, 0, 0, 0.500)",
                    "track_width": 1.2,
                    "via_diameter": 0.8,
                    "via_drill": 0.4,
                    "wire_width": 6,
                },
            ],
            "meta": {"version": 4},
            "net_colors": None,
            "netclass_assignments": [["VLED", "LED_POWER"], ["GND", "LED_POWER"]],
            "netclass_patterns": [],
        },
        "pcbnew": {
            "last_paths": {
                "gencad": "",
                "idf": "",
                "netlist": "",
                "plot": "",
                "pos_files": "",
                "specctra_dsn": "",
                "step": "",
                "svg": "",
                "vrml": "",
            },
            "page_layout_descr_file": "",
        },
        "schematic": {
            "annotate_start_num": 0,
            "bom_export_filename": "${PROJECTNAME}.csv",
            "bom_fmt_presets": [],
            "bom_presets": [],
            "bom_settings": {},
            "connection_grid_size": 50.0,
            "legacy_lib_dir": "",
            "legacy_lib_list": [],
            "meta": {"version": 1},
            "net_format_name": "",
            "page_layout_descr_file": "",
            "plot_directory": "",
        },
        "sheets": [],
        "text_variables": {},
    }
    (OUT_DIR / f"{PROJECT}.kicad_pro").write_text(json.dumps(project, indent=2) + "\n")


def build() -> None:
    board = pcbnew.BOARD()

    nets = {
        name: net(board, name)
        for name in [
            "GND",
            "VLED",
            "STEMMA_VPLUS",
            "STEMMA_SDA",
            "STEMMA_SCL",
            "GPIO_DIN",
            "DATA_RAW",
            "DATA_OUT",
            "GPIO_REF",
            "NC_WHITE",
        ]
    }

    # Board outline: roomy first article, can shrink after connector selection.
    width = 60.0
    height = 35.0
    edge(board, (0, 0), (width, 0))
    edge(board, (width, 0), (width, height))
    edge(board, (width, height), (0, height))
    edge(board, (0, height), (0, 0))

    fps = {}
    fps["J1"] = load_fp(
        board,
        "J1",
        "GROVE_OUT_PH4_PLACEHOLDER",
        "Connector_JST",
        "JST_PH_B4B-PH-K_1x04_P2.00mm_Vertical",
        55.0,
        12.0,
        270,
    )
    fps["J2"] = load_fp(
        board,
        "J2",
        "VLED_IN",
        "Connector_JST",
        "JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical",
        5.0,
        8.0,
        90,
    )
    fps["J3"] = load_fp(
        board,
        "J3",
        "STEMMA_DATA",
        "Connector_JST",
        "JST_SH_BM04B-SRSS-TB_1x04-1MP_P1.00mm_Vertical",
        6.0,
        29.0,
        90,
    )
    fps["J4"] = load_fp(
        board,
        "J4",
        "GPIO_DATA",
        "Connector_JST",
        "JST_SH_BM03B-SRSS-TB_1x03-1MP_P1.00mm_Vertical",
        45.0,
        29.0,
        0,
    )
    fps["R1"] = load_fp(board, "R1", "330R", "Resistor_SMD", "R_0805_2012Metric", 49.0, 16.0, 0)
    fps["C1"] = load_fp(board, "C1", "0.1uF", "Capacitor_SMD", "C_0805_2012Metric", 49.0, 13.0, 90)
    fps["C2"] = load_fp(board, "C2", "100-470uF_DNP", "Capacitor_SMD", "CP_Elec_6.3x5.4", 44.0, 8.0, 0)

    for ref, x, y in [
        ("SJ1", 18.0, 28.5),
        ("SJ2", 18.0, 26.0),
        ("SJ3", 45.0, 25.0),
        ("SJ4", 18.0, 21.5),
    ]:
        rotation = 180 if ref == "SJ3" else 0
        fps[ref] = load_fp(
            board,
            ref,
            "OPEN",
            "Jumper",
            "SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm",
            x,
            y,
            rotation,
        )

    for ref, value, x, y in [
        ("TP1", "VLED", 34.0, 11.5),
        ("TP2", "GND", 52.0, 4.0),
        ("TP3", "DATA_RAW", 38.0, 25.0),
        ("TP4", "DATA_OUT", 50.0, 22.0),
        ("TP5", "NC", 55.0, 26.0),
    ]:
        fps[ref] = load_fp(board, ref, value, "TestPoint", "TestPoint_Pad_D1.5mm", x, y, 0)

    load_fp(board, "H1", "M2", "MountingHole", "MountingHole_2.2mm_M2", 8.0, 16.0, 0)
    load_fp(board, "H2", "M2", "MountingHole", "MountingHole_2.2mm_M2", 56.0, 31.0, 0)

    assign(fps["J1"], nets, {"1": "GND", "2": "VLED", "3": "DATA_OUT", "4": "NC_WHITE"})
    assign(fps["J2"], nets, {"1": "VLED", "2": "GND"})
    assign(fps["J3"], nets, {"1": "GND", "2": "STEMMA_VPLUS", "3": "STEMMA_SDA", "4": "STEMMA_SCL"})
    assign(fps["J4"], nets, {"1": "GND", "2": "GPIO_REF", "3": "GPIO_DIN"})
    assign(fps["R1"], nets, {"1": "DATA_RAW", "2": "DATA_OUT"})
    assign(fps["C1"], nets, {"1": "VLED", "2": "GND"})
    assign(fps["C2"], nets, {"1": "VLED", "2": "GND"})
    assign(fps["SJ1"], nets, {"1": "STEMMA_SDA", "2": "DATA_RAW"})
    assign(fps["SJ2"], nets, {"1": "STEMMA_SCL", "2": "DATA_RAW"})
    assign(fps["SJ3"], nets, {"1": "GPIO_DIN", "2": "DATA_RAW"})
    assign(fps["SJ4"], nets, {"1": "STEMMA_VPLUS", "2": "VLED"})
    for ref, net_name in [
        ("TP1", "VLED"),
        ("TP2", "GND"),
        ("TP3", "DATA_RAW"),
        ("TP4", "DATA_OUT"),
        ("TP5", "NC_WHITE"),
    ]:
        assign(fps[ref], nets, {"1": net_name})

    # VLED and GND trunks, sized for the Rev A 1 A learning target.
    for a, b in [
        ((5.0, 8.0), (41.2, 8.0)),
        ((41.2, 8.0), (41.2, 14.0)),
        ((41.2, 14.0), (55.0, 14.0)),
        ((34.0, 8.0), (34.0, 11.5)),
    ]:
        track(board, nets, "VLED", a, b, 1.2)

    for a, b in [
        ((5.0, 6.0), (46.8, 6.0)),
        ((46.8, 6.0), (46.8, 12.0)),
        ((46.8, 12.0), (55.0, 12.0)),
        ((52.0, 12.0), (52.0, 4.0)),
    ]:
        track(board, nets, "GND", a, b, 1.2)

    # Low-current ground references for data connectors route around the board
    # perimeter so they do not cut through the selectable-data jumper field.
    for a, b in [
        ((7.325, 30.5), (3.0, 30.5)),
        ((3.0, 30.5), (3.0, 6.0)),
        ((3.0, 6.0), (5.0, 6.0)),
        ((44.0, 30.325), (44.0, 33.0)),
        ((44.0, 33.0), (3.0, 33.0)),
        ((3.0, 33.0), (3.0, 30.5)),
    ]:
        track(board, nets, "GND", a, b, 0.35)

    # Data-source jumpers and post-resistor output.
    data_tracks = [
        ("STEMMA_SDA", (7.325, 28.5), (17.35, 28.5), 0.2),
        ("STEMMA_SCL", (7.325, 27.5), (13.0, 27.5), 0.2),
        ("STEMMA_SCL", (13.0, 27.5), (13.0, 26.0), 0.2),
        ("STEMMA_SCL", (13.0, 26.0), (17.35, 26.0), 0.2),
        ("GPIO_DIN", (46.0, 30.325), (50.0, 30.325), 0.2),
        ("GPIO_DIN", (50.0, 30.325), (50.0, 25.0), 0.2),
        ("GPIO_DIN", (50.0, 25.0), (45.65, 25.0), 0.2),
        ("DATA_RAW", (18.65, 28.5), (36.0, 28.5), 0.25),
        ("DATA_RAW", (18.65, 26.0), (36.0, 26.0), 0.25),
        ("DATA_RAW", (44.35, 25.0), (36.0, 25.0), 0.25),
        ("DATA_RAW", (36.0, 28.5), (36.0, 16.0), 0.25),
        ("DATA_RAW", (36.0, 16.0), (48.087, 16.0), 0.25),
        ("DATA_OUT", (49.913, 16.0), (55.0, 16.0), 0.25),
        ("DATA_OUT", (50.0, 16.0), (50.0, 22.0), 0.25),
        ("NC_WHITE", (55.0, 18.0), (55.0, 26.0), 0.25),
    ]
    for net_name, a, b, width_mm in data_tracks:
        track(board, nets, net_name, a, b, width_mm)

    # Optional STEMMA V+ to VLED jumper.  Route STEMMA_VPLUS on the back layer
    # so it does not cross the adjacent JST-SH data signals on the front.
    via(board, nets, "STEMMA_VPLUS", 9.0, 29.5)
    via(board, nets, "STEMMA_VPLUS", 17.0, 21.5)
    track(board, nets, "STEMMA_VPLUS", (7.325, 29.5), (9.0, 29.5), 0.2)
    track(board, nets, "STEMMA_VPLUS", (9.0, 29.5), (9.0, 21.5), 0.25, pcbnew.B_Cu)
    track(board, nets, "STEMMA_VPLUS", (9.0, 21.5), (17.0, 21.5), 0.25, pcbnew.B_Cu)
    track(board, nets, "STEMMA_VPLUS", (17.0, 21.5), (17.35, 21.5), 0.2)

    # Connect the VLED side of SJ4 to the injected LED rail, while leaving the
    # solder jumper itself open in copper.
    track(board, nets, "VLED", (18.65, 21.5), (18.65, 14.0), 0.5)
    track(board, nets, "VLED", (18.65, 14.0), (41.2, 14.0), 0.5)

    # Short power jumpers into local decoupling.
    track(board, nets, "VLED", (49.0, 14.0), (49.0, 13.95), 0.5)
    track(board, nets, "GND", (49.0, 12.0), (49.0, 12.05), 0.5)

    text(board, "NeoHEX adapter Rev A", 14.0, 2.0, 1.2)
    text(board, "J1 PH4 PLACEHOLDER", 36.0, 2.0, 0.9)
    text(board, "GND VLED DATA NC", 36.0, 3.5, 0.9)
    text(board, "SJ1 SDA  SJ2 SCL  SJ3 GPIO", 10.0, 19.0, 0.9)
    text(board, "CLOSE ONE", 10.0, 17.5, 0.9)
    text(board, "SJ4 OPEN FOR NEOHEX", 10.0, 12.5, 0.9)
    text(board, "VLED IN", 1.5, 11.0, 0.9)
    text(board, "STEMMA", 10.0, 32.0, 0.9)
    text(board, "GPIO", 41.0, 31.8, 0.9)

    write_project_file()
    pcbnew.SaveBoard(str(OUT_DIR / f"{PROJECT}.kicad_pcb"), board)


if __name__ == "__main__":
    build()
