# Synesthesia


## Requirements

### Audio
This project uses miniaudio, which is included in the `external` folder.

### Visual

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
DRI_PRIME=1  ./opengl_demo
```


## Performance

To count the number of assembly instructions in the compiled object file, you can use the following command:
```bash
objdump -d file.o | grep -E "^[[:space:]]+[0-9a-f]+:" | wc -l
```