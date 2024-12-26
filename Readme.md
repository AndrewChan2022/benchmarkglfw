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

| Device             | Triangles     | FPS  |
|--------------------|---------------|------|
| Desktop GTX 1660   | 100M          | 26   |
| Desktop GTX 1660   | 66M           | 39   |
| Desktop GTX 1660   | 47M           | 54   |
| Desktop GTX 1660   | 30M           | 60   |
| Laptop RTX 3080    | 100M          | 55   |
| Laptop Intel       | 100M          | 11   |


## generate model file

uncomment code to save model file:

```cpp
saveToObj("./plane100m.obj", vertices, indices);
printf("save obj file done\n");
```
