# DirectX Game Engine
THM Giessen  
**_CS5328: Effiziente Algorithmen in der Computergrafik_**

Stephanie Käs  
Paul Konstantin Christof  
Vytautas Hermann  
Frank Kevin Zey

## Frustum Culling
```
Windows 10 Version 10.0.19043 Build 19043
AMD Ryzen 5 2600X Six-Core Processor, 3600 Mhz
16Gb RAM
NVIDIA GeForce GTX 1660 Ti 6Gb

VSync                off
Field of View:       75°
Near- / Far-Planes:  0.01 / 15.0
```

| **Test Type**         | **FPS** | **Time**   | **Rendered** |
|-----------------------|---------|------------|--------------|
| `BoundingBox` (AABB)  | ~356    | **0.10ms** | 56 of 122    |
| `BoundingSphere`      | ~329    | **0.09ms** | 98 of 122    |
| `BoundingOrientedBox` | ~282    | **0.77ms** | 56 of 122    |

The above Tests were made during the same running instance and from identical
position and orientation. Even the `BoundingSphere` has the best resolution time
its culling is rough at best. The `BoundingBox` has comparable performance to the
`BoundingSphere` but culls much more precise. By no suprise was the culling precision
of the `BoundingOrientedBox`. Due to the nature of the scene and the alignment of its
objects in the world there was no real difference. But the needed time to calculate
the collisions goes by factor of 7-8 times than `BoundingSphere` and `BoundingBox`.

Since `BoundingBox` and `BoundingSphere` have similar resolution times and the
resolution of both is $\mathcal{O}(n)$ with nearly identical $\mathcal{C}$, using
`BoundingBox` as a default is desirable.  
The number of possible visible objects with `BoundingBox` is smaller, therefor a
higher framerate was observed and therefor `BoundingBox` is set to be the engines
default frustum culling test instance.

During runtime the frustum culling test instances can be switch by pressing
| Key | Test Instance         |
|-----|-----------------------|
| `1` | `BoundingSphere`      |
| `2` | `BoundingBox`         |
| `3` | `BoundingOrientedBox` |

## Broad Phase Collision Detection


## Narrow Phase Collision Detection

