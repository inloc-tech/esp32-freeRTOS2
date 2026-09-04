#!/usr/bin/env python3
"""Print labels via serial (default) or CUPS, with optional QR rendering in CUPS mode."""

from __future__ import annotations

import argparse
import glob
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


DEFAULT_PRINTER = "Clabel__CT221B"
DEFAULT_SERIAL_PORT = "/dev/ttyUSB0"
DEFAULT_SERIAL_BAUD = 9600
DEFAULT_SERIAL_LANGUAGE = "tspl"
DEFAULT_CUPS_FORMAT = "tspl"
DEFAULT_LABEL_WIDTH = 40
DEFAULT_LABEL_HEIGHT = 30
DEFAULT_TEXT_SIZE = 8
DEFAULT_PADDING = 2
DEFAULT_LEFT_PADDING = 1
DEFAULT_QR_SIZE = 20
DOTS_PER_MM = 8


def require_cmd(name: str) -> None:
    if shutil.which(name) is None:
        print(f"Error: '{name}' not found in PATH", file=sys.stderr)
        sys.exit(1)


def run_cmd(cmd: list[str], check: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(cmd, check=False, text=True, capture_output=True)
    if check and result.returncode != 0:
        print("Error: command failed:", " ".join(cmd), file=sys.stderr)
        if result.stdout:
            print("stdout:", file=sys.stderr)
            print(result.stdout, end="", file=sys.stderr)
        if result.stderr:
            print("stderr:", file=sys.stderr)
            print(result.stderr, end="", file=sys.stderr)
        sys.exit(result.returncode or 1)
    return result


def pick_font_path() -> str | None:
    candidates = [
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
    ]
    for path in candidates:
        if Path(path).exists():
            return path
    return None


def tspl_escape(value: str) -> str:
    return value.replace('"', "'")


def mm_to_dots(value_mm: int) -> int:
    return max(1, int(round(value_mm * DOTS_PER_MM)))


def build_label_lines(text: str) -> list[str]:
    return [
        "device:",
        text,
        "",
        "",
        "Manuf:Inloc",
        "inloc.cloud"
    ]


def estimate_qr_total_modules(payload: str) -> int:
    # Approximate QR version from byte length at ECC L to size/position more reliably.
    capacities_l = [17, 32, 53, 78, 106, 134, 154, 192, 230, 271]
    byte_len = len(payload.encode("utf-8"))
    version = 10
    for idx, cap in enumerate(capacities_l, start=1):
        if byte_len <= cap:
            version = idx
            break
    modules = 17 + 4 * version
    quiet_zone = 8
    return modules + quiet_zone


def tspl_text_scale_from_size(text_size: int) -> int:
    # Map point-like size hint to TSPL multipliers (1..6), tuned for small 40x30 labels.
    # This keeps 8-16 around 1x and requires larger values to step up.
    return max(1, min(6, int(round(text_size / 12))))


def tspl_font_from_size(text_size: int) -> str:
    # Force a smaller bitmap font on compact labels for sizes up to 16.
    # This makes default size 12 significantly smaller than font 0.
    return "8" if text_size <= 16 else "0"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="print_label.py",
        description="Print labels via serial (default) or CUPS.",
    )
    parser.add_argument("-t", "--text", default="", help="Label text to print")
    parser.add_argument(
        "-i",
        "--device-id",
        default="",
        help="Device id for QR URL: https://devices.dev.inloc.cloud/device/<id>",
    )
    parser.add_argument(
        "--qr-url",
        default="",
        help="Full QR URL override (if provided, --device-id is ignored)",
    )
    parser.add_argument(
        "--connection",
        choices=["serial", "cups"],
        default="serial",
        help="Connection mode: serial (default) or cups",
    )
    parser.add_argument(
        "-P",
        "--port",
        default=DEFAULT_SERIAL_PORT,
        help=f"Serial device path (default: {DEFAULT_SERIAL_PORT})",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_SERIAL_BAUD,
        help=f"Serial baud rate (default: {DEFAULT_SERIAL_BAUD})",
    )
    parser.add_argument(
        "--serial-language",
        choices=["tspl", "raw", "escpos"],
        default=DEFAULT_SERIAL_LANGUAGE,
        help="Serial protocol mode: tspl (default), raw text, or escpos",
    )
    parser.add_argument(
        "--serial-debug-hex",
        action="store_true",
        help="Print hex preview of serial bytes before sending",
    )
    parser.add_argument(
        "-p",
        "--printer",
        default=DEFAULT_PRINTER,
        help=f"CUPS printer queue name (default: {DEFAULT_PRINTER})",
    )
    parser.add_argument(
        "--cups-format",
        choices=["tspl", "pdf", "text"],
        default=DEFAULT_CUPS_FORMAT,
        help="CUPS data format: tspl (default), pdf, or text",
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="List likely serial ports and exit",
    )
    parser.add_argument(
        "--list-printers",
        action="store_true",
        help="List available CUPS printers and exit",
    )
    parser.add_argument(
        "-c",
        "--copies",
        type=int,
        default=1,
        help="Number of copies (default: 1)",
    )
    parser.add_argument(
        "--label-width",
        type=int,
        default=DEFAULT_LABEL_WIDTH,
        help=f"Label width in mm (default: {DEFAULT_LABEL_WIDTH})",
    )
    parser.add_argument(
        "--label-height",
        type=int,
        default=DEFAULT_LABEL_HEIGHT,
        help=f"Label height in mm (default: {DEFAULT_LABEL_HEIGHT})",
    )
    parser.add_argument(
        "--text-size",
        type=int,
        default=DEFAULT_TEXT_SIZE,
        help=f"Text size hint (TSPL scale / PDF points, default: {DEFAULT_TEXT_SIZE})",
    )
    parser.add_argument(
        "--qr-size",
        type=int,
        default=DEFAULT_QR_SIZE,
        help=f"QR square size in mm (default: {DEFAULT_QR_SIZE})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show print command without printing",
    )
    return parser.parse_args()


