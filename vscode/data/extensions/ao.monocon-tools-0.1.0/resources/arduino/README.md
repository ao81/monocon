# Bundled Arduino Mega 2560 toolchain

このディレクトリには、Monocon ToolsをWindows x64版VS Codeへインストールするだけでオフライン動作させるため、次の実行環境を同梱しています。

- Arduino AVR Boards 1.8.7のMega 2560用コアおよびvariant
- Arduino配布のAVR GCC 7.3.0-atmel3.6.1-arduino7ツールチェーン
- 上記コアをArduino Mega 2560向けに事前ビルドした`core.a`

Arduinoコアの各ソースファイルに記載されたライセンス、および各ツールチェーン構成物に同梱されたライセンスが適用されます。`core.a`は同梱ソースから生成した派生ビルド成果物です。
