import json
import random
from pathlib import Path

ROAD_NAMES = [
    "Maple Street", "Oak Avenue", "Pine Road", "Cedar Lane", "Elm Street", 
    "Washington Avenue", "Main Street", "Park Road", "Sunset Boulevard", "Lakeview Drive", 
    "River Road", "Highland Avenue", "Ocean Way", "Forest Lane", "Meadow Drive", 
    "Valley Road", "Spring Street", "Autumn Lane", "Winter Boulevard", "Summer Avenue"
]

BUILDING_NAMES = [
    "Skyline Tower", "Central Plaza", "Grand Hotel", "City Hall", "Tech Hub", 
    "Financial Center", "Emerald Building", "Diamond Tower", "Ruby Heights", "Sapphire Place", 
    "Opal Station", "Pearl Heights", "Golden Tower", "Silver Plaza", "Bronze Building", 
    "Crystal Center", "Metro Station", "Art Museum", "Science Center", "History Museum", 
    "Central Library", "City Hospital", "University Main Building", "Stadium", "Arena"
]

def generate_grid_city():
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
            "lanes": 2,
            "points": points
        })
        routes.append({"id": f"route_{route_index}", "points": points})
        route_index += 1
        routes.append({"id": f"route_{route_index}", "points": list(reversed(points))})
        route_index += 1
        road_index += 1

    # Generate vertical roads
    for x in vertical_xs:
        points = [[x, y] for y in range(100, height, 20)]
        roads.append({
            "id": f"road_{road_index}",
            "name": random.choice(ROAD_NAMES),
            "lanes": 2,
            "points": points
        })
        routes.append({"id": f"route_{route_index}", "points": points})
        route_index += 1
        routes.append({"id": f"route_{route_index}", "points": list(reversed(points))})
        route_index += 1
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

    output = {
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

    output_path = Path("../data/random_city_area.json")
    with open(output_path, "w") as f:
        json.dump(output, f, indent=2)

    print(f"Generated {output_path} with completely connected grid network.")

if __name__ == "__main__":
    generate_grid_city()