def validate_positive(name: str, value: int) -> None:
    if value <= 0:
        print(f"Error: --{name} must be a positive integer", file=sys.stderr)
        sys.exit(1)


def list_printers() -> None:
    print("Available printers:")
    result = run_cmd(["lpstat", "-p"], check=False)
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)


def list_serial_ports() -> None:
    print("Likely serial ports:")
    ports = sorted(set(glob.glob("/dev/tty.*") + glob.glob("/dev/cu.*") + glob.glob("/dev/ttyUSB*")))
    if not ports:
        print("(none found)")
        return
    for port in ports:
        print(port)


def ensure_printer_exists(printer: str) -> None:
    result = run_cmd(["lpstat", "-p", printer], check=False)
    if result.returncode != 0:
        print(f"Warning: Printer '{printer}' not found in CUPS queue list.")
        print("Run with --list-printers and pass --printer with the exact queue name.")


def build_qr_url(qr_url: str, device_id: str) -> str:
    if qr_url:
        return qr_url
    if device_id:
        return f"https://devices.dev.inloc.cloud/device/{device_id}"
    return ""


def create_cups_print_file(
    tmp_dir: Path,
    text: str,
    qr_url: str,
    cups_format: str,
    label_width: int,
    label_height: int,
    text_size: int,
    qr_size: int,
    padding: int,
) -> tuple[Path, bool]:
    label_lines = build_label_lines(text)
    header_text = "\n".join(label_lines)

    if cups_format == "tspl":
        tspl_file = tmp_dir / "label.tspl"
        tspl_file.write_bytes(
            build_serial_payload(
                text,
                qr_url,
                "tspl",
                label_width,
                label_height,
                text_size,
                qr_size,
                padding,
            )
        )
        return tspl_file, True

    text_file = tmp_dir / "label.txt"
    text_file.write_text(header_text + "\n", encoding="utf-8")

    if cups_format == "text":
        if qr_url:
            text_file.write_text(header_text + "\n" + qr_url + "\n", encoding="utf-8")
        return text_file, False

    if not qr_url:
        return text_file, False

    qr_png = tmp_dir / "qr.png"
    qr_resized_png = tmp_dir / "qr_resized.png"
    label_png = tmp_dir / "label.png"
    final_png = tmp_dir / "final.png"
    final_pdf = tmp_dir / "final.pdf"
    font_path = pick_font_path()
    label_width_px = mm_to_dots(label_width)
    label_height_px = mm_to_dots(label_height)
    qr_size_px = mm_to_dots(qr_size)
    padding_px = mm_to_dots(padding)

    run_cmd(["qrencode", "-o", str(qr_png), qr_url])
    draw_cmd = [
        "magick",
        "-size",
        f"{label_width_px}x{label_height_px}",
        "xc:white",
        "-fill",
        "black",
        "-gravity",
        "NorthWest",
        "-pointsize",
        str(text_size),
    ]
    if font_path:
        draw_cmd.extend(["-font", font_path])
    draw_cmd.extend(
        [
            "-annotate",
            f"+{padding_px}+{padding_px}",
            header_text,
            str(label_png),
        ]
    )
    run_cmd(draw_cmd)
    run_cmd(["magick", str(qr_png), "-resize", f"{qr_size_px}x{qr_size_px}", str(qr_resized_png)])
    run_cmd(
        [
            "magick",
            str(label_png),
            str(qr_resized_png),
            "-gravity",
            "SouthEast",
            "-geometry",
            f"+{padding_px}+{padding_px}",
            "-composite",
            str(final_png),
        ]
    )
    run_cmd(["magick", str(final_png), str(final_pdf)])
    return final_pdf, False


