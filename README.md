<!-- markdownlint-disable -->

# [snippets](./snippets.md)


<details>
<summary><h4><s>arduino-cliのセットアップ手順</s></h4></summary>

- .arduino-cliディレクトリをコピーし、環境変数に登録する。
- ```arduino-cli config init```
- ```arduino-cli core update-index```
- ```arduino-cli core install arduino:avr```
- ```arduino-cli board list```

</details>

<details>
<summary><h4>vim</h4></summary>

- `"yp` レジスタpに選択位置をヤンク
- `"pp` レジスタpの内容を貼り付け

</details>

<details>
<summary><h4>cmake build command</h4></summary>

- daemon/
- ```mkdir build & cd build```
- ```cmake .. -DCMAKE_BUILD_TYPE=Release```
- ```cmake --build . --config Release --parallel```

</details>

<details>
<summary><h4>free claude code command</h4></summary>

- ```python -m uv run uvicorn server:app --host 0.0.0.0 --port 8082```
- ```$env:ANTHROPIC_AUTH_TOKEN="freecc"; $env:ANTHROPIC_BASE_URL="http://localhost:8082"; claude```

</details>

[mono_con.h](./mono_con.h)
