Subject 	: CSCI420 - Computer Graphics 
Assignment 2: Simulating a Roller Coaster
Author		: < Haoyun Zhu >
USC ID 		: < 2697-6003-85 >

Description: In this assignment, we use Catmull-Rom splines along with OpenGL core profile shader-based texture mapping and Phong shading to create a roller coaster simulation.

Core Credit Features: (Answer these Questions with Y/N; you can also insert comments as appropriate)
======================

1. Uses OpenGL core profile, version 3.2 or higher - Y

2. Completed all Levels:
  Level 1 : - Y
  level 2 : - Y
  Level 3 : - Y
  Level 4 : - Y
  Level 5 : - Y

3. Rendered the camera at a reasonable speed in a continuous path/orientation - Y

4. Run at interactive frame rate (>15fps at 1280 x 720) - Y

5. Understandably written, well commented code - Y

6. Attached an Animation folder containing not more than 1000 screenshots - Y

7. Attached this ReadMe File - Y

Extra Credit Features: (Answer these Questions with Y/N; you can also insert comments as appropriate)
======================

1. Render a T-shaped rail cross section - N

2. Render a Double Rail - N

3. Made the track circular and closed it with C1 continuity - N

4. Any Additional Scene Elements? (list them here) - A sky-box.

5. Render a sky-box - Y

6. Create tracks that mimic real world roller coaster - N

7. Generate track from several sequences of splines - N

8. Draw splines using recursive subdivision - N

9. Render environment in a better manner -  A space texture.

10. Improved coaster normals - N

11. Modify velocity with which the camera moves - N

12. Derive the steps that lead to the physically realistic equation of updating u 
    - Actually yes, since the roller coaster is in the space, no gravity is applied. 
        If there's no energy loss in friction or spinning, the camera should be moving at a uniform speed,
        uniform as the curve itself is.
        So it's "physically realisitc" in some way.

Additional Features: (Please document any additional features you may have implemented other than the ones described above)
Press "v" to hide/show the rail.

Open-Ended Problems: (Please document approaches to any open-ended problems that you have tackled)
None.

Keyboard/Mouse controls: (Please document Keyboard/Mouse controls if any)
Press "v" to hide/show the rail.

Names of the .cpp files you made changes to:
1. hw2.cpp


Comments :
"make" and " ./hw1 ./track.txt" to run the program.

Try running the program with the rail set to be invisible by pressing "v".
Enjoy a slow hovering in the universe background taken by the Hubble Space Telescope.

