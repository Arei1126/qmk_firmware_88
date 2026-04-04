# pc9801v2 matrix.c implementation notes

このファイルは、`keyboards/pc9801v2/matrix.c` を lite custom matrix 前提で書くときの整理メモです。

## 前提

- `CUSTOM_MATRIX = lite` を使う
- `matrix_init_custom()` と `matrix_scan_custom(matrix_row_t current_matrix[])` を実装する
- 物理マトリクスではなく、UART で来たコードを仮想マトリクスに反映する
- debounce や key event の最終処理は QMK 側に任せる

## custom matrix lite の基本形

lite では、`matrix_scan_custom()` が毎回 `current_matrix[]` を更新し、変化があったかどうかを `bool` で返す。

```c
void matrix_init_custom(void) {
    // UART や GPIO の初期化
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool changed = false;

    // UART を読み、current_matrix を更新
    // 変化があれば changed = true

    return changed;
}
```

## UART 初期化の考え方

`pc9801v2` では、UART 受信を別ファイルに分離しておくと保守しやすい。

- `pc9801_uart.c`
- `pc9801_uart.h`

こうしておくと、`matrix.c` 側は「受信したバイトをどう解釈するか」だけに集中できる。

### 使う設定

- `PC9801_UART_DRIVER` : 使う SIO ドライバ
- `PC9801_UART_TX_PIN` : TX ピン
- `PC9801_UART_RX_PIN` : RX ピン
- `PC9801_UART_BAUD` : 19200

RP2040 では `SIO` を使う。

## UART ピン設定

UART ピンは `palSetLineMode()` で UART 用の alternate mode にする。

```c
palSetLineMode(PC9801_UART_TX_PIN, PC9801_UART_TX_PAL_MODE);
palSetLineMode(PC9801_UART_RX_PIN, PC9801_UART_RX_PAL_MODE);
```

### ポイント

- GPIO の「普通の入出力」と UART の「代替機能」は別物
- UART を使うピンは、送受信前に alternate mode に切り替える
- 他の用途に使う GPIO は UART 初期化に触れない

## シリアル通信の設定

このキーボードでは 19200bps, 8O1 を想定する。

RP2040 の SIO 設定では、概念的には次のように考える。

- 8 data bits
- parity enabled
- odd parity
- 1 stop bit
- FIFO enabled

`SERIAL_USART_SPEED` ではなく、キーボード専用の `SIOConfig` を持つと、QMK 本体の他機能に影響しにくい。

## 受信コマンドの扱い

最初の実装では、UART で何かを受けたら特定キーを送る smoke test が有効。

### デバッグモード

- 受信した 1 バイトごとに、指定した 1 キーを短押しする
- UART が生きているか、配線が合っているかを最初に確認できる
- 実コードのキー解釈は後から差し替える

### 本実装の考え方

実データの仕様が確定したら、例えば次のように解釈する。

- bit7 = release
- lower 7 bits = キー index
- 特殊コード = all keys up など

例:

```c
if (code == PC9801_ALL_KEYS_UP_CODE) {
    clear matrix;
    changed = true;
} else if (code has release bit) {
    clear key bit;
} else {
    set key bit;
}
```

## マトリクスの更新方針

`current_matrix[row]` の該当ビットを立てる/落とす。

- `row` はキー index から計算
- `col` はキー index から計算
- `matrix_row_t` を使ってビットを操作する

### 例

```c
matrix_row_t old_row = current_matrix[row];
current_matrix[row] |= ((matrix_row_t)1 << col);
changed |= old_row != current_matrix[row];
```

## GPIO の設定方法（実装向け）

ここでは「どのファイルで、どう設定するか」を具体的にまとめる。

### 1. 設定を書く場所

- `config.h`
    - ピン番号、論理（active-low など）、デバッグモード定数を定義する
- `matrix.c`
    - GPIO の初期化、状態遷移、受信処理と連動した制御を書く
- `pc9801_uart.c`
    - UART に使うピンだけ alternate mode にする

### 2. QMK で使う GPIO API

