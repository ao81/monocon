# Bundled Arduino Mega 2560 toolchain

このディレクトリには、Monocon ToolsをWindows x64版VS Codeへインストールするだけでオフライン動作させるため、次の実行環境を同梱しています。

- Arduino AVR Boards 1.8.7のMega 2560用コアおよびvariant
- Arduino AVR Boards 1.8.7付属のEEPROM 2.0、HID 1.0、SoftwareSerial 1.0、SPI 1.0、Wire 1.0
- Arduino公式ライブラリのEthernet 2.0.2、LiquidCrystal 1.0.7、SD 1.3.0、Servo 1.2.2、Stepper 1.1.3、TFT 1.0.6
- Arduino配布のAVR GCC 7.3.0-atmel3.6.1-arduino7ツールチェーン
- 上記コアをArduino Mega 2560向けに事前ビルドした`core.a`

Arduinoコアと各ライブラリのソースファイルに記載されたライセンス、および各ツールチェーン構成物に同梱されたライセンスが適用されます。`core.a`は同梱ソースから生成した派生ビルド成果物です。
