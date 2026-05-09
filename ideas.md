

### Path 2: Spatial Partitioning (Grid-based Sensing)

Right now, particles are "sentient" about the mouse, but they are **blind to each other** (except for the Predators). If you want them to flock like birds (Boids) or flow like water, they need to "see" their neighbors.

* **The Challenge:** Checking 500,000 particles against each other is $O(N^2)$, which will crash your frame rate.
* **The Task:**
1. Implement a **Spatial Hash Grid**.
2. Create a compute kernel that assigns each particle to a "cell" index based on its position.
3. Sort the particles by cell index (using a GPU bitonic sort).
4. Update the `prepareFeatures` shader so particles can look at the 8 cells surrounding them to input "Local Density" or "Average Neighbor Velocity" into the neural network.



---

### Path 3: Visual Polish (Instanced Rendering & Trails)

Point sprites are fast, but they look like "dots." You can make the sandbox look like a high-end production by adding movement trails.

* **The Task:**
1. **Motion Blur/Trails:** Don't clear the screen fully every frame. Use a persistent texture. Draw the particles, then on the next frame, draw a semi-transparent black quad over the whole screen before drawing the particles again. This creates "ghosting" trails.
2. **Instancing:** Instead of `MTLPrimitiveTypePoint`, use instanced rendering to draw small triangles or meshes that "stretch" based on the particle's velocity vector.


---

