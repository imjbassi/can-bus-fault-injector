/*
 * node.ino
 *
 * Heartbeat node for the CAN bus fault injector rig. Transmits a single-byte
 * frame on its own CAN ID once per second and prints a confirmation to serial.
 *
 * Flash this to each of the three Nanos, changing NODE_ID and NODE_NAME below
 * for each board:
 *
 *   Node 1 -> NODE_ID 0x036,  NODE_NAME "Node 1"
 *   Node 2 -> NODE_ID 0x037,  NODE_NAME "Node 2"
 *   Node 3 -> NODE_ID 0x038,  NODE_NAME "Node 3"
 *
 * For the duplicate-ID fault (fault 4), set two boards to the same NODE_ID
 * and give them different DATA_BYTE values, so the conflict is visible in
 * candump as alternating data on a single ID.
 *
 * The serial print matters more than it looks: it's the free diagnostic that
 * distinguishes "this node stopped transmitting" from "this node's frames
 * aren't reaching the bus." Those are different faults with different fixes.
 *
 * Library: autowp-mcp2515 (Arduino Library Manager, by autowp)
 * Wiring:  see docs/wiring.md
 */

#include <SPI.h>
#include <mcp2515.h>

// ---- per-board configuration -------------------------------------------
#define NODE_ID    0x036      // 0x036 / 0x037 / 0x038
#define NODE_NAME  "Node 1"   // "Node 1" / "Node 2" / "Node 3"
#define DATA_BYTE  0xAA       // vary this between boards for fault 4
// ------------------------------------------------------------------------

MCP2515 mcp2515(10);   // CS on D10
struct can_frame canMsg;

void setup() {
    Serial.begin(115200);
    mcp2515.reset();
    mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();   // real bus, not loopback

    canMsg.can_id  = NODE_ID;
    canMsg.can_dlc = 1;
    canMsg.data[0] = DATA_BYTE;
}

void loop() {
    // Only claim "sent" when the frame was actually queued. With nobody on the
    // bus to ACK, the MCP2515 retransmits forever and its TX buffers fill, so a
    // blind "sent" would hide exactly the fault this print exists to expose.
    MCP2515::ERROR err = mcp2515.sendMessage(&canMsg);
    if (err == MCP2515::ERROR_OK) {
        Serial.println(NODE_NAME ": sent");
    } else {
        Serial.print(NODE_NAME ": send failed, error ");
        Serial.println((int)err);
    }

    delay(1000);
}
