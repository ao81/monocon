# Change Log

## 0.13.1 - 2024-09-17

### Changed

- Change signing to support VSCode extension signing.

## 0.12.0 - 2024-06-20

### Added

- Added the ability to use the Line Ending dropdown when in TCP mode. [#130](https://github.com/microsoft/vscode-serial-monitor/issues/130)
- Added a timestamp on the echoing of sent messages when both echoing and timestamps are enabled. [#135](https://github.com/microsoft/vscode-serial-monitor/issues/135)
- Added workaround for `set` issue on node serial-port specifically for Linux. [#159](https://github.com/microsoft/vscode-serial-monitor/issues/159) [#142](https://github.com/microsoft/vscode-serial-monitor/issues/142)

### Changed

- Enabled the ability to send as hex/binary for TCP mode. [#149](https://github.com/microsoft/vscode-serial-monitor/issues/149)

### Fixed

- Fixed a bug with hex view where bytes weren't handled correctly. [#139](https://github.com/microsoft/vscode-serial-monitor/issues/139)
- Fixed the icon when the Serial Monitor is moved to to a vertical panel. [#170](https://github.com/microsoft/vscode-serial-monitor/issues/170)
- Fixed issue where invoking `clearOutput` from tasks wasn't working. [#176](https://github.com/microsoft/vscode-serial-monitor/issues/176)

## 0.11.0 - 2023-12-05

### Added

- Enabled a `Paste` context menu entry when in terminal mode. [#95](https://github.com/microsoft/vscode-serial-monitor/issues/95)
- Enabled the `Clear Output` command in the command pallete. This will enable users to set their own key bindings to call this command. [#38](https://github.com/microsoft/vscode-serial-monitor/issues/38)
- Support for sending file contents to the Serial Monitor by right-clicking within a file and using the `Send File Contents to Serial Monitor` entry! [#7](https://github.com/microsoft/vscode-serial-monitor/issues/7)
- Hex view [#5](https://github.com/microsoft/vscode-serial-monitor/issues/5)
- Add command to reset all Serial Monitor windows to help usability and ensure windows are available. [#152](https://github.com/microsoft/vscode-serial-monitor/issues/152)

### Changed

- Modified the behavior of the `customBaudRates` setting to add to the baud rate list, rather than replace. [#93](https://github.com/microsoft/vscode-serial-monitor/issues/93)

### Fixed

- Fixed the `\r` issue in Terminal Mode. [#100](https://github.com/microsoft/vscode-serial-monitor/issues/100)
- Fixed an issue in the Terminal Mode where duplicate lines were being sent when "Enter" is pressed.
- Fixed the sporadic issue with logging to a file. [#110](https://github.com/microsoft/vscode-serial-monitor/issues/110)
- Fixed localization issues.
- Fixed an issue where monitoring failed to start when the log directory doesn't exist.

## 0.10.0 - 2023-06-07

### Added

- Error popup when there is invalid input in the message box.
- A terminal mode! [#82](https://github.com/microsoft/vscode-serial-monitor/issues/82)
- An API to interact with TCP connections [#97](https://github.com/microsoft/vscode-serial-monitor/issues/97)

### Fixed

- Fixed output issue for non-Latin characters [#88](https://github.com/microsoft/vscode-serial-monitor/issues/88)
- Fixed sending/printing of JSON object strings [#91](https://github.com/microsoft/vscode-serial-monitor/issues/91)
- Fixed an issue where modifying the custom baud rates was not being respected.

## 0.9.0 - 2023-03-20

### Added

- Toggle button that controls whether an echo of what was sent to the serial port is printed. [#79](https://github.com/microsoft/vscode-serial-monitor/issues/79)
- Checkboxes for setting DTR and RTS COM output lines in the serial monitor additional settings. Default values are both DTR and RTS being checked/true (both set active low). [#27](https://github.com/microsoft/vscode-serial-monitor/issues/27)

### Fixed

- Label colors in dark themes.
- Context menu copy not working. Also added a `Clear Output` button.

### Changed

- Automatically adds spaces in typed input for sending hex and binary encoded messages. [#61](https://github.com/microsoft/vscode-serial-monitor/issues/61)
- In VS Code Remote scenarios, we switched back to having the extension to prefer running remotely. To override this behavior, use the `remote.extensionKind` setting.

## 0.8.0 - 2023-02-15

### Added

- Changed our text view implementation to support ASNI color codes and terminal escape codes. [#46](https://github.com/microsoft/vscode-serial-monitor/issues/46)
- Added the ability to send empty messages.
- Added [an experimental API](https://www.npmjs.com/package/@microsoft/vscode-serial-monitor-api) for communicating with the Serial Monitor extension [#20](https://github.com/microsoft/vscode-serial-monitor/issues/20). This API should be considered unstable until it reaches version 1.0.0.
- Added a setting for modifying the font size of text output pane, `vscode-serial-monitor.outputFontSize` [#57](https://github.com/microsoft/vscode-serial-monitor/issues/57).
- A manual refresh button to manually refresh the port list [#62](https://github.com/microsoft/vscode-serial-monitor/issues/62).
- Added the ability to send common control signals (Ctrl+C, Ctrl+D, Ctrl+X, Ctrl+Z). [#22](https://github.com/microsoft/vscode-serial-monitor/issues/22)
- Added the ability to select message encoding (utf-8, hex, binary). Only multiples of 1 byte of data can be sent. [#61](https://github.com/microsoft/vscode-serial-monitor/issues/61)
- Added a setting for modifiying the timestamp format as well as the timestamp separator. [#75](https://github.com/microsoft/vscode-serial-monitor/issues/75).

### Changed

- Enabled autoscroll by default and added persistence for autoscroll and show timestamp settings. [#52](https://github.com/microsoft/vscode-serial-monitor/issues/52)
- In VS Code Remote scenarios, the extension now prefers running locally. This provides access to locally connected devices by default instead of devices connected to the remote environment. To override this behavior, use the `remote.extensionKind` setting. [#25](https://github.com/microsoft/vscode-serial-monitor/issues/25)
- Updated styling to better reflect VSCode's overall styling.

### Fixed

- Issue with scrolling in the text view.

## 0.7.0 - 2022-12-05

### Fixed

- Fixed some bugs.

## 0.6.0 - 2022-11-30

### Added

- Added TCP Support to the serial monitor [#15](https://github.com/microsoft/vscode-serial-monitor/issues/15)
- Added Serial Wire Output (SWO) support in TCP monitoring.
- Added auto-reconnection feature. When enabled, the serial monitor will block when a port is disconnected until the port is reconnected. [#33](https://github.com/microsoft/vscode-serial-monitor/issues/33)

### Changed

- How control characters are handled. They are now stripped from the input data so the text view looks cleaner.
- Changed styling of additional settings and additional settings button (gear icon) to make connection more obvious

### Fixed

- Fixed Open Additional Serial Monitor button's icon color issue
- Pulled in an updated dependency to fix port name localization issues. [#47](https://github.com/microsoft/vscode-serial-monitor/issues/47)

## 0.5.0 - 2022-10-18

### Fixed

- Fixed some bugs

## 0.4.0 - 2022-10-12

### Added

- Added milliseconds to the timestamp
- Added ability to open multiple serial monitors concurrently.

### Changed

- Switched from choosing log files to choosing log directories. Log files are now given a unique timestamp-based name when a monitor session is started.

## 0.3.0 - 2022-08-31

### Fixed

- Updated our node-usb dependency to fix device connection handling.
- Fixed accessibility bugs related to tabbing through UI components.

## 0.2.0 - 2022-07-28

### Added

- Additional settings to configure port monitoring: Data bits, Stop bits, Parity.
- Ability to log to a file, choose desired log file, and open the log file.
- Ability to navigate history of sent messages in text field.
- Support for settings.json configurations and custom baud rate.
- Add system message on sending a message to the port.

### Fixed

- Fixed the font in the output window, while ensuring the text view calculated positions correctly. 
- Restored tooltips for dropdowns/buttons/etc.
- Fixed bug where serial port connections didn't close when webview moved to new container.
- Edited the activation event to only activate when the Serial Monitor view is opened. 
- Color of additional settings icon when disabled.

## 0.1.0 - 2022-06-06

Initial Release.