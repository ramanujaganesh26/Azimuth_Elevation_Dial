# Azimuth & Elevation Command Receiver

A high-performance, receive-only Ground Control / Gimbal Monitoring Station desktop application developed with **Qt 6.8 (C++17), QML, Qt Quick Controls, and CMake**.

The application receives continuous azimuth and elevation command packets from an external transmitter (or gimbal controller) and displays the telemetry live on two custom animated engineering dials. It features hardware transport management (Serial, UDP, TCP Client, TCP Server), raw HEX packet stream inspection, incremental framing, CRC-16/CCITT-FALSE validation, and a bounded communication event log.

---

## 1. Features & Architectural Highlights

- **Strict Receive-Only Enforcement**: The application operates solely in monitor mode (`QIODevice::ReadOnly`). No transmit buttons, no manual command injection, no slider-driven targets, and no automatic ACK/PING responses.
- **Unified Transport Layer (`ITransport`)**:
  - **Serial (`QSerialPort`)**: Asynchronous non-blocking serial reception with selectable port, baud rate (9600 to 460800), 8N1 default, parity, stop bits, and flow control.
  - **UDP (`QUdpSocket`)**: Continuous datagram reception on configurable local bind IP and port (default `0.0.0.0:14550`).
  - **TCP (`QTcpSocket` / `QTcpServer`)**: Dual mode support:
    - *TCP Client*: Connects to remote endpoint (default `127.0.0.1:5000`).
    - *TCP Server*: Listens on local endpoint (default `0.0.0.0:5000`) and accepts incoming transmitter client streams.
- **Robust Incremental Packet Stream Parser**:
  - Recovers synchronization after arbitrary line noise.
  - Transparently handles concatenated packets (multiple packets in a single read) and fragmented packets (packets split across multiple read buffers).
  - Enforces payload bounds ($\le 64$ bytes) and strict 4-byte payload for angle commands.
  - Validates 10-bit raw range ($0 - 1023$, upper 6 bits must be zero). Invalid values and CRC mismatches never corrupt the last valid display angles.
- **Live Dial Gauges**:
  - **Azimuth Dial (360° Circular)**: Markings at 0°, 30°, 60°, ..., 330°, 0° reference at top, digital angle readout, 10-bit raw readout, and **shortest-path unwrapping algorithm** to eliminate 359° reverse spin during 359° $\leftrightarrow$ 0° transitions.
  - **Elevation Dial (-90° to +90°)**: Markings at -90°, -60°, -30°, 0°, +30°, +60°, +90°, level 0° reference at center, distinct positive (cyan/sky blue) and negative (amber/orange) pitch sectors.
- **Telemetry Freshness & Watchdog**:
  - Watchdog timer (default 2000 ms) automatically flags telemetry as `STALE` if incoming packet rate drops, preventing outdated measurements from appearing live.
- **Packet Analyzer & Communication Log**:
  - Latest received raw stream bytes and complete packet HEX breakdown.
  - Decoded sequence number, message type, payload length, and received vs. calculated CRC comparison.
  - High-performance, bounded 1000-entry `QAbstractListModel` with Auto-Scroll, Pause/Resume, and Clear controls.
- **Loopback Simulation Source**: Built-in 10 Hz simulated packet generator with clear `[SIMULATION]` tagging for self-testing without external hardware.

---

## 2. Directory Structure

```text
Azimuth_Elevation/
├── CMakeLists.txt                # CMake project definition (Qt 6.8, MSVC / GCC / Clang)
├── README.md                     # Technical documentation & protocol specification
├── main.cpp                      # Application entry point & QML engine setup
├── Main.qml                      # Main dashboard layout
├── components/
│   ├── AngleDial.qml             # 360-degree Azimuth gauge with shortest-path rotation
│   ├── ElevationDial.qml         # -90° to +90° Elevation gauge with positive/negative zones
│   ├── CommunicationPanel.qml    # Serial, UDP, TCP Client/Server configuration & controls
│   ├── PacketAnalyzer.qml        # Raw HEX inspector, CRC status, and decoded telemetry
│   └── ReceiveLog.qml            # Real-time event log with auto-scroll and pause
├── src/
│   ├── PacketProtocol.h          # Protocol framing, CRC-16, and angle conversion headers
│   ├── PacketProtocol.cpp        # Protocol implementation & stream parsing logic
│   ├── ITransport.h              # Common receive-only transport interface
│   ├── SerialTransport.h         # QSerialPort implementation
│   ├── SerialTransport.cpp
│   ├── UdpTransport.h            # QUdpSocket implementation
│   ├── UdpTransport.cpp
│   ├── TcpTransport.h            # QTcpSocket / QTcpServer implementation
│   ├── TcpTransport.cpp
│   ├── LogModel.h                # High-performance circular QAbstractListModel
│   ├── LogModel.cpp
│   ├── ReceiverController.h      # QML controller exposing properties and signals
│   └── ReceiverController.cpp
└── tests/
    └── tst_protocol.cpp          # QtTest unit tests for CRC, framing, and angle scaling
```

