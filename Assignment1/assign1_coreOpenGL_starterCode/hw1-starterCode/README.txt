The program features:
1. Press '1' for dots mode.
2. Press '2' for girds mode(line rendering).
3. Press '3' for triangle rendering mode.
4. Press '4' to toggle on/off smoothened vertices.
5. Drag with left mouse button to rotate around x and y axis. 
    Drag with middle mouse button to rotate around z-axis.
6. Drag with left mouse button and shift pressed, to scale.
7. Press 't' to toggle on/off translation movement.
    Then drag with left mouse button to move on x-y plane.
    Drag with middle mouse button to move along z-aixs.
8. Press 'x' to generate a 'screenshot.jpg'.

The special point I'd mention of my program is that I choose not to stream 4
    VBO arrays to my shader for smoothening. Instead, I calculate the smoothened
    positions and colors on CPU. That is because vertices arrays for generating 
    grids and triangles are carefully manipulated to a designated order and both of
    them intimately depend on the original order and value of vertice posisitions and
    colors. Instead of the repeated calculation on GPU taking up 4 times of memory space, 
    I just have to calculate the smoothened value once for each point on CPU. And on GPU
    side, the only task it has to do is to choose between original values or smoothened
    values. 