import json
import math
import random
from pathlib import Path

ROAD_NAMES = [
    "Maple Street", "Oak Avenue", "Pine Road", "Cedar Lane", "Elm Street", 
    "Washington Avenue", "Main Street", "Park Road", "Sunset Boulevard", "Lakeview Drive", 
    "River Road", "Highland Avenue", "Ocean Way", "Forest Lane", "Meadow Drive", 
    "Valley Road", "Spring Street", "Autumn Lane", "Winter Boulevard", "Summer Avenue"
]

LANES_PER_DIRECTION = 3
TOTAL_LANES = LANES_PER_DIRECTION * 2
LANE_WIDTH_JSON = 6.5


def offset_route(points, offset):
    if len(points) < 2 or abs(offset) < 0.001:
        return [list(p) for p in points]

    result = []
    for i in range(len(points)):
        if i == 0:
            dx = points[1][0] - points[0][0]
            dy = points[1][1] - points[0][1]
        elif i == len(points) - 1:
            dx = points[i][0] - points[i - 1][0]
            dy = points[i][1] - points[i - 1][1]
        else:
            dx = points[i + 1][0] - points[i - 1][0]
            dy = points[i + 1][1] - points[i - 1][1]

        length = math.hypot(dx, dy)
        if length < 0.001:
            result.append(list(points[i]))
            continue

        nx = -dy / length
        ny = dx / length
        result.append([
            points[i][0] + nx * offset,
            points[i][1] + ny * offset,
        ])

    return result


def generate_lane_routes(points, route_index):
    routes = []

    for lane_idx in range(LANES_PER_DIRECTION):
        lane_center = (lane_idx + 0.5) * LANE_WIDTH_JSON

        routes.append({
            "id": f"route_{route_index}",
            "points": offset_route(points, lane_center),
        })
        route_index += 1

        routes.append({
            "id": f"route_{route_index}",
            "points": offset_route(list(reversed(points)), lane_center),
        })
        route_index += 1

    return routes, route_index


BUILDING_NAMES = [
    "Skyline Tower", "Central Plaza", "Grand Hotel", "City Hall", "Tech Hub", 
    "Financial Center", "Emerald Building", "Diamond Tower", "Ruby Heights", "Sapphire Place", 
    "Opal Station", "Pearl Heights", "Golden Tower", "Silver Plaza", "Bronze Building", 
    "Crystal Center", "Metro Station", "Art Museum", "Science Center", "History Museum", 
    "Central Library", "City Hospital", "University Main Building", "Stadium", "Arena"
]

def generate_grid_city(seed=None):
    if seed is not None:
        random.seed(seed)

    roads = []
    routes = []
    traffic_lights = []
    buildings = []
    crossings = []

    width = 1600
    height = 1200
    grid_size = 300

    # Generate horizontal roads
    road_index = 1
    route_index = 1
    
    horizontal_ys = [y for y in range(grid_size, height, grid_size)]
    vertical_xs = [x for x in range(grid_size, width, grid_size)]

    for y in horizontal_ys:
        points = [[x, y] for x in range(100, width, 20)]
        roads.append({
            "id": f"road_{road_index}",
            "name": random.choice(ROAD_NAMES),
            "lanes": TOTAL_LANES,
            "points": points
        })
        road_routes, route_index = generate_lane_routes(points, route_index)
        routes.extend(road_routes)
        road_index += 1

    # Generate vertical roads
    for x in vertical_xs:
        points = [[x, y] for y in range(100, height, 20)]
        roads.append({
            "id": f"road_{road_index}",
            "name": random.choice(ROAD_NAMES),
            "lanes": TOTAL_LANES,
            "points": points
        })
        road_routes, route_index = generate_lane_routes(points, route_index)
        routes.extend(road_routes)
        road_index += 1

    # Intersections
    signal_index = 1
    crossing_index = 1
    for x in vertical_xs:
        for y in horizontal_ys:
            # Add traffic lights at intersections
            traffic_lights.append({
                "id": f"signal_{signal_index}",
                "position": [x, y],
                "cycle": [random.randint(3,6), 2, random.randint(3,6)]
            })
            signal_index += 1
            
            # Add crossings near intersection
            crossings.append({
                "id": f"crossing_{crossing_index}",
                "points": [[x - 15, y - 15], [x + 15, y - 15]]
            })
            crossing_index += 1
            crossings.append({
                "id": f"crossing_{crossing_index}",
                "points": [[x - 15, y + 15], [x + 15, y + 15]]
            })
            crossing_index += 1

    # Add Buildings in blocks
    building_index = 1
    for i in range(len(vertical_xs) - 1):
        for j in range(len(horizontal_ys) - 1):
            if random.random() > 0.1:
                # Add multiple buildings per block
                x_start = vertical_xs[i] + 40
                x_end = vertical_xs[i+1] - 40
                y_start = horizontal_ys[j] + 40
                y_end = horizontal_ys[j+1] - 40
                
                # Split block into 4 buildings
                mid_x = (x_start + x_end) / 2
                mid_y = (y_start + y_end) / 2
                
                blocks = [
                    ([x_start, y_start], [mid_x-10, mid_y-10]),
                    ([mid_x+10, y_start], [x_end, mid_y-10]),
                    ([x_start, mid_y+10], [mid_x-10, y_end]),
                    ([mid_x+10, mid_y+10], [x_end, y_end])
                ]
                
                for b_start, b_end in blocks:
                    if random.random() > 0.2:
                        buildings.append({
                            "id": f"building_{building_index}",
                            "name": random.choice(BUILDING_NAMES),
                            "height": random.randint(40, 150),
                            "base": [
                                [b_start[0], b_start[1]],
                                [b_end[0], b_start[1]],
                                [b_end[0], b_end[1]],
                                [b_start[0], b_end[1]]
                            ]
                        })
                        building_index += 1

    return {
        "name": "Traffic Simulation Demo Area",
        "latitude": 6.9,
        "longitude": 79.8,
        "defaultTrafficLevel": "HEAVY",
        "roads": roads,
        "buildings": buildings,
        "trafficLights": traffic_lights,
        "crossings": crossings,
        "routes": routes
    }


def write_area(path, area):
    output_path = Path(path)
    with output_path.open("w", encoding="utf-8") as f:
        json.dump(area, f, indent=2)
    print(
        f"Generated {output_path} "
        f"({len(area['roads'])} roads, {len(area['routes'])} lane routes, "
        f"{LANES_PER_DIRECTION}x2 lanes per road)"
    )


if __name__ == "__main__":
    data_dir = Path(__file__).resolve().parent.parent / "data"
    write_area(data_dir / "demo_traffic_area.json", generate_grid_city(seed=42))
    write_area(data_dir / "random_city_area.json", generate_grid_city(seed=1337))
