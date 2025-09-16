# Synesthesia
Project aiming to visualize sounds.

This program run two threads and one process:
- one thread to decode an mp3 file and store the decoded bytes.
- one process to play the decoded bytes.
- one thread creating the visual.

## Requirements

### Audio
Audio is done using miniaudio, which is included in the `external` folder.

### Visual
Visual is done using OpenGL version 4.6 with GLFW and GLAD.

```bash
sudo apt update
sudo apt install -y build-essential cmake libglfw3-dev mesa-common-dev
```

#### GLAD
Glad has been included in the `external` folder. You can regenerate it using the [GLAD web service](https://glad.dav1d.de/).

## Build and Run
```bash
mkdir build
cd build
cmake ..
make -j10
DRI_PRIME=1  ./src/synesthesia piano_2.mp3
```


## Performance

To count the number of assembly instructions in the compiled object file, you can use the following command:
```bash
objdump -d file.o | grep -E "^[[:space:]]+[0-9a-f]+:" | wc -l
```