# benchmark draw big mesh

## build

```bash
mkdir build
cd build
cmake ..
# open the solution file by visual studio
```

## benchmark

set benchmarkglfw as start project and run,  the title will show the FPS

## benchmark result

| Device             | Triangles | FPS | GPU Memory | GPU FLOPs   |
|--------------------|-----------|-----|------------|-------------|
| Desktop GTX 1660   | 100M      | 26  | 6 GB       | 5.4 TFLOPs  |
| Desktop GTX 1660   | 66M       | 39  | 6 GB       | 5.4 TFLOPs  |
| Desktop GTX 1660   | 47M       | 54  | 6 GB       | 5.4 TFLOPs  |
| Desktop GTX 1660   | 30M       | 60  | 6 GB       | 5.4 TFLOPs  |
| Laptop RTX 3080    | 100M      | 55  | 16 GB      | 29.77 TFLOPs|
| Laptop Intel       | 100M      | 11  | 8 GB       | 0.3 TFLOPs  |


## generate model file

uncomment code to save model file:

```cpp
saveToObj("./plane100m.obj", vertices, indices);
printf("save obj file done\n");
```
