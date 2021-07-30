# DirectX Game Engine
THM Giessen  
**_CS5328: Effiziente Algorithmen in der Computergrafik_**

Stephanie Käs  
Paul Konstantin Christof  
Vytautas Hermann  
Frank Kevin Zey

# TODOs
- [ ] Implement keys `4`, `5`, `6` according to table below.
- [ ] Rename all Headers to match `*.h` instead of `*.hpp`
- [ ] Test everything over again to ensure requirements stated as follows
```
- 50% Implementierung des Image Renderers incl. laden der Modelldatei. (✓)
- 20% Laden des Levels und der Modelle. Rendern der kompletten Szene. Navigation mit Maus und Tastatur. Anzeigen Splash - Screens beim Laden. Lauffähiges Spiel. README. Kommentierter Code.
- 10% Culling + Bounding Spheres
- 10% Broad Phase + AABBs
- 10% Narrow Phase + OBB
```

## What's not implemented but would have been nice
The `Collision Resolution` is implemented as stated in

**Aufgabe 11 Teil 3 Paragraph 3**:
```
Implementieren Sie eine Kollisionsauflösung. Statt auszugeben ob eine Kollision
vorliegt soll nun die Kamera bei Bedarf wieder Rückwärts bewegt werden damit der
Spieler das Gefühl hat nicht durch Wände laufen zu können.
```
Therefor a smooth running along the walls isn't given.

## Known *Bugs*
When colliding with the *barrier* object and looking down/up, the player can move
towards its center because the rotation of the camera is translated onto the OBB
Volume.  
When lokking down/up slows down movement speed.

## Controls
| Key      | Effect       |
|----------|--------------|
| `W`      | Move forward |
| `A`      | Move left    |
| `S`      | Move back    |
| `D`      | Move right   |
| `LShift` | Move Faster  |
| `LCtrl`  | Move Slower  |

| Key | Test Instance                          |
|-----|----------------------------------------|
|     | *Will be used for Frustum Culling*     |
|-----|----------------------------------------|
| `1` | `BoundingSphere`                       |
| `2` | `BoundingBox`                          |
| `3` | `BoundingOrientedBox`                  |
|-----|----------------------------------------|
|     | *Will be used for Collision Detection* |
|-----|----------------------------------------|
| `4` | `BoundingSphere`                       |
| `5` | `BoundingBox`                          |
| `6` | `BoundingOrientedBox`                  |
|-----|----------------------------------------|

## HowTo: Testing the BoundingVolume class
To change what bounding volume implementation the `BoundingVolume` class
uses you have to use the provided enum `BoundingVolumeTestType` like
this to use an OBB for example:
```c++
BoundingVolume::BVTT() = BoundingVolumeTestType::OBB;
```

The keys `1`, `2`, `3` changes only the behaviour for `Frustum Culling`.  
The keys `4`, `5`, `6` changes the behaviour for `Collision Detection`.

Since `Broad Phase` and `Narrow Phase` are implemented using `AABB` and `OBB` respectively,
testing can be performed with the function:
```c++
static bool BoundingVolume::SimpleCollisionCheck(const std::vector<BoundingVolume*>& models) noexcept;
```
The function needs a `vector` of `BoundingVolume` pointer to test against the camera. The concrete
`BoundingVolume` can be chosen by the keys `4`, `5`, `6`.

The `Camera` class needs to have a function similar like this:
```c++
static inline BoundingVolume& Body(void) noexcept { return m_Body; }
```
`m_Body` at this case represents the `BoundingVolume` emulating the players body.

## Integrating the `BoundingVolume` class
To integrate the `BoundingVolume` class the `BoundingVolume.h` and `BoundingVolume.cpp` have to be included to the
project. The `BoundingVolume`s `Update` function needs to be called inside the `Model::Update` function after updating
the model itself.  
`Broad Phase` and `Narrow Phase` can be called separatly by calling either
```c++
static std::vector<BoundingVolume*> broad(const std::vector<BoundingVolume*>& models) noexcept;
```
or
```c++
static bool narrow(const std::vector<BoundingVolume*>& models) noexcept;
```
Notice that those two function test the given `BoundingVolume`s against the `Camera`, since the `Camera` is the
only dynamic 'object' in our scene!  

The `Camera` class needs to have a function similar like this:
```c++
static inline BoundingVolume& Body(void) noexcept { return m_Body; }
```
`m_Body` at this case represents the `BoundingVolume` emulating the players body.

The function `SweepNPrune` defined like this
```c++
static inline bool SweepNPrune(const std::vector<BoundingVolume*>& models) noexcept { return narrow(broad(models)); }
```
tests every given `BoundingVolume` only against the camera for the reason explained above.

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

The above tests were made during the same running instance and from identical
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

During runtime the frustum culling test instances can be switch as shown on the table
above.


```
Windows 10 Version 10.0.19041.1110
Intel Core i5-6600K CPU @ 3.5GHz
16Gb RAM
NVIDIA GeForce GTX 970 4GB

VSync                off
Field of View:       75°
Near- / Far-Planes:  0.01 / 15.0
```

| **Test Type**         | **FPS** | **Time**            | **Rendered** |
|-----------------------|---------|---------------------|--------------|
| `BoundingBox` (AABB)  | 144    | **0.08 - 0.09ms**   | 85 of 122    |
| `BoundingSphere`      | 144    | **0.08 - 0.09ms**   | 67 of 122    |
| `BoundingOrientedBox` | 144    | **0.57ms - 0.58ms** | 45 of 122    |

This in another test on another computer and from a different angle.
The frames per seceond seem to be limeted by the monitors refrsh rate in this test.
Notice that in this test BoundingSphere culls more precice that BoundingBox.
This is most likely due to the chosen angle for this test, since BoundingBox
culls more precide from most other angles.

![alt text](TestAngle.png "Test angle 1")

From another angle the results are:

| **Test Type**         | **FPS** | **Time**            | **Rendered** |
|-----------------------|---------|---------------------|--------------|
| `BoundingBox` (AABB)  | 144     | **0.08 - 0.09ms**   | 91 of 122    |
| `BoundingSphere`      | 144     | **0.09 - 0.10ms**   | 111 of 122   |
| `BoundingOrientedBox` | 144     | **0.56ms - 0.58ms** | 33 of 122    |

In this test, the better clling precision from BoudningBox compared to BoudingSphere was even measurable
in the time it takes to cull.
It is also a good example on how much more precise culling can be performed with
BoundingOrientedBox. However the time it takes to do culling with
BoundingOrientedBox doesnt make it the best option for this application.

![alt text](TestAngle2.png "Test angle 1")