"""Unit tests for maze generator and BFS solver."""
import sys
from maze import generate_maze, is_solvable, make_level

passed = failed = 0

def ok(name, cond):
    global passed, failed
    if cond:
        print(f"  PASS: {name}")
        passed += 1
    else:
        print(f"  FAIL: {name}")
        failed += 1


# generate_maze
grid = generate_maze(11, 9)
ok("grid has correct rows", len(grid) == 9)
ok("grid has correct cols", len(grid[0]) == 11)
ok("outer top wall", all(c == '#' for c in grid[0]))
ok("outer bottom wall", all(c == '#' for c in grid[-1]))
ok("outer left wall", all(grid[r][0] == '#' for r in range(9)))
ok("outer right wall", all(grid[r][10] == '#' for r in range(9)))
ok("has passable cells", any(grid[r][c] == ' ' for r in range(9) for c in range(11)))

# is_solvable
simple = [
    list("#####"),
    list("# # #"),
    list("### #"),
    list("#   #"),
    list("#####"),
]
# top-right cell isolated from bottom-left area
ok("unsolvable: isolated cell unreachable", not is_solvable(simple, (3, 3), (1, 1)))

open_grid = [list("   "), list("   "), list("   ")]
ok("fully open grid solvable", is_solvable(open_grid, (0, 0), (2, 2)))

blocked = [list("###"), list("###"), list("###")]
ok("fully blocked unsolvable", not is_solvable(blocked, (0, 0), (2, 2)))

# make_level — check solvability for several sizes
for cols, rows in [(11, 9), (21, 11), (31, 15)]:
    g, start, goal = make_level(cols, rows)
    ok(f"make_level {cols}x{rows} solvable", is_solvable(g, start, goal))
    ok(f"make_level {cols}x{rows} player on passable", g[start[1]][start[0]] == ' ')
    ok(f"make_level {cols}x{rows} goal on passable", g[goal[1]][goal[0]] == ' ')

print(f"\nPassed: {passed}/{passed + failed}")
sys.exit(0 if failed == 0 else 1)
