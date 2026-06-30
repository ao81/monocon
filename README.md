<!-- markdownlint-disable -->

## 測距モジュール（GP2Y0E03）の接続方法
|センサー側|名称|アナログ専用での扱い|Arduinoの接続先
|---|---|---|---|
|1（赤）|VDD|必要|5V|
|2（白）|Vout(A)|必要|A0などのアナログ入力|
|3（黒）|GND|必要|GND|
|4（橙）|VIN(IO)|必要|3.3V|
|5（紫）|GPIO1|必要|3.3V|
|6（緑）|SCL|不要|未接続|
|7（黄）|SDA|不要|未接続|

<details>
<summary><h4>vim汎用コマンド</h4></summary>

- `"yp` レジスタpに選択位置をヤンク
- `"pp` レジスタpの内容を貼り付け

</details>

### [snippets](./snippets.md) (2026中国大会)
