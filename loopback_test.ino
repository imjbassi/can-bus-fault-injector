/*
 * loopback_test.ino
 *
 * Verifies one Arduino Nano + MCP2515 pair in isolation, with no CAN bus
 * wiring at all. The MCP2515 is put into loopback mode, where it receives
 * its own transmitted frames internally.
 *
 * Run this on every board BEFORE connecting anything together. It separates
 * SPI wiring problems from bus problems — if this passes, the Nano, the
 * MCP2515, and the six SPI wires between them are all confirmed good, and
 * any later failure is a bus-level issue.
 *
 * Expected output on the serial monitor (115200 baud):
 *
 *   Loopback OK - message received
 *   Loopback OK - message received
 *   ...
 *
 * If nothing prints, check SO/SI first — they're easy to swap, and swapping
 * them fails silently rather than throwing an error.
 *
 * Library: autowp-mcp2515 (Arduino Library Manager, by autowp)
 *
 * Wiring:
 *   MCP2515 VCC -> Nano 5V
 *   MCP2515 GND -> Nano GND
 *   MCP2515 CS  -> Nano D10
 *   MCP2515 SO  -> Nano D12
 *   MCP2515 SI  -> Nano D11
 *   MCP2515 SCK -> Nano D13
 */

#include <SPI.h>
#include <mcp2515.h>

MCP2515 mcp2515(10);   // CS on D10
struct can_frame canMsg;

void setup() {
    Serial.begin(115200);
    mcp2515.reset();
    mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ);
    mcp2515.setLoopbackMode();   // no external bus required
}

void loop() {
    canMsg.can_id  = 0x036;
    canMsg.can_dlc = 1;
    canMsg.data[0] = 0xAA;
    mcp2515.sendMessage(&canMsg);

    struct can_frame received;
    if (mcp2515.readMessage(&received) == MCP2515::ERROR_OK) {
        Serial.println("Loopback OK - message received");
    }

    delay(1000);
}
