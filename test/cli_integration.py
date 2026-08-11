#!/usr/bin/env python3
"""End-to-end checks for the installed CCSDSPack command-line tools.

Expected packet bytes are loaded from the committed independent golden vectors.
Malformed cases are derived in Python and use an independent CRC-16/CCITT-FALSE
implementation, never CCSDSPack itself.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def read_hex(path: Path) -> bytes:
    tokens: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0]
        tokens.extend(line.split())
    return bytes(int(token, 16) for token in tokens)


def crc16(data: bytes, polynomial: int = 0x1021, initial: int = 0xFFFF) -> int:
    value = initial
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            value = ((value << 1) ^ polynomial) & 0xFFFF if value & 0x8000 else (value << 1) & 0xFFFF
    return value


def replace_crc(packet: bytearray) -> None:
    value = crc16(bytes(packet[:-2]))
    packet[-2] = value >> 8
    packet[-1] = value & 0xFF


def reference_packet(packet_type: int, apid: int, secondary: bytes, payload: bytes) -> bytes:
    """Construct one independent version-0, unsegmented, CRC16 Space Packet vector."""
    packet_id = (packet_type << 12) | (int(bool(secondary)) << 11) | apid
    sequence = 0xC000
    data_length = len(secondary) + len(payload) + 2 - 1
    without_crc = (
        packet_id.to_bytes(2, "big")
        + sequence.to_bytes(2, "big")
        + data_length.to_bytes(2, "big")
        + secondary
        + payload
    )
    return without_crc + crc16(without_crc).to_bytes(2, "big")


def packet_config(apid: int, data_field_size: int = 2, *, pec: str | None = None) -> str:
    lines = [
        f"ccsds_packet_error_control:string={pec or 'crc16'}",
        f"data_field_size:int={data_field_size}",
        "sync_pattern_enable:bool=false",
        "validation_enable:bool=true",
        "ccsds_version_number:int=0",
        "ccsds_type:bool=false",
        f"ccsds_APID:int={apid}",
        "ccsds_segmented:bool=false",
        "define_secondary_header:bool=false",
    ]
    return "\n".join(lines) + "\n"


def executable(bin_dir: Path, name: str) -> Path:
    suffix = ".exe" if os.name == "nt" else ""
    path = bin_dir / f"{name}{suffix}"
    if not path.is_file():
        raise AssertionError(f"missing CLI executable: {path}")
    return path


def run(command: list[str], expected: int = 0) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, text=True, capture_output=True, check=False)
    if result.returncode != expected:
        raise AssertionError(
            f"command returned {result.returncode}, expected {expected}: {' '.join(command)}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return result


def assert_contains(result: subprocess.CompletedProcess[str], text: str) -> None:
    output = result.stdout + result.stderr
    if text not in output:
        raise AssertionError(f"expected output to contain {text!r}\n{output}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin-dir", type=Path, required=True)
    parser.add_argument("--resources", type=Path, required=True)
    args = parser.parse_args()

    encoder = executable(args.bin_dir.resolve(), "ccsds_encoder")
    decoder = executable(args.bin_dir.resolve(), "ccsds_decoder")
    validator = executable(args.bin_dir.resolve(), "ccsds_validator")
    resources = args.resources.resolve()
    example_configs = Path(__file__).resolve().parents[1] / "example" / "config"

    golden_crc = read_hex(resources / "ccsds_golden_no_secondary_crc16.hex")
    golden_none = read_hex(resources / "ccsds_golden_no_crc.hex")
    golden_rollover = read_hex(resources / "ccsds_golden_rollover_concat_crc16.hex")

    with tempfile.TemporaryDirectory(prefix="ccsdspack-cli-") as temp_name:
        temp = Path(temp_name)
        payload = temp / "payload.bin"
        payload.write_bytes(b"\xAA\x55")
        config0 = temp / "apid0.cfg"
        config0.write_text(packet_config(0), encoding="utf-8")

        encoded_crc = temp / "encoded-crc.bin"
        run([str(encoder), "-i", str(payload), "-o", str(encoded_crc), "-c", str(config0)])
        assert encoded_crc.read_bytes() == golden_crc, "encoder default must match independent CRC16 vector"

        encoded_none = temp / "encoded-none.bin"
        run([
            str(encoder), "-i", str(payload), "-o", str(encoded_none), "-c", str(config0),
            "--packet-error-control", "none",
        ])
        assert encoded_none.read_bytes() == golden_none, "encoder none mode must match independent vector"

        decoded_none = temp / "decoded-none.bin"
        run([
            str(decoder), "-i", str(encoded_none), "-o", str(decoded_none), "-c", str(config0),
            "-e", "none",
        ])
        assert decoded_none.read_bytes() == payload.read_bytes(), "CRC-free decoder round trip failed"

        config1 = temp / "apid1.cfg"
        config1.write_text(packet_config(1, data_field_size=1), encoding="utf-8")
        trailing = b"\xDE\xAD\xBE\xEF"
        stream_with_trailing = temp / "rollover-trailing.bin"
        stream_with_trailing.write_bytes(golden_rollover + trailing)
        decoded = temp / "decoded.bin"
        trailing_output = temp / "trailing.bin"
        result = run([
            str(decoder), "-i", str(stream_with_trailing), "-o", str(decoded),
            "-c", str(config1), "--trailing-output", str(trailing_output),
        ])
        assert decoded.read_bytes() == b"\xAA\xBB", "decoder did not reassemble concatenated packets"
        assert trailing_output.read_bytes() == trailing, "decoder consumed or lost unrelated trailing bytes"
        assert_contains(result, "trailing byte(s) unconsumed")

        valid_stream = temp / "rollover-valid.bin"
        valid_stream.write_bytes(golden_rollover)
        run([str(validator), "-i", str(valid_stream), "-c", str(config1)])
        run([str(validator), "-i", str(encoded_none), "-e", "none"])

        length_bad = bytearray(golden_crc)
        length_bad[4] = 0x00
        length_bad[5] = 0x08
        length_path = temp / "length-bad.bin"
        length_path.write_bytes(length_bad)
        result = run([str(validator), "-i", str(length_path)], expected=18)
        assert_contains(result, "Packet Data Length")

        crc_bad = bytearray(golden_crc)
        crc_bad[6] ^= 0x01
        crc_path = temp / "crc-bad.bin"
        crc_path.write_bytes(crc_bad)
        result = run([str(validator), "-i", str(crc_path)], expected=18)
        assert_contains(result, "CRC16")

        version_bad = bytearray(golden_crc)
        version_bad[0] |= 0x20
        replace_crc(version_bad)
        version_path = temp / "version-bad.bin"
        version_path.write_bytes(version_bad)
        result = run([str(validator), "-i", str(version_path)], expected=18)
        assert_contains(result, "CCSDS version")

        apid_path = temp / "apid-mismatch.bin"
        apid_path.write_bytes(golden_crc)
        result = run([str(validator), "-i", str(apid_path), "-c", str(config1)], expected=18)
        assert_contains(result, "Packet Identification")

        sequence_bad = bytearray(golden_rollover)
        second = 9
        sequence_bad[second + 2] = 0xC0
        sequence_bad[second + 3] = 0x01
        second_packet = bytearray(sequence_bad[second:])
        replace_crc(second_packet)
        sequence_bad[second:] = second_packet
        sequence_path = temp / "sequence-bad.bin"
        sequence_path.write_bytes(sequence_bad)
        result = run([str(validator), "-i", str(sequence_path), "-c", str(config1)], expected=18)
        assert_contains(result, "Sequence count")

        profile_vectors = {
            "generic": reference_packet(0, 42, b"", payload.read_bytes()),
            "pus_a_tc": reference_packet(1, 42, bytes([0x19, 17, 1, 0x42]), payload.read_bytes()),
            "pus_a_tm": reference_packet(0, 42, bytes([0x10, 3, 25, 7, 0x42]), payload.read_bytes()),
            "pus_c_tc": reference_packet(1, 42, bytes([0x29, 17, 1, 0x12, 0x34]), payload.read_bytes()),
            "pus_c_tm_no_time": reference_packet(0, 42, bytes([0x20, 3, 25, 0, 7, 0x12, 0x34]), payload.read_bytes()),
            "pus_c_tm": reference_packet(
                0, 42,
                bytes([0x22, 3, 25, 0, 7, 0x12, 0x34, 0x1E, 1, 2, 3, 4, 0xA0, 0xB0]),
                payload.read_bytes(),
            ),
        }
        for profile_name, expected_wire in profile_vectors.items():
            profile = example_configs / f"{profile_name}.cfg"
            profile_wire = temp / f"{profile_name}.bin"
            profile_output = temp / f"{profile_name}-decoded.bin"
            run([str(encoder), "-i", str(payload), "-o", str(profile_wire), "-c", str(profile)])
            assert profile_wire.read_bytes() == expected_wire, (
                f"{profile_name} encoder differs from the independent vector"
            )
            run([str(decoder), "-i", str(profile_wire), "-o", str(profile_output), "-c", str(profile)])
            assert profile_output.read_bytes() == payload.read_bytes(), f"{profile_name} CLI round trip failed"
            validation = run([str(validator), "-i", str(profile_wire), "-c", str(profile), "-v"])
            if profile_name.startswith("pus_"):
                assert_contains(validation, "PUS secondary header")

        pus_c_tm = example_configs / "pus_c_tm.cfg"
        pus_wire = temp / "pus-c-tm.bin"
        run([str(encoder), "-i", str(payload), "-o", str(pus_wire), "-c", str(pus_c_tm)])
        invalid_pus = bytearray(pus_wire.read_bytes())
        invalid_pus[6] = (invalid_pus[6] & 0x0F) | 0x30
        replace_crc(invalid_pus)
        invalid_pus_path = temp / "pus-version-bad.bin"
        invalid_pus_path.write_bytes(invalid_pus)
        result = run([
            str(validator), "-i", str(invalid_pus_path), "-c", str(pus_c_tm), "-v",
        ], expected=18)
        assert_contains(result, "PUS secondary header")

        help_result = run([str(encoder), "--help"])
        assert_contains(help_result, "--packet-error-control")
        help_result = run([str(decoder), "--help"])
        assert_contains(help_result, "--trailing-output")
        help_result = run([str(validator), "--help"])
        assert_contains(help_result, "Packet template")

    print("CCSDSPack CLI integration tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"CLI integration failure: {error}", file=sys.stderr)
        raise SystemExit(1)