def set_serial_mode(port: str, baud: int) -> None:
    # macOS uses -f, Linux commonly uses -F.
    cmd_sets = [
        ["stty", "-f", port, str(baud), "cs8", "-cstopb", "-parenb", "raw", "-echo"],
        ["stty", "-F", port, str(baud), "cs8", "-cstopb", "-parenb", "raw", "-echo"],
    ]
    for cmd in cmd_sets:
        if run_cmd(cmd, check=False).returncode == 0:
            return
    print(f"Warning: could not configure serial mode for {port}; attempting write anyway.")


def maybe_prefer_cu_port(port: str) -> str:
    # On macOS, /dev/cu.* is generally the correct endpoint for outgoing writes.
    if port.startswith("/dev/tty."):
        cu_port = "/dev/cu." + port[len("/dev/tty.") :]
        if Path(cu_port).exists():
            print(f"Info: using {cu_port} instead of {port} for output.")
            return cu_port
    return port


def build_serial_payload(
    text: str,
    qr_url: str,
    language: str,
    label_width: int,
    label_height: int,
    text_size: int,
    qr_size: int,
    padding: int,
) -> bytes:
    label_lines = build_label_lines(text)

    if language == "raw":
        raw = "\n".join(label_lines) + "\n"
        if qr_url:
            raw += f"QR: {qr_url}\n"
        return raw.encode("utf-8", errors="replace")

    if language == "escpos":
        # Basic ESC/POS text mode. QR is included as text fallback.
        esc = b"\x1b@"
        body = "\n".join(label_lines) + "\n"
        if qr_url:
            body += qr_url + "\n"
        return esc + body.encode("utf-8", errors="replace") + b"\n\n\n"

    # TSPL label command set for thermal label printers.
    width_mm = label_width
    height_mm = label_height
    label_width_dots = mm_to_dots(label_width)
    label_height_dots = mm_to_dots(label_height)
    text_x = mm_to_dots(DEFAULT_LEFT_PADDING)
    text_y = mm_to_dots(padding)
    text_font = tspl_font_from_size(text_size)
    text_scale = tspl_text_scale_from_size(text_size)
    # Keep spacing readable while preventing extreme overflow on small labels.
    preferred_step = mm_to_dots(4 * text_scale)
    max_step = max(mm_to_dots(3), (label_height_dots - (2 * text_y)) // max(1, len(label_lines) - 1))
    line_step = min(preferred_step, max_step)
    qr_size_dots = mm_to_dots(qr_size)

    qr_total_modules = estimate_qr_total_modules(qr_url) if qr_url else 45
    qr_cell = max(2, min(8, qr_size_dots // max(1, qr_total_modules)))
    qr_estimated_dots = qr_total_modules * qr_cell
    qr_x = max(0, label_width_dots - qr_estimated_dots)
    qr_y = max(0, label_height_dots - qr_estimated_dots)
    lines = [
        f"SIZE {width_mm} mm,{height_mm} mm",
        "GAP 2 mm,0 mm",
        "DENSITY 8",
        "REFERENCE 0,0",
        "DIRECTION 1",
        "CLS",
    ]
    for idx, line in enumerate(label_lines):
        if not line:
            continue
        y = text_y + (idx * line_step)
        lines.append(f'TEXT {text_x},{y},"{text_font}",0,{text_scale},{text_scale},"{tspl_escape(line)}"')
    if qr_url:
        lines.append(f'QRCODE {qr_x},{qr_y},L,{qr_cell},M,0,M2,S2,"{tspl_escape(qr_url)}"')
    lines.append("PRINT 1,1")
    return ("\r\n".join(lines) + "\r\n").encode("ascii", errors="ignore")


def print_serial_job(
    port: str,
    baud: int,
    payload: bytes,
    copies: int,
    dry_run: bool,
    debug_hex: bool,
) -> None:
    eff_port = maybe_prefer_cu_port(port)

    if debug_hex:
        preview = payload[:256]
        print("Serial bytes preview (hex):", preview.hex(" "))

    if dry_run:
        print(f"Dry run serial target: {eff_port} @ {baud} baud")
        print(f"Dry run payload length: {len(payload)} bytes")
        print(f"Dry run payload preview: {payload[:200]!r}")
        return

    if not Path(eff_port).exists():
        print(f"Error: serial port not found: {eff_port}", file=sys.stderr)
        sys.exit(1)

    set_serial_mode(eff_port, baud)
    print("Sending label to serial printer...")
    for _ in range(copies):
        with open(eff_port, "wb", buffering=0) as handle:
            handle.write(payload)
    print("Print job submitted.")


def print_job(printer: str, copies: int, print_file: Path, dry_run: bool, raw_mode: bool = False) -> None:
    cmd = ["lp", "-d", printer, "-n", str(copies)]
    if raw_mode:
        cmd.extend(["-o", "raw"])
    cmd.append(str(print_file))
    if dry_run:
        print("Dry run command:", " ".join(cmd))
        return
    print("Sending label to printer...")
    result = run_cmd(cmd)
    if result.stdout:
        print(result.stdout, end="")
    print("Print job submitted.")


def main() -> int:
    args = parse_args()

    if args.list_ports:
        list_serial_ports()
        return 0

    if args.list_printers:
        require_cmd("lpstat")
        list_printers()
        return 0

    validate_positive("copies", args.copies)
    validate_positive("baud", args.baud)
    validate_positive("label-width", args.label_width)
    validate_positive("label-height", args.label_height)
    validate_positive("text-size", args.text_size)
    validate_positive("qr-size", args.qr_size)

    qr_url = build_qr_url(args.qr_url, args.device_id)
    text = args.text if args.text else (f"uid:{args.device_id}" if args.device_id else "")

    if not text:
        print(
            "Error: --text is required (or pass --device-id to auto-generate text)",
            file=sys.stderr,
        )
        return 1

    print(f"Connection: {args.connection}")
    print(f"Copies:  {args.copies}")
    print(f"Text:    {text}")
    if qr_url:
        print(f"QR URL:  {qr_url}")

    if args.connection == "serial":
        print(f"Port:    {args.port}")
        print(f"Baud:    {args.baud}")
        print(f"Proto:   {args.serial_language}")
        serial_payload = build_serial_payload(
            text,
            qr_url,
            args.serial_language,
            args.label_width,
            args.label_height,
            args.text_size,
            args.qr_size,
            DEFAULT_PADDING,
        )
        print_serial_job(
            args.port,
            args.baud,
            serial_payload,
            args.copies,
            args.dry_run,
            args.serial_debug_hex,
        )
        return 0

    require_cmd("lp")
    require_cmd("lpstat")
    if qr_url and args.cups_format == "pdf":
        require_cmd("qrencode")
        require_cmd("magick")

    ensure_printer_exists(args.printer)
    print(f"Printer: {args.printer}")
    print(f"CUPS:    {args.cups_format}")

    with tempfile.TemporaryDirectory(prefix="ct221b-label.") as tmp:
        print_file, raw_mode = create_cups_print_file(
            Path(tmp),
            text,
            qr_url,
            args.cups_format,
            args.label_width,
            args.label_height,
            args.text_size,
            args.qr_size,
            DEFAULT_PADDING,
        )
        print_job(args.printer, args.copies, print_file, args.dry_run, raw_mode=raw_mode)

    return 0


if __name__ == "__main__":
    sys.exit(main())
