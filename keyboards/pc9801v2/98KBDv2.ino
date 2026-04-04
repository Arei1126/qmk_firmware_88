#include <Adafruit_TinyUSB.h>
#include "98_key_map.h"

uint8_t const desc_hid_report[] = {
        TUD_HID_REPORT_DESC_KEYBOARD()
};
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_KEYBOARD, 2, false);

#define TX_PIN          0
#define RX_PIN          1
#define I_RDY_PIN       4

uint8_t modifier_state = 0;
uint8_t key_state[6] = {0};

/*
 * リリース(離上)を保留している時刻を記録する配列。
 * 25cps (40ms周期) のBreakを吸い取るため、30msの保留時間を設定。
 */
uint32_t pending_release_time[6] = {0};
const uint32_t RELEASE_DELAY_MS = 4; 

// ==========================================
// 状態管理関数群
// ==========================================

bool pressKey(uint8_t usage_id) {
        if (usage_id >= 224 && usage_id <= 231) {
                uint8_t mask = 1 << (usage_id - 224);
                if ((modifier_state & mask) == 0) {
                        modifier_state |= mask;
                        return true; 
                }
                return false; 
        } else {
                for (int i = 0; i < 6; i++) {
                        if (key_state[i] == usage_id) {
                                // すでに配列にある場合、もしリリース保留中だったらキャンセル！
                                if (pending_release_time[i] != 0) {
                                        pending_release_time[i] = 0; 
                                }
                                return false; // リピート信号なのでUSB送信は不要
                        }
                }
                for (int i = 0; i < 6; i++) {
                        if (key_state[i] == 0) {
                                key_state[i] = usage_id;
                                pending_release_time[i] = 0;
                                return true; 
                        }
                }
                return false; 
        }
}

void markRelease(uint8_t usage_id) {
        if (usage_id >= 224 && usage_id <= 231) {
                modifier_state &= ~(1 << (usage_id - 224));
        } else {
                for (int i = 0; i < 6; i++) {
                        if (key_state[i] == usage_id) {
                                pending_release_time[i] = millis(); 
                                if (pending_release_time[i] == 0) pending_release_time[i] = 1;
                                break;
                        }
                }
        }
}

void forceRelease(uint8_t usage_id) {
        for (int i = 0; i < 6; i++) {
                if (key_state[i] == usage_id) {
                        key_state[i] = 0;
                        pending_release_time[i] = 0;
                }
        }
}

void my_set_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {}

// ==========================================
// Setup & Loop
// ==========================================

void setup() {
        usb_hid.setReportCallback(NULL, my_set_report_cb);
        usb_hid.begin();
  
        pinMode(I_RDY_PIN, OUTPUT);
        digitalWrite(I_RDY_PIN, LOW); // アイドル時の基本状態
        pinMode(RX_PIN, INPUT);
        //pinMode(TX_PIN, OUTPUT);

        uint32_t t = millis();
        while (!TinyUSBDevice.mounted() && (millis() - t) < 3000) { delay(1); }

        // ハードウェアリセット
        digitalWrite(I_RDY_PIN, HIGH);
        digitalWrite(TX_PIN, LOW);
        delayMicroseconds(15);
        digitalWrite(TX_PIN, HIGH);
        delay(200);
        digitalWrite(I_RDY_PIN, LOW); // リセット後もLOW

        Serial1.setRX(RX_PIN);
        Serial1.setTX(TX_PIN);
        Serial1.begin(19200, SERIAL_8O1);
}

void loop() {
        // キーボードに対して常に受信準備完了(LOW)を通知
        digitalWrite(I_RDY_PIN, LOW);

        if (Serial1.available() > 0) {
                
                unsigned char raw_data = Serial1.read();
                
                // データを受信した瞬間にACKパルス（HIGH）を40us送信
                digitalWrite(I_RDY_PIN, HIGH);
                delayMicroseconds(40); 
                
                if (raw_data != 0xFA && raw_data != 0xFC && raw_data != 0xFB) {
                        signed char data = (signed char)raw_data;
                        
                        if (data < 0) {    
                                // Break (離上): 補正してフィルタリングタイマー開始
                                data += 128;
                                markRelease(PC98_to_UsageID[data]);
                        } else {  
                                // Make (押下): 新規ならUSB送信
                                if (pressKey(PC98_to_UsageID[data])) {
                                        uint32_t t = millis();
                                        while (!usb_hid.ready() && (millis() - t) < 10) { delay(1); }
                                        if (usb_hid.ready()) {
                                                usb_hid.keyboardReport(0, modifier_state, key_state);
                                        }
                                }
                        }

                        // ==========================================
                        // 伝説の「両対応」トグルロジック
                        // ==========================================
                        // カナ(114)とCaps(113)は、押し込み(Make)と跳ね返り(Break)の
                        // 両方のタイミングでOSへワンショットのトグル信号を送る。
                        if (data == 114 || data == 113) { 
                                uint8_t target_key = (data == 114) ? 53 : 57;
                                
                                // 押してすぐ離す
                                pressKey(target_key);
                                uint32_t t = millis();
                                while (!usb_hid.ready() && (millis() - t) < 10) { delay(1); }
                                if (usb_hid.ready()) { usb_hid.keyboardReport(0, modifier_state, key_state); }
                                
                                forceRelease(target_key);
                                t = millis();
                                while (!usb_hid.ready() && (millis() - t) < 10) { delay(1); }
                                if (usb_hid.ready()) { usb_hid.keyboardReport(0, modifier_state, key_state); }
                        }
                }
                // 処理完了。確実にLOWに戻す
                digitalWrite(I_RDY_PIN, LOW);
                delayMicroseconds(1);
        } 
        else {
                digitalWrite(I_RDY_PIN, LOW); // アイドル時もLOWを維持
        }

        // ==========================================
        // 遅延リリース(デバウンス)の監視
        // ==========================================
        bool needs_release_report = false;
        uint32_t now = millis();
        
        static uint8_t prev_modifier_state = 0;
        if (modifier_state != prev_modifier_state) {
                needs_release_report = true;
                prev_modifier_state = modifier_state;
        }

        for (int i = 0; i < 6; i++) {
                if (pending_release_time[i] != 0 && (now - pending_release_time[i] >= RELEASE_DELAY_MS)) {
                        key_state[i] = 0;
                        pending_release_time[i] = 0;
                        needs_release_report = true;
                }
        }
        
        if (needs_release_report && usb_hid.ready()) {
                usb_hid.keyboardReport(0, modifier_state, key_state);
        }
}