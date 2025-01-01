# ClipBoardNotification

ClipBoardNotification is a lightweight program designed to monitor clipboard activity and immediately display a preview of the copied content in a non-intrusive popup in the bottom-right corner of your screen. The program was created to address a common issue in Windows 11, where pressing `Ctrl+C` fails to copy content. With this tool, you’ll get instant feedback to notice any clipboard failure before navigating to the destination page and discovering that nothing is pasted.

Unlike many existing clipboard managers, which are often large and feature-heavy, ClipBoardNotification focuses on simplicity and low interference. It provides a minimalistic reminder without forcing system notifications or sounds, making it quick to use without distractions.

## Features

- Display an icon in the system tray.
- Detect and display **text** copied to the clipboard.
- Detect and display **images** copied to the clipboard.
- Detect and display **file paths** copied to the clipboard.
- Automatically build and release using GitHub Actions.

## Why this Project?

Many clipboard management tools offer excessive features that can be overwhelming, and some even force message notification sounds or popups. ClipBoardNotification aims to provide a straightforward, lightweight alternative with just the essential functionality: notifying you when the clipboard activity fails to copy properly, without disrupting your workflow.

## Installation

1. Download the latest release from the [GitHub Releases page](https://github.com/Wu-Yijun/ClipBoardNotification/releases/latest).
2. Extract the file and run the executable. It will start automatically and appear in your system tray.
3. You can place the executable in any folder you prefer. To have it start automatically with Windows, you can manually set it up to launch at startup.
4. After running the program, simply copy content (text, image, or file path), and a preview notification will appear in the bottom-right corner.

## Contributing

Feel free to open an issue or submit a pull request if you have suggestions, improvements, or bug fixes.
