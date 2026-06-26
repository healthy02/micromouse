import pygame
import sys
import json
import threading
import queue
import math

CELL_SIZE = 50
SIDEBAR_WIDTH = 350
FPS = 60
COLORS = {
    "bg": (20, 20, 20),
    "undiscovered": (40, 40, 40),
    "discovered": (70, 70, 90),
    "wall": (255, 255, 255),
    "grid": (60, 60, 60),
    "text": (220, 220, 220),
    "done": (120, 220, 120),
    "segment": (255, 210, 90),
    "agent": (255, 120, 80),
}

def read_stdin(q):
    for line in sys.stdin:
        try:
            data = json.loads(line)
            q.put(data)
        except json.JSONDecodeError:
            pass

def draw_arrow(surface, color, cx, cy, angle_deg, size):
    angle_rad = math.radians(angle_deg)
    tip = (cx + size * math.cos(angle_rad), cy - size * math.sin(angle_rad))
    back_left = (cx + size * 0.8 * math.cos(angle_rad + 2.5), cy - size * 0.8 * math.sin(angle_rad + 2.5))
    back_right = (cx + size * 0.8 * math.cos(angle_rad - 2.5), cy - size * 0.8 * math.sin(angle_rad - 2.5))
    pygame.draw.polygon(surface, color, [tip, back_left, back_right])

def draw_wall(surface, color, x, y, maze_h, wall_type):
    thickness = 3
    bx = x * CELL_SIZE
    by = (maze_h - 1 - y) * CELL_SIZE
    if wall_type == 'n':
        pygame.draw.rect(surface, color, (bx, by, CELL_SIZE, thickness))
    elif wall_type == 's':
        pygame.draw.rect(surface, color, (bx, by + CELL_SIZE - thickness, CELL_SIZE, thickness))
    elif wall_type == 'e':
        pygame.draw.rect(surface, color, (bx + CELL_SIZE - thickness, by, thickness, CELL_SIZE))
    elif wall_type == 'w':
        pygame.draw.rect(surface, color, (bx, by, thickness, CELL_SIZE))

def reset_session(maze_w, maze_h, algorithm="", run_index=0, run_total=1):
    return {
        "maze_w": maze_w,
        "maze_h": maze_h,
        "algorithm": algorithm,
        "run_index": run_index,
        "run_total": run_total,
        "discovered_cells": set(),
        "discovered_walls": {},
        "path_history": [],
        "sim_done": False,
        "segment_done": False,
    }

