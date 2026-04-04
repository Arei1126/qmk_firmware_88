/**
 * KLEのJSONデータからQMKのlayout配列を生成する処理
 * 拡張性を持たせ、x, y, w, h などの修飾子に動的に対応可能とする
 */
function parseKleToQmk(kleArray) {
        const layout = [];
        let currentX = 0;
        let currentY = 0;

        // 最初の要素（メタデータのオブジェクト）は除外し、行データのみを抽出する
        const rows = kleArray.filter(item => Array.isArray(item));

        rows.forEach(row => {
                currentX = 0;
                let nextW = 1;
                let nextH = 1;
                let nextOffsetX = 0;
                let nextOffsetY = 0;

                row.forEach(item => {
                        if (typeof item === 'object') {
                                // プロパティオブジェクトの処理: 次のキーに適用される状態を更新する
                                if (item.x !== undefined) nextOffsetX = item.x;
                                if (item.y !== undefined) nextOffsetY = item.y;
                                if (item.w !== undefined) nextW = item.w;
                                if (item.h !== undefined) nextH = item.h;
                        } else if (typeof item === 'string') {
                                // 文字列（キー）の処理: 実際のキーデータを生成する
                                currentX += nextOffsetX;
                                currentY += nextOffsetY;

                                // ラベル "行,列" を分割してマトリクスの数値配列に変換する
                                const matrix = item.split(',').map(Number);

                                const keyObj = {
                                        matrix: matrix,
                                        x: currentX,
                                        y: currentY
                                };

                                // デフォルト値(1)以外の幅・高さを持つ場合のみプロパティを追加する
                                if (nextW !== 1) keyObj.w = nextW;
                                if (nextH !== 1) keyObj.h = nextH;

                                layout.push(keyObj);

                                // 現在のX座標をキー幅分進め、修飾子を初期値にリセットする
                                currentX += nextW;
                                nextW = 1;
                                nextH = 1;
                                nextOffsetX = 0;
                                nextOffsetY = 0;
                        }
                });
                // 行が終了するごとにY座標を1進める
                currentY++;
        });

        return layout;
}

// 実行例:
//
const data = require("./pc-9801-kle.json");
//
const qmkLayout = parseKleToQmk(data);
console.log(JSON.stringify(qmkLayout, null, 8));
