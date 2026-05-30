import json
from pathlib import Path

INPUT_FILE = Path("../data/osm/moratuwa_raw_osm.json")
OUTPUT_FILE = Path("../data/moratuwa_area_real.json")

SCALE = 90000
OFFSET_X = 500
OFFSET_Y = 350

MAX_ROADS = 30
MAX_BUILDINGS = 70
MAX_SIGNALS = 6
MAX_CROSSINGS = 12


def load_geojson(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def collect_all_coordinates(features):
    coords = []

    def add_coord_pair(pair):
        lon, lat = pair[0], pair[1]
        coords.append((lat, lon))

    for feature in features:
        geometry = feature.get("geometry", {})
        gtype = geometry.get("type")
        coordinates = geometry.get("coordinates", [])

        if gtype == "Point":
            add_coord_pair(coordinates)

        elif gtype == "LineString":
            for pair in coordinates:
                add_coord_pair(pair)

        elif gtype == "Polygon":
            if coordinates:
                for pair in coordinates[0]:
                    add_coord_pair(pair)

        elif gtype == "MultiPolygon":
            for polygon in coordinates:
                if polygon:
                    for pair in polygon[0]:
                        add_coord_pair(pair)

        elif gtype == "MultiLineString":
            for line in coordinates:
                for pair in line:
                    add_coord_pair(pair)

    return coords


def find_center(coords):
    if not coords:
        raise RuntimeError("No coordinates found in GeoJSON file.")

    center_lat = sum(lat for lat, lon in coords) / len(coords)
    center_lon = sum(lon for lat, lon in coords) / len(coords)

    return center_lat, center_lon


def latlon_to_xy(lat, lon, center_lat, center_lon):
    x = (lon - center_lon) * SCALE + OFFSET_X
    y = (center_lat - lat) * SCALE + OFFSET_Y
    return [round(x, 2), round(y, 2)]


def convert_point(pair, center_lat, center_lon):
    lon, lat = pair[0], pair[1]
    return latlon_to_xy(lat, lon, center_lat, center_lon)


def get_polygon_outer_ring(geometry):
    gtype = geometry.get("type")
    coordinates = geometry.get("coordinates", [])

    if gtype == "Polygon":
        if coordinates:
            return coordinates[0]

    if gtype == "MultiPolygon":
        if coordinates and coordinates[0]:
            return coordinates[0][0]

    return []


def get_line_points(geometry):
    gtype = geometry.get("type")
    coordinates = geometry.get("coordinates", [])

    if gtype == "LineString":
        return [coordinates]

    if gtype == "MultiLineString":
        return coordinates

    return []


def estimate_building_height(properties, index):
    if "height" in properties:
        try:
            raw_height = str(properties["height"]).replace("m", "").strip()
            return max(25, min(float(raw_height) * 4, 140))
        except ValueError:
            pass

    if "building:levels" in properties:
        try:
            levels = int(float(properties["building:levels"]))
            return max(30, min(levels * 22, 140))
        except ValueError:
            pass

    return 35 + (index % 5) * 15


def convert_buildings(features, center_lat, center_lon):
    buildings = []
    index = 1

    for feature in features:
        properties = feature.get("properties", {})
        geometry = feature.get("geometry", {})

        if "building" not in properties:
            continue

        ring = get_polygon_outer_ring(geometry)

        if len(ring) < 4:
            continue

        base = []

        for pair in ring:
            base.append(convert_point(pair, center_lat, center_lon))

        if len(base) > 2 and base[0] == base[-1]:
            base.pop()

        if len(base) >= 3:
            buildings.append({
                "id": f"building_{index}",
                "name": properties.get("name", f"Building {index}"),
                "base": base,
                "height": estimate_building_height(properties, index)
            })

            index += 1

        if len(buildings) >= MAX_BUILDINGS:
            break

    return buildings


def convert_roads(features, center_lat, center_lon):
    roads = []
    index = 1

    allowed_highways = {
        "primary",
        "secondary",
        "tertiary",
        "residential",
        "service",
        "unclassified",
        "living_street",
        "road",
        "pedestrian"
    }

    for feature in features:
        properties = feature.get("properties", {})
        geometry = feature.get("geometry", {})

        highway = properties.get("highway")

        if highway not in allowed_highways:
            continue

        line_groups = get_line_points(geometry)

        for line in line_groups:
            points = []

            for pair in line:
                points.append(convert_point(pair, center_lat, center_lon))

            if len(points) >= 2:
                roads.append({
                    "id": f"road_{index}",
                    "name": properties.get("name", f"Road {index}"),
                    "points": points,
                    "lanes": 2 if highway in {"primary", "secondary", "tertiary"} else 1
                })

                index += 1

            if len(roads) >= MAX_ROADS:
                break

        if len(roads) >= MAX_ROADS:
            break

    return roads


def convert_traffic_lights(features, center_lat, center_lon):
    signals = []
    index = 1

    for feature in features:
        properties = feature.get("properties", {})
        geometry = feature.get("geometry", {})

        if properties.get("highway") != "traffic_signals":
            continue

        if geometry.get("type") == "Point":
            signals.append({
                "id": f"signal_{index}",
                "position": convert_point(geometry["coordinates"], center_lat, center_lon),
                "cycle": [4, 2, 4]
            })
            index += 1

        if len(signals) >= MAX_SIGNALS:
            break

    return signals


def convert_crossings(features, center_lat, center_lon):
    crossings = []
    index = 1

    for feature in features:
        properties = feature.get("properties", {})
        geometry = feature.get("geometry", {})

        if properties.get("highway") != "crossing":
            continue

        if geometry.get("type") == "Point":
            x, y = convert_point(geometry["coordinates"], center_lat, center_lon)

            crossings.append({
                "id": f"crossing_{index}",
                "points": [[x - 22, y - 8], [x + 22, y + 8]]
            })

            index += 1

        if len(crossings) >= MAX_CROSSINGS:
            break

    return crossings


def generate_routes_from_roads(roads):
    routes = []

    for index, road in enumerate(roads[:6], start=1):
        if len(road["points"]) >= 2:
            routes.append({
                "id": f"route_{index}",
                "points": road["points"]
            })

    return routes


def add_fallback_signals(traffic_lights, roads):
    if traffic_lights:
        return traffic_lights

    signals = []

    for index, road in enumerate(roads[:3], start=1):
        mid = road["points"][len(road["points"]) // 2]

        signals.append({
            "id": f"signal_{index}",
            "position": mid,
            "cycle": [4, 2, 4]
        })

    return signals


def main():
    geojson = load_geojson(INPUT_FILE)

    if geojson.get("type") != "FeatureCollection":
        raise RuntimeError("This converter expects GeoJSON FeatureCollection format.")

    features = geojson.get("features", [])

    if not features:
        raise RuntimeError("No features found in GeoJSON file.")

    coords = collect_all_coordinates(features)
    center_lat, center_lon = find_center(coords)

    roads = convert_roads(features, center_lat, center_lon)
    buildings = convert_buildings(features, center_lat, center_lon)
    traffic_lights = convert_traffic_lights(features, center_lat, center_lon)
    crossings = convert_crossings(features, center_lat, center_lon)
    routes = generate_routes_from_roads(roads)

    traffic_lights = add_fallback_signals(traffic_lights, roads)

    output = {
        "name": "University of Moratuwa Real GeoJSON Area",
        "latitude": center_lat,
        "longitude": center_lon,
        "defaultTrafficLevel": "MODERATE",
        "roads": roads,
        "buildings": buildings,
        "trafficLights": traffic_lights,
        "crossings": crossings,
        "routes": routes
    }

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        json.dump(output, f, indent=2)

    print("Converted GeoJSON data successfully.")
    print(f"Roads: {len(roads)}")
    print(f"Buildings: {len(buildings)}")
    print(f"Traffic lights: {len(traffic_lights)}")
    print(f"Crossings: {len(crossings)}")
    print(f"Routes: {len(routes)}")
    print(f"Saved to: {OUTPUT_FILE}")


if __name__ == "__main__":
    main()