# 測資說明

```
testcases/
├── inputs/      輸入 layout（project-1 格式）
├── expected/    標準答案（同名 .out）
├── figures/     每個測資的圖：<name>_input.png、<name>_expected.png（紅色斜線框 = violation）
└── gen_random.py  產生大型隨機測資（沒有標準答案，測速度用）
```

執行方式：

```bash
make test                      # 編譯 + 跑全部測資並比對答案
./scripts/run_tests.sh tc06    # 只跑名稱含 tc06 的測資
```

你的輸出放在 `output/<name>.out`，程式印出的訊息在 `output/<name>.log`，答案不同時會有 `output/<name>.diff`。比對時會忽略違規的順序和數字格式（四捨五入到小數 3 位）。

## 規則參數（來自作業文件）

| 文件用語 | 值 (um) | `EolRule` 欄位 |
|---|---|---|
| EOL spacing | 1.0 | `eolSpace` |
| Length of end | 0.6 | `eolWidth` |
| Parallel length | 0.7 | `parWithin` |
| Parallel halo | 0.4 | `parSpace` |
| 圖中的 halo / eolWithin（文件沒給值，**假設**） | 0.4 | `eolWithin` |

## 標準答案採用的判定方式（重要）

作業文件有幾個地方沒有講清楚，標準答案是依照下面的解讀產生的。如果你在報告中採用不同的解讀，對應的測資答案要自己調整。

1. **只有同一層的圖形會互相影響。**
2. **同一層互相重疊或相接的矩形視為同一塊金屬**（先 merge 再找邊）。所以相接矩形的接縫不是 EOL；兩個 0.3 高的矩形疊成 0.6 寬的線，線端只有**一條** EOL 邊。
3. **EOL 邊**：merge 後圖形的邊，長度 **≤ 0.6**（文件寫 "smaller & equal"），且兩端都是凸角。
4. **平行條件（parallel edge）**：在 EOL 邊左右兩側，往外 0.4（parallel halo）、從 EOL 邊往回 0.7（parallel length）的區域內，**至少一側**有金屬時，才需要檢查 violation region。
   - 距離**剛好 0.4 也算**。範例中 #1 和 #2 的距離剛好是 0.4（1.0 − 0.6），#2/#4 這組違規能不能成立就看這一點。
   - 同一塊金屬的其他部分也算。範例中 #5 的 "object 3" 就是和它重疊的 #4。
5. **Violation region（T 字形，Fig. 2(a)）**：EOL 邊前方深 1.0，左右各比線端寬 0.4（halo / eolWithin，文件沒給值，假設 0.4）。
   - 任何金屬的**內部**跟這個區域重疊就違規。正前方的算，從線端旁邊伸進 T 兩側的也算。
   - **只碰到邊界不算**：前方距離剛好 1.0（文件寫 "at least 1"），或橫向剛好在 0.4 外，都合法。
6. **回報位置（marker）**：violation region 中，EOL 邊到違規金屬之間的部分。
   - 橫向裁切到 T 的範圍內。
   - 金屬在正前方：從 EOL 邊到金屬底部。
   - 金屬從旁邊伸進來：從 EOL 邊到金屬在區域內的頂端。
   - 例如 #5 上端 y=1.8（x 10.4~11.0）對 #6（y 從 2.2 開始）→ marker `(10.0, 1.8)-(11.4, 2.2)`。
   - 兩端互相違規會產生同一個 marker，writer 會自動去掉重複的。

## 輸出格式

```
number_of_violations 2
1  6.100 1.000,  7.000 1.600
1  10.000 1.800,  11.400 2.200
end_of_violation
```

每一行是 `layer  xl yl,  xh yh`（marker 的左下、右上角，單位 um），由 `src/report.cpp` 輸出。

## 每個測資在測什麼

| 測資 | 內容 | 答案數 |
|---|---|---|
| `tc00_sample` | 作業範例。#2 右端與 #4 左端間距 0.9，#1 在下方剛好 0.4，所以違規。#5 上端（寬 0.6）與 #6 間距 0.4，#4 在旁邊，所以違規。 | 2 |
| `tc01_basic` | 一條寬 0.6 的垂直線，上方 0.5 處有金屬，右側 0.3 處有鄰居。 | 1 |
| `tc02_parallel_cond` | 平行條件，由左到右五組：沒有鄰居；鄰居距離 0.5（超出 halo）；鄰居高度沒進入 parallel length；鄰居在右側範圍內；鄰居在左側範圍內。只有最後兩組違規。 | 2 |
| `tc03_space_width` | 間距剛好 1.0（合法）；間距 0.9（違規）；線寬 0.7（不是 EOL）；線寬 0.4、間距 0.2（違規）；上方金屬在斜前方，離線端側邊 0.6，超出 T 的 0.4（不違規）；上方金屬只擋到線端的一部分（違規，marker 裁切到 T 的範圍，x 53.3~54.0）。 | 3 |
| `tc04_directions` | 朝上、下、右、左的 EOL 各一個違規。 | 4 |
| `tc05_multilayer` | 兩層：鄰居在別層，不算；整組都在 M2，違規（layer 2）；上方金屬在別層，不算。 | 1 |
| `tc06_merge`（進階） | 三個相接/重疊矩形組成的長線，只有真正的線端違規，接縫不能報；兩個 0.5 疊成寬 1.0 的線，不是 EOL；兩個 0.3 疊成寬 0.6 的線，只能報一個 marker。 | 2 |
| `tc07_overlap`（進階） | 重複的矩形加上包在裡面的小矩形，只能報一次；大方塊裡包著一條細矩形，沒有 EOL；鉤子形狀，EOL 對著自己同一塊金屬的另一部分，要報。 | 2 |
| `tc08_float` | 浮點誤差陷阱：2.0−1.4、2.3−1.3、2.2−1.8 用 double 算都不精確。座標用整數 DBU（`layout.h` 已經這樣做）就不會出錯。 | 2 |
| `tc09_t_shape` | T 字形兩側，由左到右五組：旁邊的鄰居比線端高出 0.3，伸進 T 的側邊（違規）；上方金屬在斜前方，離側邊 0.2，在 0.4 內（違規）；離側邊 0.5，超出 0.4（不違規）；離側邊剛好 0.4，只碰到 T 的邊界（不違規）；旁邊鄰居頂端剛好跟線端齊平（不違規）。 | 2 |
| `tc90_random_10k` | 隨機 10000 個矩形，沒有標準答案，只看執行時間。 | — |

## 小工具

```bash
# 重畫 testcases/figures/ 的全部圖；跑過 make test 的話，也會把程式結果
# 連同檢查過的區域（EOL 邊、平行區域、T 字形 violation region）畫成 output/<name>.png
make figures                     # 或 ./scripts/make_figures.sh tc06 只畫一個

# 單獨畫一張（需要 matplotlib；副檔名可用 .png / .svg / .pdf）
./eol_drc testcases/inputs/tc06_merge.txt output/tc06_merge.out --regions output/tc06_merge.regions
python3 scripts/plot_layout.py testcases/inputs/tc06_merge.txt -r output/tc06_merge.out \
    -c output/tc06_merge.regions -o tc06.png

# 產生更大的隨機測資（矩形數、亂數種子、層數）
python3 testcases/gen_random.py 100000 7 1 > testcases/inputs/tc91_random_100k.txt
```

隨機測資沒有標準答案。想確認正確性的話，可以另外寫一個 O(n²) 的暴力版本，和你的快速版本互相對答案。