---

## 3. Protocol Specification

### 3.1 Packet Wire Format

All multi-byte fields are transmitted **Little-Endian**.

| Offset | Field | Size | Description |
|---|---|---|---|
| `0` | Sync Byte 1 | 1 byte | Fixed `0xAA` |
| `1` | Sync Byte 2 | 1 byte | Fixed `0x55` |
| `2` | Message Type | 1 byte | `0x01`: ANGLE_COMMAND<br>`0x02`: ANGLE_TELEMETRY<br>`0x03`: ACK<br>`0x04`: PING |
| `3` | Payload Length ($L$) | 1 byte | Payload byte count ($0 \le L \le 64$) |
| `4` | Sequence Number (Low) | 1 byte | Sequence number low byte |
| `5` | Sequence Number (High)| 1 byte | Sequence number high byte |
| `6` | Payload | $L$ bytes | Message payload data |
| `6 + L` | CRC-16 (Low) | 1 byte | CRC low byte |
| `7 + L` | CRC-16 (High) | 1 byte | CRC high byte |

**Total packet size**: $8 + L$ bytes (Minimum: 8 bytes for $L=0$; 12 bytes for angle packets).

### 3.2 Angle Payload Format

For `ANGLE_COMMAND` (`0x01`) and `ANGLE_TELEMETRY` (`0x02`), the payload length must be exactly 4 bytes ($L=4$):

| Byte Offset in Payload | Field | Description |
|---|---|---|
| `0 - 1` | Azimuth Raw | 16-bit unsigned little-endian ($0 - 1023$). Upper 6 bits must be 0. |
| `2 - 3` | Elevation Raw | 16-bit unsigned little-endian ($0 - 1023$). Upper 6 bits must be 0. |

Any angle packet with $L \ne 4$ or raw value $> 1023$ is rejected as malformed and logged without updating the dials.

### 3.3 CRC-16/CCITT-FALSE Specification

- **Polynomial**: `0x1021` ($x^{16} + x^{12} + x^5 + 1$)
- **Initial Value**: `0xFFFF`
- **Reflection In**: `False`
- **Reflection Out**: `False`
- **Final XOR**: `0x0000`
- **Checksum Span**: Calculated over **Message Type**, **Payload Length**, **Sequence Number (2 bytes)**, and **Payload ($L$ bytes)**. The two leading sync bytes (`0xAA 0x55`) are **excluded**.
- **Standard Verification Vector**: The ASCII string `"123456789"` (`0x31 0x32 0x33 0x34 0x35 0x36 0x37 0x38 0x39`) evaluates to CRC `0x29B1`.

### 3.4 Angle Scaling & Conversion

#### Azimuth (0.0° to 360.0°)
$$\text{azimuthDegrees} = \frac{\text{azimuthRaw} \times 360.0}{1023.0}$$

**Transmitter Inverse Mapping**:
$$\text{azimuthRaw} = \operatorname{clamp}\left(\operatorname{round}\left(\frac{\text{azimuthDegrees} \times 1023.0}{360.0}\right), 0, 1023\right)$$

#### Elevation (-90.0° to +90.0°)
$$\text{elevationDegrees} = \frac{\text{elevationRaw} \times 180.0}{1023.0} - 90.0$$

**Transmitter Inverse Mapping**:
$$\text{elevationRaw} = \operatorname{clamp}\left(\operatorname{round}\left(\frac{(\text{elevationDegrees} + 90.0) \times 1023.0}{180.0}\right), 0, 1023\right)$$

---

## 4. Building the Application

### 4.1 Prerequisites
- **Qt 6.5+** (tested and verified with **Qt 6.8.3 LTS**).
- Required Qt modules: `Quick`, `QuickControls2`, `SerialPort`, `Network`, `Test`.
- **C++17** compiler: MSVC 2022 (v143+), GCC 11+, or Clang 13+.
- **CMake 3.21+** and **Ninja** (or NMake / Make).

### 4.2 Building with CMake Command Line (Windows / MSVC)

