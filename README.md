<!-- markdownlint-disable -->

## 起動方法

**必ず `start.bat` から起動してください** (Code.exe を直接起動すると vscode-neovim が
nvim を見つけられず、拡張機能が正常に動作しません)。

start.bat は起動時に以下を自動設定します:
- ポータブル nvim (`data/nvim/bin`) を PATH に追加
- Neovim 設定 (`data/nvim-config`) を XDG_CONFIG_HOME にセット
- Arduino ビルドツール (`data/daemon/build/bin`) を PATH に追加
- `default-path.txt` があればそのパスで Code.exe を開く

## パス登録の手順

Code.exeと同じ階層に `default-path.txt` を作成し、ここにmonokon/のパスを配置する

## git lfsのインストールコマンド
```
git lfs install
```

## vscodeブランチのみcloneするコマンド
```
git clone -b vscode https://github.com/ao81/monokon.git
```

<details>
<summary><h4>cmake build command</h4></summary>

- daemon/
- ```mkdir build & cd build```
- ```cmake .. -DCMAKE_BUILD_TYPE=Release```
- ```cmake --build . --config Release --parallel```

</details>
