# Shortcut Manager (C++/Qt)

A C++/Qt implementation of a GUI tool to manage executable shortcuts for the command line in `/usr/local/bin`. This is a port of the Python version with the same functionality but with improved performance and native look and feel.

#Install / Usage

Copy the executable "shorts" from ~/ into your system path folder /usr/sbin/ then simply type "sudo shorts" in the command line (executable will not launch without sudo). Additionaly, copy shorts.destop to your desktop as the desktop launcher.


## Features

- View all executable shortcuts in `/usr/local/bin`
- Create new shortcuts with custom commands
- Edit existing shortcuts
- Delete shortcuts
- Toggle command options:
  - Run with sudo
  - Run in background
  - Open ended (supports arguments with `$@`)
- Dark theme with modern UI

## Building from Source

### Prerequisites

- CMake (3.16 or higher)
- Qt 5.15 or Qt 6.0 or higher (with development packages)
- C++17 compatible compiler
- Make or Ninja

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the application
make -j$(nproc)

# Install (optional, requires root)
sudo make install
```

## Usage

Run the application:

```bash
./shortcut_manager
```

### Command Line Options

- `--help` - Show help message
- `--version` - Show version information

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
