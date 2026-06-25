import pygame
import sys
import json
import threading
import queue
import math

# Config
CELL_SIZE = 50
SIDEBAR_WIDTH = 350
FPS = 60
COLORS = {
    "bg": (20, 20, 20),
    "undiscovered": (40, 40, 40),
    "discovered": (70, 70, 90),
    "true_wall": (50, 20, 20),
    "wall": (255, 255, 255),
    "grid": (60, 60, 60),
    "text": (220, 220, 220),
    "agent_0": (255, 80, 80),   # Red
    "agent_1": (80, 255, 80),   # Green
    "agent_2": (80, 80, 255),   # Blue
    "agent_3": (255, 255, 80),  # Yellow
}

def read_stdin(q):
    for line in sys.stdin:
        try:
            data = json.loads(line)
            q.put(data)
        except json.JSONDecodeError:
            pass

def draw_arrow(surface, color, cx, cy, angle_deg, size):
    # Pygame coordinate system: y increases downwards.
    # Angle 0 is East. We must negate the angle or y computation to point correctly.
    # Wait: dy in our logic was up = positive y, which means in pygame up is negative y.
    # angle_deg is computed as atan2(dy, dx), so angle 90 is North.
    # In pygame, to go North, we go negative y.
    angle_rad = math.radians(angle_deg)
    
    # We will draw a triangle: tip, back_left, back_right
    tip = (cx + size * math.cos(angle_rad), cy - size * math.sin(angle_rad))
    back_left = (cx + size * 0.8 * math.cos(angle_rad + 2.5), cy - size * 0.8 * math.sin(angle_rad + 2.5))
    back_right = (cx + size * 0.8 * math.cos(angle_rad - 2.5), cy - size * 0.8 * math.sin(angle_rad - 2.5))
    pygame.draw.polygon(surface, color, [tip, back_left, back_right])

def draw_wall(surface, color, x, y, maze_h, wall_type):
    # wall_type: 'n', 'e', 's', 'w'
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

