# pc9801v2 Implementation Guide

このドキュメントは、`keyboards/pc9801v2` の UART + GPIO + custom matrix(lite) 実装を、今後の保守/拡張のために整理したものです。

## 1. 全体アーキテクチャ

- MCU: RP2040
- Matrix 方式: `CUSTOM_MATRIX = lite`
- 入力ソース: PC-98 キーボードからの UART 受信データ
- 変換方式: `pc98_codes.h` の scan code -> 仮想 matrix 座標マップ
- GPIO 制御:
  - `I_RST` (active-low reset): 通常時は常に HIGH
  - `I_RDY` (active-low ready): 通常 LOW、受信時に ACK パルスとして HIGH を 40us 出す
- TX 側: 現在は無効 (`PC9801_UART_ENABLE_TX = 0`)

## 2. シリアル実装のしかた

### 2.1 実装ファイル

- `pc9801_uart.c`
- `pc9801_uart.h`

### 2.2 UART 設定

`pc9801_uart.c` 内の `SIOConfig` を使って RP2040 の UART を設定しています。

- baud: `19200`
- frame: `8O1`
  - 8 data bits
  - parity enabled
  - odd parity (`EPS=0`)
  - 1 stop bit (`STP2=0`)

### 2.3 初期化手順

`pc9801_uart_init()` で:

1. RX ピンを UART alternate mode に設定
2. `sioStart()` で UART 開始

補足:

- TX は `PC9801_UART_ENABLE_TX` が 0 のため設定しない
- UART 受信は `pc9801_uart_read()` で `TIME_IMMEDIATE` 非ブロッキング読み取り

## 3. matrix.c の書き方（lite）

### 3.1 エントリポイント

- `matrix_init_custom()`
  - GPIO 初期状態を確定
  - UART 初期化

- `matrix_scan_custom(matrix_row_t current_matrix[])`
  - 受信バイトを処理して `current_matrix` を更新
  - 変化があれば `true` を返す

### 3.2 受信データ処理

`matrix_scan_custom()` の処理順:

1. `I_RST` を HIGH に維持
2. `I_RDY` を LOW に維持
3. UART 受信ループ
4. 1バイト受信ごとに ACK パルス
   - `I_RDY` HIGH
   - `wait_us(40)`
   - `I_RDY` LOW
5. コマンドバイト `0xFA / 0xFB / 0xFC` は無視
6. `0x7F` は all-keys-up 扱い
7. signed 解釈で make/break 判定
8. `pc98_to_matrix` で座標へ変換
9. 通常キーは delayed release で平滑化
10. Caps/Kana は tap 化（後述）

### 3.3 delayed release（リピート平滑化）

目的:

- キーボード内蔵リピートとホスト側リピートの競合を減らす

方法:

- break を受けても即 OFF せず、`pending_release_ms[row][col]` に予約
- `PC9801_RELEASE_DELAY_MS` 経過後に OFF
- 予約中に make が来たら予約をキャンセル

## 4. Caps/Kana の特殊仕様

対象:

- Caps: `[3,1]`
- Kana: `[5,0]`

背景:

- メカニカルロックで make/break の挙動が通常キーと異なる
- 現代 PC 側では「トグル用ワンショット入力」の方が扱いやすい

実装:

- make/break どちらのイベントでも一旦 ON
- 短時間後 (`PC9801_LOCK_TAP_MS`) に OFF
- これにより tap として送出

## 5. GPIO の書き方

### 5.1 使用 API

- `gpio_set_pin_output(pin)`
- `gpio_write_pin_high(pin)`
- `gpio_write_pin_low(pin)`

### 5.2 現在のポリシー

- `I_RST`:
  - active-low reset
  - 低くするとリセットがかかるため、通常運用では常に HIGH
- `I_RDY`:
  - active-low ready
  - 待機時 LOW
  - ACK 時のみ HIGH 40us

### 5.3 注意

- `I_RST` と `I_RDY` は常に明示的に駆動し、浮かせない
- `I_RDY` パルス幅は仕様依存。変更時はロジアナで確認する

## 6. キーボード仕様（実装上の前提）

- UART 受信は 19200/8O1
- 受信データは PC-98 scan code ベース
- signed byte の負値は break 系として扱う
- 一部コマンドバイト (`FA/FB/FC`) が混在する
- Caps/Kana はメカロック挙動をワンショット化して扱う

## 7. 必要なファイル

この実装で最低限関係するファイル:

- `rules.mk`
  - `CUSTOM_MATRIX = lite`
  - `UART_DRIVER_REQUIRED = yes`
  - `SRC += matrix.c`
  - `SRC += pc9801_uart.c`

- `config.h`
  - UART ピン/baud
  - TX 有効/無効
  - `I_RST`, `I_RDY` ピン
  - debug モード切替

- `matrix.c`
  - protocol decode
  - matrix 反映
  - `I_RDY` ACK タイミング制御
  - Caps/Kana 特殊処理
  - delayed release

- `pc9801_uart.c`, `pc9801_uart.h`
  - UART 初期化と受信 I/F

- `pc98_codes.h`
  - scan code -> matrix 座標マップ

- `mcuconf.h`
  - RP2040 の SIO(UART) 有効化

## 8. 設定が必要な箇所（変更ポイント）

通常の移植や派生で変更しやすい箇所:

- UART 設定
  - `PC9801_UART_DRIVER`
  - `PC9801_UART_RX_PIN`
  - `PC9801_UART_TX_PIN`
  - `PC9801_UART_BAUD`
  - `PC9801_UART_ENABLE_TX`

- GPIO 設定
  - `PC9801_I_RST_PIN`
  - `PC9801_I_RDY_PIN`
  - `PC9801_I_RDY_ACK_US`

- 入力解釈
  - `PC9801_CMD_FA/FB/FC`
  - `PC9801_ALL_KEYS_UP_CODE`
  - `pc98_to_matrix[]` (in `pc98_codes.h`)

- 入力平滑化
  - `PC9801_RELEASE_DELAY_MS`
  - `PC9801_LOCK_TAP_MS`

- デバッグ
  - `PC9801_UART_DEBUG_TEST_MODE`
  - `PC9801_UART_DEBUG_ROW/COL`

## 9. 拡張時の推奨手順

1. まず `pc98_codes.h` のマッピングを確定
2. 次に `I_RDY` パルス幅 (`PC9801_I_RDY_ACK_US`) を実機で調整
3. その後 `PC9801_RELEASE_DELAY_MS` を調整してリピート競合を最小化
4. 最後に必要なら TX を有効化 (`PC9801_UART_ENABLE_TX = 1`) し、下りコマンド層を別関数で追加

## 10. 検証チェックリスト

- ビルド
  - `qmk compile -kb pc9801v2 -km default`
  - `qmk compile -kb pc9801v2 -km via`

- 実機
  - `I_RST` が常時 HIGH であること
  - `I_RDY` が受信時にのみ HIGH パルスになること
  - 通常キーの make/break が正しく反映されること
  - Caps/Kana がトグルとして使えること
  - 長押し時に過剰リピートが起きないこと
