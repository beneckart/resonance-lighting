#pragma once

#include <stdint.h>
#include <string.h>

// One received ESP-NOW frame, copied out of the WiFi-task callback.
// 250 B covers every fleet frame (packet.h static_asserts the heartbeat fits).
struct RxItem {
  uint32_t ms;      // receive timestamp (millis at callback)
  uint8_t mac[6];   // radio source address
  int8_t rssi;      // per-packet RSSI from rx_ctrl
  uint8_t len;
  uint8_t data[250];
};

// Single-producer (radio callback) / single-consumer (census drain) ring.
// Power-of-two capacity; storage injected so the device puts it in PSRAM and
// tests put it on the heap. Lock-free: only the producer writes mHead, only
// the consumer writes mTail.
class RxRing {
 public:
  void init(RxItem *storage, uint32_t capPow2) {
    mBuf = storage;
    mMask = capPow2 - 1;
    mHead = mTail = mDrops = 0;
  }
  bool push(const RxItem &item) {  // producer context
    uint32_t head = mHead;
    if (head - mTail > mMask) {  // full
      ++mDrops;
      return false;
    }
    mBuf[head & mMask] = item;
    mHead = head + 1;
    return true;
  }
  bool pop(RxItem *out) {  // consumer context
    if (mTail == mHead) return false;
    *out = mBuf[mTail & mMask];
    ++mTail;
    return true;
  }
  uint32_t drops() const { return mDrops; }
  uint32_t pending() const { return mHead - mTail; }

 private:
  RxItem *mBuf = nullptr;
  uint32_t mMask = 0;
  volatile uint32_t mHead = 0;
  volatile uint32_t mTail = 0;
  volatile uint32_t mDrops = 0;
};
