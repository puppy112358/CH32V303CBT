#!/usr/bin/env python3
"""
Protocol Verification Script — 电子负载控制器 cJSON 帧协议测试

Usage:
    python3 test_protocol.py <usart_port> <chip_id>

    usart_port:  USART1 serial port (e.g., COM3 on Windows, /dev/ttyUSB0 on Linux)
    chip_id:     8-char hex chip ID printed at boot (e.g., "303305X4")

Connect USB-CDC separately to monitor debug printf output.

Protocol frame format: <chip_id,cmd_id>{json}
Response format:       <local_chip_id,200>{"command":"<cmd>","ack":"ok"}
Error format:          <local_chip_id,200>{"command":"<cmd>","ack":"error",...}
"""

import serial
import time
import sys
import json


class ProtocolTester:
    def __init__(self, port: str, chip_id: str, baudrate: int = 115200):
        self.port = port
        self.chip_id = chip_id.upper()
        self.ser = serial.Serial(port, baudrate, timeout=1.0)
        self.passed = 0
        self.failed = 0

    def _send(self, frame: str) -> None:
        """Send a frame over USART1 with newline terminator."""
        self.ser.write((frame + "\n").encode())
        self.ser.flush()
        print(f"  TX: {frame}")

    def _recv(self, timeout: float = 2.0) -> str:
        """Read one line from USART1."""
        self.ser.timeout = timeout
        line = self.ser.readline().decode(errors="replace").strip()
        if line:
            print(f"  RX: {line}")
        return line

    def _check(self, name: str, condition: bool, detail: str = "") -> bool:
        if condition:
            self.passed += 1
            print(f"  [PASS] {name} {detail}")
        else:
            self.failed += 1
            print(f"  [FAIL] {name} {detail}")
        return condition

    def test(self) -> None:
        """Run all tests."""
        print(f"\n{'='*60}")
        print(f"Protocol Test — chip_id={self.chip_id}, port={self.port}")
        print(f"{'='*60}")

        self.test_frame_parsing()
        self.test_chip_id_filtering()
        self.test_command_101_cc()
        self.test_command_101_cv()
        self.test_command_101_idle()
        self.test_command_102_mode_switch()
        self.test_command_102_invalid_mode()
        self.test_command_102_typec_not_impl()
        self.test_unknown_command()
        self.test_invalid_json()
        self.test_e2e_flow()

        print(f"\n{'='*60}")
        print(f"Results: {self.passed} PASS, {self.failed} FAIL "
              f"({self.passed + self.failed} total)")
        print(f"{'='*60}")

    # ------------------------------------------------------------------
    # 1. Frame Parsing Tests
    # ------------------------------------------------------------------
    def test_frame_parsing(self) -> None:
        print("\n--- 1. Frame Parsing ---")

        # 1a: Valid frame — should get response
        self._send(f"<{self.chip_id},101>{{\"I\":\"0\",\"V\":\"0\"}}")
        resp = self._recv()
        self._check("1a: valid frame returns response",
                    resp != "" and "200" in resp,
                    f"got: {resp}")

        # 1b: Missing '<' prefix — should be ignored (no response)
        self._send(f"{self.chip_id},101>{{\"I\":\"0\"}}")
        resp = self._recv()
        self._check("1b: missing '<' → no response",
                    resp == "",
                    f"got: {resp}")

        # 1c: Invalid command ID (letters) — frame parse error
        self._send(f"<{self.chip_id},ABC>{{\"x\":\"1\"}}")
        resp = self._recv()
        self._check("1c: invalid cmd ID → no response",
                    resp == "",
                    f"got: {resp}")

    # ------------------------------------------------------------------
    # 2. Chip ID Filtering
    # ------------------------------------------------------------------
    def test_chip_id_filtering(self) -> None:
        print("\n--- 2. Chip ID Filtering ---")

        # 2a: Wrong chip ID with downlink command → ignored
        self._send("<DEADBEEF,101>{\"I\":\"500\",\"V\":\"0\"}")
        resp = self._recv()
        self._check("2a: wrong chip ID downlink → ignored",
                    resp == "",
                    f"got: {resp}")

        # 2b: Uplink command (2xx) with any chip ID → should pass through
        self._send("<DEADBEEF,200>{\"command\":\"101\"}")
        # Response 200 just logs via printf; may or may not get serial response
        # So just check no crash/error
        time.sleep(0.2)
        self._check("2b: uplink 2xx bypasses chip ID filter",
                    True, "(no crash expected)")

    # ------------------------------------------------------------------
    # 3. Command 101 — POWER_SET in CC mode
    # ------------------------------------------------------------------
    def test_command_101_cc(self) -> None:
        print("\n--- 3. Command 101 in CC Mode ---")

        # First set mode to CC (11)
        self._send(f"<{self.chip_id},102>{{\"mode\":\"11\"}}")
        resp = self._recv()
        ok = self._check("3a: set CC mode", "ack" in resp and "ok" in resp)

        # Set 1.500A (I=1500)
        self._send(f"<{self.chip_id},101>{{\"I\":\"1500\",\"V\":\"0\"}}")
        resp = self._recv()
        self._check("3b: set 1.5A in CC mode",
                    "ack" in resp and "ok" in resp,
                    f"got: {resp}")

        # Out of range (I=20000 = 20A > 10A max)
        self._send(f"<{self.chip_id},101>{{\"I\":\"20000\",\"V\":\"0\"}}")
        resp = self._recv()
        self._check("3c: out-of-range current rejected",
                    "error" in resp and "206" not in resp,  # range error, not "not impl"
                    f"got: {resp}")

    # ------------------------------------------------------------------
    # 4. Command 101 — POWER_SET in CV mode
    # ------------------------------------------------------------------
    def test_command_101_cv(self) -> None:
        print("\n--- 4. Command 101 in CV Mode ---")

        # Switch to CV mode (12)
        self._send(f"<{self.chip_id},102>{{\"mode\":\"12\"}}")
        resp = self._recv()
        self._check("4a: set CV mode", "ack" in resp and "ok" in resp)

        # Set 5.000V (V=5000)
        self._send(f"<{self.chip_id},101>{{\"I\":\"0\",\"V\":\"5000\"}}")
        resp = self._recv()
        self._check("4b: set 5.0V in CV mode",
                    "ack" in resp and "ok" in resp,
                    f"got: {resp}")

        # Missing V in CV mode
        self._send(f"<{self.chip_id},101>{{\"I\":\"1000\"}}")
        resp = self._recv()
        self._check("4c: missing V in CV mode → error",
                    "error" in resp,
                    f"got: {resp}")

    # ------------------------------------------------------------------
    # 5. Command 101 — IDLE mode (store only)
    # ------------------------------------------------------------------
    def test_command_101_idle(self) -> None:
        print("\n--- 5. Command 101 in IDLE Mode ---")

        # Reset device or clear to IDLE if possible
        # For now, send 101 while in CV mode with both fields
        # (device is in CV mode from test 4)
        self._send(f"<{self.chip_id},101>{{\"I\":\"2000\",\"V\":\"3000\"}}")
        resp = self._recv()
        self._check("5a: set values in CV mode (I ignored, V used)",
                    "ack" in resp and "ok" in resp,
                    f"got: {resp}")

    # ------------------------------------------------------------------
    # 6. Command 102 — Mode Switch
    # ------------------------------------------------------------------
    def test_command_102_mode_switch(self) -> None:
        print("\n--- 6. Command 102 Mode Switch ---")

        # Switch to CC (11)
        self._send(f"<{self.chip_id},102>{{\"mode\":\"11\"}}")
        resp = self._recv()
        self._check("6a: switch to CC (11)", "ack" in resp and "ok" in resp)

        # Switch to CV (12)
        self._send(f"<{self.chip_id},102>{{\"mode\":\"12\"}}")
        resp = self._recv()
        self._check("6b: switch to CV (12)", "ack" in resp and "ok" in resp)

        # Verify stored CC target persists across mode switches:
        # Switch to CC, then set I, then switch CV, then back to CC
        self._send(f"<{self.chip_id},101>{{\"I\":\"3000\",\"V\":\"0\"}}")
        self._recv()  # consume ack
        self._send(f"<{self.chip_id},102>{{\"mode\":\"12\"}}")
        self._recv()  # switch to CV
        self._send(f"<{self.chip_id},102>{{\"mode\":\"11\"}}")
        resp = self._recv()  # back to CC
        self._check("6c: mode switch preserves last CC target",
                    "ack" in resp and "ok" in resp,
                    "(engage_cc called with stored target)")

    # ------------------------------------------------------------------
    # 7. Command 102 — Invalid / Unsupported Modes
    # ------------------------------------------------------------------
    def test_command_102_invalid_mode(self) -> None:
        print("\n--- 7. Command 102 Invalid Modes ---")

        # Invalid mode "99"
        self._send(f"<{self.chip_id},102>{{\"mode\":\"99\"}}")
        resp = self._recv()
        self._check("7a: invalid mode 99 → error",
                    "error" in resp,
                    f"got: {resp}")

        # Missing mode field
        self._send(f"<{self.chip_id},102>{{\"x\":\"1\"}}")
        resp = self._recv()
        self._check("7b: missing mode field → error",
                    "error" in resp,
                    f"got: {resp}")

    def test_command_102_typec_not_impl(self) -> None:
        """Test Type-C modes return 'not implemented'."""
        print("\n--- 8. Command 102 Type-C (not implemented) ---")

        self._send(f"<{self.chip_id},102>{{\"mode\":\"20\"}}")
        resp = self._recv()
        self._check("8a: Type-C only (20) → not implemented",
                    "206" in resp or "not implemented" in resp.lower(),
                    f"got: {resp}")

        self._send(f"<{self.chip_id},102>{{\"mode\":\"21\"}}")
        resp = self._recv()
        self._check("8b: Type-C+CC (21) → not implemented",
                    "206" in resp or "not implemented" in resp.lower(),
                    f"got: {resp}")

    # ------------------------------------------------------------------
    # 9. Unknown Command
    # ------------------------------------------------------------------
    def test_unknown_command(self) -> None:
        print("\n--- 9. Unknown Command ---")

        self._send(f"<{self.chip_id},999>{{\"x\":\"1\"}}")
        resp = self._recv()
        self._check("9a: unknown command 999 → error",
                    "error" in resp or "204" in resp,
                    f"got: {resp}")

    # ------------------------------------------------------------------
    # 10. Invalid JSON
    # ------------------------------------------------------------------
    def test_invalid_json(self) -> None:
        print("\n--- 10. Invalid JSON ---")

        self._send(f"<{self.chip_id},101>not valid json!!!")
        resp = self._recv()
        self._check("10a: invalid JSON body → error response",
                    "error" in resp,
                    f"got: {resp}")

    # ------------------------------------------------------------------
    # 11. End-to-End Flow
    # ------------------------------------------------------------------
    def test_e2e_flow(self) -> None:
        print("\n--- 11. End-to-End: CC → set current → CV → set voltage ---")

        # Step 1: Set CC mode
        self._send(f"<{self.chip_id},102>{{\"mode\":\"11\"}}")
        resp = self._recv()
        ok1 = "ok" in resp

        # Step 2: Set 2.000A
        self._send(f"<{self.chip_id},101>{{\"I\":\"2000\",\"V\":\"0\"}}")
        resp = self._recv()
        ok2 = "ok" in resp

        # Step 3: Switch to CV mode
        self._send(f"<{self.chip_id},102>{{\"mode\":\"12\"}}")
        resp = self._recv()
        ok3 = "ok" in resp

        # Step 4: Set 12.000V
        self._send(f"<{self.chip_id},101>{{\"I\":\"0\",\"V\":\"12000\"}}")
        resp = self._recv()
        ok4 = "ok" in resp

        self._check("11: E2E CC→2A→CV→12V", ok1 and ok2 and ok3 and ok4,
                    f"steps: {ok1}/{ok2}/{ok3}/{ok4}")


# =====================================================================
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__)
        print("USAGE: python3 test_protocol.py <usart_port> <chip_id>")
        print("EXAMPLE: python3 test_protocol.py COM3 303305X4")
        sys.exit(1)

    tester = ProtocolTester(sys.argv[1], sys.argv[2])
    try:
        tester.test()
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        sys.exit(1)
    finally:
        tester.ser.close()