- 出力化: `gpio_set_pin_output(pin)`
- 入力化: `gpio_set_pin_input(pin)`
- プルアップ入力化: `gpio_set_pin_input_high(pin)`
- High/Low 出力: `gpio_write_pin_high(pin)`, `gpio_write_pin_low(pin)`
- 入力読む: `gpio_read_pin(pin)`

### 3. 典型初期化（active-low の I_RDY）

```c
// I_RDY: active-low (LOW=ready)
gpio_set_pin_output(PC9801_I_RDY_PIN);
gpio_write_pin_high(PC9801_I_RDY_PIN); // 起動直後は非アサート
```

### 4. matrix_scan_custom() での使い方

```c
// ready を出す
gpio_write_pin_low(PC9801_I_RDY_PIN);

// UART 受信処理...

// 必要なら処理完了で解除
gpio_write_pin_high(PC9801_I_RDY_PIN);
```

### 5. GPIO 設計の注意

- 同じピンを「UART alternate」と「GPIO」の両方で使う場合は、モード切替の責務を 1 箇所に寄せる
- 受信中にピンモードを頻繁に切り替えると取りこぼしや framing error の原因になる
- active-low 信号は起動直後のデフォルトレベルを必ず決める

## UART TX と I_RDY の共用（active-low）

要件:

- 同じ物理ピンを
    - I_RDY として使うとき: GPIO
    - キーボードへの下りコマンド送信時: UART TX

### 結論（先に）

- **実装は可能だが、難易度は中〜高**
- 初期段階では、**TX を無効化して GP0 を GPIO 専用（I_RDY）にする方が現実的**
- 受信安定後に、必要なら TX 共用へ拡張するのが安全

### 難しい理由

1. ピンモード切替の競合
     - GPIO 出力(low/high) と UART alternate を都度切り替える必要がある
2. UART 状態管理が増える
     - 送信開始前に UART idle を保証し、切替後に `sioStart()` の再初期化や同期が必要になる可能性がある
3. active-low 制御の整合
     - I_RDY は LOW アサート、UART TX はアイドル HIGH のため、遷移タイミングを誤ると誤動作しやすい
4. デバッグ工数が増える
     - ロジアナで「モード切替」と「実データ」を同時検証する必要がある

### 難易度の目安

- RX のみ（今の構成）: 低
- TX を別ピンで追加: 低〜中
- TX と I_RDY を同一ピンで共用: 中〜高

### 初期フェーズ推奨（安全策）

1. UART は RX のみ有効
2. GP0 を I_RDY 専用 GPIO として使用
3. 下りコマンド機能は一旦保留

この場合、実装は次の方針になる。

- `pc9801_uart.c`
    - RX ピンのみ UART alternate に設定
    - TX は設定しない（または未使用）
- `matrix.c`
    - GP0 を GPIO として初期化し、I_RDY 制御に専念

### 将来 TX 共用をやる場合の最小手順

1. I_RDY を非アサート（HIGH）へ移行
2. 対象ピンを UART alternate へ切替
3. 下りコマンド送信
4. 送信完了待ち
5. ピンを GPIO 出力へ戻す
6. I_RDY 制御を再開

このステート遷移を関数 1 つに閉じ込めること。

## 実装方針（今回の推奨）

- まずは RX + I_RDY(GPIO) で受信系を完成させる
- 下りコマンドが本当に必要と確定した時点で TX 共用を別コミットで追加
- TX 共用は、受信処理と分離した専用層を作って実装する

## まず入れるべきデバッグ手段

優先順位は次の通り。

1. 受信したら 1 キーを短押し
2. デバッグ GPIO をトグル
3. 受信コードを `hid_listen` か debug 出力に出す

これで次が分かる。

- UART が本当に来ているか
- ボーレートが合っているか
- RX/TX が逆になっていないか
- parity / stop bit の設定が合っているか

## 実装の分離方針

保守しやすくするには、次の分離がよい。

- `pc9801_uart.c` : UART 初期化と 1 byte 受信
- `matrix.c` : 受信コードをマトリクスへ変換
- `config.h` : ピンと速度、デバッグモードの定義

この構成にしておくと、QMK 側の更新があっても修正箇所が局所化しやすい。