```cmd
:: 1. Open the Visual Studio 2022 x64 Developer Command Prompt
:: 2. Navigate to the project root
cd C:\Users\RAM\Documents\Azimuth_Elevation

:: 3. Configure CMake with Ninja
cmake -S . -B build -GNinja -DCMAKE_PREFIX_PATH="C:\Qt\6.8.3\msvc2022_64"

:: 4. Build executable and tests
ninja -C build

:: 5. Run Unit Tests
set PATH=C:\Qt\6.8.3\msvc2022_64\bin;%PATH%
build\ReceiverTests.exe -txt
```

### 4.3 Building in Qt Creator
1. Open Qt Creator.
2. Select **File > Open File or Project...** and select `CMakeLists.txt`.
3. Choose the **Desktop Qt 6.8.3 MSVC2022 64bit** (or MinGW 64bit) kit.
4. Click **Configure Project**.
5. Press **Ctrl + B** to build, and **Ctrl + R** to run.

---

## 5. Transport Configuration & Setup

### 5.1 Serial Port
1. Select the **SERIAL** tab in the Interface Select panel.
2. Click **Refresh** to populate available COM ports.
3. Select your serial device (e.g., `COM3`).
4. Select the matching baud rate (default: `115200`). Standard 8N1 parameters are set by default.
5. Click **CONNECT**.

### 5.2 UDP Receiver
1. Select the **UDP** tab.
2. Set the Bind IP (`0.0.0.0` listens on all network interfaces).
3. Set the Listening Port (default: `14550`).
4. Click **BIND / LISTEN**.
5. Configure your external transmitter to send UDP datagrams to this machine's IP address on the specified port.

### 5.3 TCP Client Mode
1. Select the **TCP CLIENT** tab.
2. Enter the remote server IP address (e.g., `192.168.1.100` or `127.0.0.1`) and remote port (e.g., `5000`).
3. Click **CONNECT**.

### 5.4 TCP Server Mode
1. Select the **TCP SERVER** tab.
2. Set the Bind IP (`0.0.0.0`) and Listening Port (default: `5000`).
3. Click **BIND / LISTEN**. The receiver will accept incoming TCP connections from transmitters and parse the continuous byte stream.

### 5.5 Simulation / Self-Test Mode
- Click the **SIMULATION OFF / ON** toggle in the top-right of the Interface Select panel.
- The internal packet generator immediately outputs valid 10 Hz packets with smooth azimuth rotation and elevation oscillation.
- All telemetry gauges, hex displays, CRC analyzers, and logs will update in real time.

---

## 6. Unit Testing Summary

The test target `ReceiverTests` executes 11 comprehensive automated tests using `QtTest`:
1. `testCrcKnownVector`: Verifies CRC-16/CCITT-FALSE against ASCII vector `"123456789"` $\rightarrow$ `0x29B1`.
2. `testValidAnglePacket`: Verifies encoding and decoding of an angle packet ($180.0^\circ, 0.0^\circ$).
3. `testCrcRejection`: Verifies rejection and error flag when CRC bytes or payload are corrupt.
4. `testIncompletePacket`: Verifies that partial packets remain in the buffer until all bytes arrive.
5. `testConcatenatedPackets`: Verifies seamless multi-packet extraction from a single buffer.
6. `testInvalidPayloadLength`: Verifies rejection of headers declaring payload length $> 64$.
7. `testInvalid10BitValues`: Verifies rejection of raw values $> 1023$ (upper 6 bits non-zero).
8. `testSyncRecoveryAfterNoise`: Verifies parser drops leading garbage bytes and locks onto `0xAA 0x55`.
9. `testAngleConversions`: Validates minimum ($0$), midpoint ($511/512$), and maximum ($1023$) scaling.

---

## 7. Troubleshooting

| Issue | Cause | Solution |
|---|---|---|
| Dials do not move | Telemetry marked `STALE` or disconnected | Verify transmitter baud rate/IP port and check CRC error counter in the header. |
| CRC Error count increases | Polynomial / initial value mismatch | Ensure transmitter uses CRC-16/CCITT-FALSE (Poly `0x1021`, Init `0xFFFF`, no reflection) and excludes sync bytes `0xAA 0x55`. |
| Port Access Denied (Serial) | COM port opened by another process (e.g., terminal or transmitter) | Close conflicting serial terminals before clicking Connect. |
| UDP Datagrams not received | Firewall blocking inbound UDP port | Allow inbound UDP traffic on port `14550` in Windows Defender Firewall. |
| Exit code `0xc0000135` when running test | Qt runtime DLLs not found in PATH | Add Qt bin folder (e.g., `C:\Qt\6.8.3\msvc2022_64\bin`) to the system PATH. |
