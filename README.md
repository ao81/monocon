<!-- markdownlint-disable -->

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
