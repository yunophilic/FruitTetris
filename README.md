# Fruit Tetris
This is an interactive 2D game that combines features from Tetris and Falling Fruits. The different fruits are represented by different colors. When a row  is completely  filled,  it  is  removed  and  the  tiles above it will be moved one row down. When three same fruits are in a row or column, they will be removed and the tiles above them  will  be  moved  down. This game is coded in C++ with OpenGL libraries.

Falling Fruits: http://www.wordgames.com/falling-fruits.html

# System Requirements
- Linux OS
- GLEW (http://glew.sourceforge.net/index.html)
- freeglut (`sudo apt-get install freeglut3-dev`)

# How to play
```
$ make
$ ./FruitTetris
```

# Controls
- Up key to rotate the tetris block
- Left and Right key to move the tetris block left and right
- Down key to accelerate falling speed of the tetris block
- Space to shuffle color
- 'Q' to quit
- 'R' to restart

# Additional Features
- Score: +1 point for 3 same color in a row or column and +3 points for a full row.
- To make the game more difficult, I make a restriction that each individual cell of the new tile piece must have a different color.

# Known Bugs/Problems
- When there is more than 3 adjacent same colour (for example there are 5 same color in a row or column), the program will only remove three of the cells. This shouldn't really be a problem but this makes the game a bit weird (and harder too).