def main():
    pygame.init()
    pygame.font.init()
    font = pygame.font.SysFont("monospace", 14)
    dist_font = pygame.font.SysFont("monospace", 12)
    flood_font = pygame.font.SysFont("monospace", 16, bold=True)
    
    screen = pygame.display.set_mode((800, 600))
    pygame.display.set_caption("Micromouse Dual-Language Sim")
    clock = pygame.time.Clock()
    
    q = queue.Queue()
    t = threading.Thread(target=read_stdin, args=(q,))
    t.daemon = True
    t.start()
    
    running = True
    state = None
    maze_w, maze_h = 16, 16 
    true_walls = {}
    discovered_cells = set()
    discovered_walls = {}
    path_histories = {}
    selected_agent = 0
    
    # Time control
    last_frame_time = pygame.time.get_ticks()
    
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN and event.key == pygame.K_TAB:
                if state and state.get("agents"):
                    selected_agent = (selected_agent + 1) % len(state["agents"])
                
        current_time = pygame.time.get_ticks()
        
        delay_ms = 50
        if state and "agents" in state:
            if all(a.get("mode") == "SPEED_RUN" for a in state["agents"]):
                delay_ms = 10 # fast speed run
            else:
                delay_ms = 100 # slower exploration

        if current_time - last_frame_time >= delay_ms and not q.empty():
            new_state = q.get()
            if new_state.get("type") == "done":
                pass
            elif new_state.get("type") == "init":
                maze_w = new_state.get("maze_w", 16)
                maze_h = new_state.get("maze_h", 16)
                if "true_walls" in new_state:
                    for w in new_state["true_walls"]:
                        true_walls[(w["x"], w["y"])] = w
            else:
                state = new_state
                for a in state.get("agents", []):
                    aid = a["id"]
                    if aid not in path_histories:
                        path_histories[aid] = []
                    # Avoid duplicate adjacent path points
                    if not path_histories[aid] or path_histories[aid][-1] != (a["x"], a["y"]):
                        path_histories[aid].append((a["x"], a["y"]))
                    
                    if "discovered_cells" in a:
                        for c in a["discovered_cells"]:
                            discovered_cells.add((c["x"], c["y"]))
                            discovered_walls[(c["x"], c["y"])] = c
                            
                last_frame_time = current_time

        width = maze_w * CELL_SIZE + SIDEBAR_WIDTH
        height = maze_h * CELL_SIZE
        if screen.get_size() != (width, height):
            screen = pygame.display.set_mode((width, height))
            
        screen.fill(COLORS["bg"])
        
        agents = state.get("agents", []) if state else []
        selected_state = agents[selected_agent % len(agents)] if agents else None
        selected_grid = selected_state.get("distance_grid") if selected_state else None

        # Draw cells
        for x in range(maze_w):
            for y in range(maze_h):
                rect = pygame.Rect(x * CELL_SIZE, (maze_h - 1 - y) * CELL_SIZE, CELL_SIZE, CELL_SIZE)
                if (x, y) in discovered_cells:
                    pygame.draw.rect(screen, COLORS["discovered"], rect)
                    
                    grid_value = None
                    if selected_grid and y < len(selected_grid) and x < len(selected_grid[y]):
                        grid_value = selected_grid[y][x]

                    if grid_value is not None:
                        text_surface = flood_font.render(str(grid_value), True, COLORS["text"])
                        tw, th = text_surface.get_size()
                        screen.blit(text_surface, (x * CELL_SIZE + CELL_SIZE//2 - tw//2, (maze_h - 1 - y) * CELL_SIZE + CELL_SIZE//2 - th//2))
                else:
                    pygame.draw.rect(screen, COLORS["undiscovered"], rect)
                    grid_value = None
                    if selected_grid and y < len(selected_grid) and x < len(selected_grid[y]):
                        grid_value = selected_grid[y][x]
                    if grid_value is not None:
                        text_surface = dist_font.render(str(grid_value), True, COLORS["grid"])
                        tw, th = text_surface.get_size()
                        screen.blit(text_surface, (x * CELL_SIZE + CELL_SIZE//2 - tw//2, (maze_h - 1 - y) * CELL_SIZE + CELL_SIZE//2 - th//2))
                    
                pygame.draw.rect(screen, COLORS["grid"], rect, 1)

        # Draw True Walls
        for (x, y), w in true_walls.items():
            if w.get("n"): draw_wall(screen, COLORS["true_wall"], x, y, maze_h, 'n')
            if w.get("e"): draw_wall(screen, COLORS["true_wall"], x, y, maze_h, 'e')
            if w.get("s"): draw_wall(screen, COLORS["true_wall"], x, y, maze_h, 's')
            if w.get("w"): draw_wall(screen, COLORS["true_wall"], x, y, maze_h, 'w')

        # Draw Discovered Walls
        for (x, y), w in discovered_walls.items():
            if w.get("n"): draw_wall(screen, COLORS["wall"], x, y, maze_h, 'n')
            if w.get("e"): draw_wall(screen, COLORS["wall"], x, y, maze_h, 'e')
            if w.get("s"): draw_wall(screen, COLORS["wall"], x, y, maze_h, 's')
            if w.get("w"): draw_wall(screen, COLORS["wall"], x, y, maze_h, 'w')

        if state and "agents" in state:
            # Draw Path Traces
            for a in state["agents"]:
                aid = a["id"]
                color = COLORS.get(f"agent_{aid}", (255, 255, 255))
                if aid in path_histories and len(path_histories[aid]) > 1:
                    points = []
                    for (px, py) in path_histories[aid]:
                        cx = px * CELL_SIZE + CELL_SIZE // 2
                        cy = (maze_h - 1 - py) * CELL_SIZE + CELL_SIZE // 2
                        points.append((cx, cy))
                    pygame.draw.lines(screen, color, False, points, 3)
            
            # Draw Agents as Arrows
            for a in state["agents"]:
                x = a["x"]
                y = a["y"]
                angle = a["angle"]
                cx = x * CELL_SIZE + CELL_SIZE // 2
                cy = (maze_h - 1 - y) * CELL_SIZE + CELL_SIZE // 2
                color = COLORS.get(f"agent_{a['id']}", (255, 255, 255))
                draw_arrow(screen, color, cx, cy, angle, CELL_SIZE // 2.2)
                
            # Draw sidebar
            sb_x = maze_w * CELL_SIZE + 20
            screen.blit(font.render(f"Tick: {state['tick']}", True, COLORS["text"]), (sb_x, 20))
            if selected_state:
                target = f"({selected_state.get('target_x', '?')},{selected_state.get('target_y', '?')})"
                screen.blit(font.render(f"Grid: Mouse {selected_state['id']} -> {target}", True, COLORS["text"]), (sb_x, 40))
            
            y_offset = 80
            for idx, a in enumerate(state["agents"]):
                color = COLORS.get(f"agent_{a['id']}", (255, 255, 255))
                marker = ">" if idx == selected_agent % len(state["agents"]) else " "
                screen.blit(font.render(f"{marker} Mouse {a['id']} ({a['mode']})", True, color), (sb_x, y_offset))
                screen.blit(font.render(f" Algo: {a.get('algorithm', 'Unknown')}", True, COLORS["text"]), (sb_x, y_offset+20))
                screen.blit(font.render(f" Pos: ({a['x']},{a['y']})", True, COLORS["text"]), (sb_x, y_offset+40))
                screen.blit(font.render(f" Target: ({a.get('target_x', '?')},{a.get('target_y', '?')})", True, COLORS["text"]), (sb_x, y_offset+60))
                screen.blit(font.render(f" Steps: {a['total_steps']}", True, COLORS["text"]), (sb_x, y_offset+80))
                screen.blit(font.render(f" Turns: {a['total_turns']}", True, COLORS["text"]), (sb_x, y_offset+100))
                screen.blit(font.render(f" Time: {a['simulated_time']:.2f}s", True, COLORS["text"]), (sb_x, y_offset+120))
                y_offset += 160
                
        pygame.display.flip()
        clock.tick(FPS)
        
    pygame.quit()

if __name__ == "__main__":
    main()
