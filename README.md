# Synesthesia


## Performance

To count the number of assembly instructions in the compiled object file, you can use the following command:
```bash
objdump -d file.o | grep -E "^[[:space:]]+[0-9a-f]+:" | wc -l
```