def main():
    pygame.init()
    pygame.font.init()
    font = pygame.font.SysFont("monospace", 14)
    dist_font = pygame.font.SysFont("monospace", 12)
    flood_font = pygame.font.SysFont("monospace", 16, bold=True)
    title_font = pygame.font.SysFont("monospace", 18, bold=True)

    screen = pygame.display.set_mode((800, 600))
    pygame.display.set_caption("Micromouse Simulation")
    clock = pygame.time.Clock()

    q = queue.Queue()
    t = threading.Thread(target=read_stdin, args=(q,))
    t.daemon = True
    t.start()

    running = True
    state = None
    session = reset_session(16, 16)
    last_frame_time = pygame.time.get_ticks()

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        current_time = pygame.time.get_ticks()
        delay_ms = 50
        if state and state.get("agents"):
            agent = state["agents"][0]
            if agent.get("mode") == "SPEED_RUN":
                delay_ms = 10
            else:
                delay_ms = 100

        while current_time - last_frame_time >= delay_ms and not q.empty():
            new_state = q.get()
            msg_type = new_state.get("type")
            if msg_type == "done":
                session["sim_done"] = True
            elif msg_type == "segment_done":
                session["segment_done"] = True
            elif msg_type == "init":
                maze_w = new_state.get("maze_w", 16)
                maze_h = new_state.get("maze_h", 16)
                algorithm = new_state.get("algorithm", "Micromouse")
                run_index = new_state.get("run_index", 0)
                run_total = new_state.get("run_total", 1)
                session = reset_session(maze_w, maze_h, algorithm, run_index, run_total)
                state = None
                pygame.display.set_caption(f"Micromouse — {algorithm} ({run_index + 1}/{run_total})")
            else:
                session["segment_done"] = False
                state = new_state
                agent = state.get("agents", [{}])[0]
                pos = (agent.get("x"), agent.get("y"))
                if not session["path_history"] or session["path_history"][-1] != pos:
                    session["path_history"].append(pos)

                for c in agent.get("discovered_cells", []):
                    session["discovered_cells"].add((c["x"], c["y"]))
                    session["discovered_walls"][(c["x"], c["y"])] = c

            last_frame_time = current_time
            current_time = pygame.time.get_ticks()

        maze_w = session["maze_w"]
        maze_h = session["maze_h"]
        width = maze_w * CELL_SIZE + SIDEBAR_WIDTH
        height = maze_h * CELL_SIZE
        if screen.get_size() != (width, height):
            screen = pygame.display.set_mode((width, height))

        screen.fill(COLORS["bg"])

        agent = state.get("agents", [{}])[0] if state else {}
        selected_grid = agent.get("distance_grid")

        for x in range(maze_w):
            for y in range(maze_h):
                rect = pygame.Rect(x * CELL_SIZE, (maze_h - 1 - y) * CELL_SIZE, CELL_SIZE, CELL_SIZE)
                if (x, y) in session["discovered_cells"]:
                    pygame.draw.rect(screen, COLORS["discovered"], rect)

                    grid_value = None
                    if selected_grid and y < len(selected_grid) and x < len(selected_grid[y]):
                        grid_value = selected_grid[y][x]

                    if grid_value is not None:
                        text_surface = flood_font.render(str(grid_value), True, COLORS["text"])
                        tw, th = text_surface.get_size()
                        screen.blit(text_surface, (x * CELL_SIZE + CELL_SIZE // 2 - tw // 2, (maze_h - 1 - y) * CELL_SIZE + CELL_SIZE // 2 - th // 2))
                else:
                    pygame.draw.rect(screen, COLORS["undiscovered"], rect)

                pygame.draw.rect(screen, COLORS["grid"], rect, 1)

        for (x, y), w in session["discovered_walls"].items():
            if w.get("n"):
                draw_wall(screen, COLORS["wall"], x, y, maze_h, 'n')
            if w.get("e"):
                draw_wall(screen, COLORS["wall"], x, y, maze_h, 'e')
            if w.get("s"):
                draw_wall(screen, COLORS["wall"], x, y, maze_h, 's')
            if w.get("w"):
                draw_wall(screen, COLORS["wall"], x, y, maze_h, 'w')

        if state and agent:
            if len(session["path_history"]) > 1:
                points = []
                for (px, py) in session["path_history"]:
                    cx = px * CELL_SIZE + CELL_SIZE // 2
                    cy = (maze_h - 1 - py) * CELL_SIZE + CELL_SIZE // 2
                    points.append((cx, cy))
                pygame.draw.lines(screen, COLORS["agent"], False, points, 3)

            cx = agent["x"] * CELL_SIZE + CELL_SIZE // 2
            cy = (maze_h - 1 - agent["y"]) * CELL_SIZE + CELL_SIZE // 2
            draw_arrow(screen, COLORS["agent"], cx, cy, agent.get("angle", 90), CELL_SIZE // 2.2)

            sb_x = maze_w * CELL_SIZE + 20
            run_label = f"Run {session['run_index'] + 1}/{session['run_total']}"
            screen.blit(title_font.render(session["algorithm"], True, COLORS["segment"]), (sb_x, 16))
            screen.blit(font.render(run_label, True, COLORS["text"]), (sb_x, 42))
            screen.blit(font.render(f"Tick: {state['tick']}", True, COLORS["text"]), (sb_x, 62))
            target = f"({agent.get('target_x', '?')},{agent.get('target_y', '?')})"
            screen.blit(font.render(f"Target: {target}", True, COLORS["text"]), (sb_x, 82))

            y_offset = 110
            screen.blit(font.render(f"Mode: {agent.get('mode', '?')}", True, COLORS["text"]), (sb_x, y_offset))
            screen.blit(font.render(f"Pos: ({agent['x']},{agent['y']})", True, COLORS["text"]), (sb_x, y_offset + 20))
            screen.blit(font.render(f"Steps: {agent.get('total_steps', 0)}", True, COLORS["text"]), (sb_x, y_offset + 40))
            screen.blit(font.render(f"Turns: {agent.get('total_turns', 0)}", True, COLORS["text"]), (sb_x, y_offset + 60))
            screen.blit(font.render(f"Time: {agent.get('simulated_time', 0):.2f}s", True, COLORS["text"]), (sb_x, y_offset + 80))
            screen.blit(font.render(f"Cells: {agent.get('cells_explored', '?')}", True, COLORS["text"]), (sb_x, y_offset + 100))

            if session["segment_done"]:
                banner = title_font.render("Run complete — next algorithm loading", True, COLORS["segment"])
                screen.blit(banner, (sb_x, y_offset + 130))
            elif session["sim_done"]:
                banner = title_font.render("All runs complete", True, COLORS["done"])
                screen.blit(banner, (sb_x, y_offset + 130))

        pygame.display.flip()
        clock.tick(FPS)

    pygame.quit()

if __name__ == "__main__":
    main()
