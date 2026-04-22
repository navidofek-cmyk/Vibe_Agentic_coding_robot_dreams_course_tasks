"""Terminal Maze Game — Etapa 2: Levels"""
import curses
import time
import random
from collections import deque


# ── Maze generation (Recursive Backtracker / DFS) ────────────────────────────

def generate_maze(cols, rows):
    """Return 2D list of '#' and ' '. Outer border is always wall."""
    assert cols % 2 == 1 and rows % 2 == 1, "dims must be odd"
    grid = [['#'] * cols for _ in range(rows)]

    def carve(cx, cy):
        dirs = [(0, -2), (0, 2), (-2, 0), (2, 0)]
        random.shuffle(dirs)
        for dx, dy in dirs:
            nx, ny = cx + dx, cy + dy
            if 1 <= nx < cols - 1 and 1 <= ny < rows - 1 and grid[ny][nx] == '#':
                grid[cy + dy // 2][cx + dx // 2] = ' '
                grid[ny][nx] = ' '
                carve(nx, ny)

    grid[1][1] = ' '
    carve(1, 1)
    return grid


def is_solvable(grid, start, goal):
    """BFS — returns True if goal reachable from start."""
    rows, cols = len(grid), len(grid[0])
    visited = set()
    q = deque([start])
    visited.add(start)
    while q:
        x, y = q.popleft()
        if (x, y) == goal:
            return True
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < cols and 0 <= ny < rows and grid[ny][nx] != '#' and (nx, ny) not in visited:
                visited.add((nx, ny))
                q.append((nx, ny))
    return False


# Level configs: (cols, rows)  — must be odd
LEVELS = [
    (21, 11),
    (31, 15),
    (41, 19),
    (51, 21),
    (61, 23),
]


def make_level(cols=21, rows=11):
    """Generate a guaranteed-solvable maze; place player and goal."""
    while True:
        grid = generate_maze(cols, rows)
        start = (1, 1)
        goal = (cols - 2, rows - 2)
        grid[start[1]][start[0]] = ' '
        grid[goal[1]][goal[0]] = ' '
        if is_solvable(grid, start, goal):
            return grid, start, goal


# ── Rendering ─────────────────────────────────────────────────────────────────

WALL  = '█'
PLAYER = '@'
GOAL  = 'X'

def draw(stdscr, grid, player, goal, steps, elapsed):
    stdscr.erase()
    rows, cols = len(grid), len(grid[0])
    sh, sw = stdscr.getmaxyx()

    # HUD
    hud = f" Steps: {steps}   Time: {elapsed:.0f}s   WASD/arrows=move  Q=quit "
    try:
        stdscr.addstr(0, 0, hud[:sw - 1], curses.color_pair(2))
    except curses.error:
        pass

    # Maze
    for gy in range(rows):
        for gx in range(cols):
            sy = gy + 1
            sx = gx * 2        # double width for readability
            if sy >= sh or sx + 1 >= sw:
                continue
            if (gx, gy) == player:
                try:
                    stdscr.addstr(sy, sx, PLAYER + ' ', curses.color_pair(3))
                except curses.error:
                    pass
            elif (gx, gy) == goal:
                try:
                    stdscr.addstr(sy, sx, GOAL + ' ', curses.color_pair(4))
                except curses.error:
                    pass
            elif grid[gy][gx] == '#':
                try:
                    stdscr.addstr(sy, sx, WALL + ' ', curses.color_pair(1))
                except curses.error:
                    pass
            else:
                try:
                    stdscr.addstr(sy, sx, '  ')
                except curses.error:
                    pass

    stdscr.refresh()


def level_complete_screen(stdscr, level_num, steps, elapsed):
    stdscr.erase()
    sh, sw = stdscr.getmaxyx()
    msg = [
        "  ╔══════════════════════════════╗  ",
        f"  ║   Level {level_num} complete!          ║  ",
        f"  ║   Steps  : {steps:<6}             ║  ",
        f"  ║   Time   : {elapsed:.1f}s              ║  ",
        "  ║                              ║  ",
        "  ║   Press any key to continue  ║  ",
        "  ╚══════════════════════════════╝  ",
    ]
    y = max(0, sh // 2 - len(msg) // 2)
    for i, line in enumerate(msg):
        x = max(0, sw // 2 - len(line) // 2)
        try:
            stdscr.addstr(y + i, x, line, curses.color_pair(4) | curses.A_BOLD)
        except curses.error:
            pass
    stdscr.refresh()
    stdscr.nodelay(False)
    stdscr.getch()
    stdscr.nodelay(True)
    stdscr.timeout(100)


def win_screen(stdscr, total_steps, total_time):
    stdscr.erase()
    sh, sw = stdscr.getmaxyx()
    msg = [
        "  ╔══════════════════════════════╗  ",
        "  ║        YOU WIN!              ║  ",
        f"  ║  Total steps : {total_steps:<6}        ║  ",
        f"  ║  Total time  : {total_time:.1f}s          ║  ",
        "  ║                              ║  ",
        "  ║  Press any key to exit       ║  ",
        "  ╚══════════════════════════════╝  ",
    ]
    y = max(0, sh // 2 - len(msg) // 2)
    for i, line in enumerate(msg):
        x = max(0, sw // 2 - len(line) // 2)
        try:
            stdscr.addstr(y + i, x, line, curses.color_pair(4) | curses.A_BOLD)
        except curses.error:
            pass
    stdscr.refresh()
    stdscr.nodelay(False)
    stdscr.getch()


# ── Main loop ─────────────────────────────────────────────────────────────────

def run_level(stdscr, level_idx):
    """Run one level; return (steps, elapsed) or None if player quit."""
    cols, rows = LEVELS[level_idx]
    grid, player, goal = make_level(cols, rows)
    steps = 0
    start_time = time.time()

    while True:
        elapsed = time.time() - start_time
        draw(stdscr, grid, player, goal, steps, elapsed)
        key = stdscr.getch()

        if key == curses.KEY_RESIZE:
            continue
        if key in (ord('q'), ord('Q')):
            return None

        dx, dy = 0, 0
        if key in (curses.KEY_UP,    ord('w'), ord('W')): dy = -1
        elif key in (curses.KEY_DOWN,  ord('s'), ord('S')): dy =  1
        elif key in (curses.KEY_LEFT,  ord('a'), ord('A')): dx = -1
        elif key in (curses.KEY_RIGHT, ord('d'), ord('D')): dx =  1

        if dx or dy:
            nx, ny = player[0] + dx, player[1] + dy
            if 0 <= nx < len(grid[0]) and 0 <= ny < len(grid) and grid[ny][nx] != '#':
                player = (nx, ny)
                steps += 1

        if player == goal:
            elapsed = time.time() - start_time
            return steps, elapsed


def main(stdscr):
    curses.curs_set(0)
    stdscr.nodelay(True)
    stdscr.timeout(100)

    curses.start_color()
    curses.use_default_colors()
    curses.init_pair(1, curses.COLOR_WHITE,   -1)
    curses.init_pair(2, curses.COLOR_BLACK,   curses.COLOR_WHITE)
    curses.init_pair(3, curses.COLOR_GREEN,   -1)
    curses.init_pair(4, curses.COLOR_YELLOW,  -1)

    total_steps = 0
    total_time  = 0.0

    for i in range(len(LEVELS)):
        result = run_level(stdscr, i)
        if result is None:
            return
        steps, elapsed = result
        total_steps += steps
        total_time  += elapsed
        if i < len(LEVELS) - 1:
            level_complete_screen(stdscr, i + 1, steps, elapsed)

    win_screen(stdscr, total_steps, total_time)


if __name__ == '__main__':
    curses.wrapper(main)
